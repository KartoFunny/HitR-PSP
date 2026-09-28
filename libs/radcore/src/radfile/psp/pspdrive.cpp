//=============================================================================
// pspdrive.cpp — PSP native drive for radfile
// Реализация на sceIo* API PSP (pspiofilemgr.h)
//=============================================================================

#include "pch.hpp"
#include <algorithm>
#include <limits.h>
#include <cstring>
#include <cctype>
#include <string>
#include "pspdrive.hpp"

#include <pspiofilemgr.h>
#include <pspkernel.h>

#ifdef RAD_PSP
#include <cstdarg>
static void PDLog(const char* fmt, ...) {
    SceUID fd = sceIoOpen("ms0:/hitr_pspdrive.log",
                          PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, 0777);
    if (fd < 0) return;
    char buf[400];
    va_list a; va_start(a, fmt);
    vsnprintf(buf, sizeof(buf), fmt, a);
    va_end(a);
    int l = 0; while (buf[l]) l++;
    sceIoWrite(fd, buf, l);
    sceIoClose(fd);
}
#define PDLOG(x) PDLog(x "\n")
#else
#define PDLOG(x) ((void)0)
#endif

//=============================================================================
// Фабрика
//=============================================================================
void radPspDriveFactory( radDrive** ppDrive, const char* pDriveName, radMemoryAllocator alloc )
{
    PDLog("[PSPDRIVE] Factory called: name=%s", pDriveName ? pDriveName : "(null)");
    *ppDrive = new( alloc ) radPspDrive( pDriveName, alloc );
    rAssert( *ppDrive != NULL );
}

//=============================================================================
// Конструктор
//=============================================================================
radPspDrive::radPspDrive( const char* pdrivespec, radMemoryAllocator alloc )
    : radDrive(),
      m_Capabilities( 0 ),
      m_OpenFiles( 0 ),
      m_pMutex( NULL )
{
    m_DriveName[0] = '\0';
    m_DrivePath[0] = '\0';

    // Создать mutex для lock/unlock
    radThreadCreateMutex( &m_pMutex, alloc );
    rAssert( m_pMutex != NULL );

    // Создать фоновый поток для драйва
    m_pDriveThread = new( alloc ) radDriveThread( m_pMutex, alloc );
    rAssert( m_pDriveThread != NULL );

    // Скопировать имя драйва
    strncpy( m_DriveName, pdrivespec, radFileDrivenameMax );
    m_DriveName[radFileDrivenameMax] = '\0';

    // Путь = имя драйва + "/"
    strncpy( m_DrivePath, pdrivespec, radFileFilenameMax - 2 );
    m_DrivePath[radFileFilenameMax - 2] = '\0';
    size_t len = strlen( m_DrivePath );
    if ( len > 0 && m_DrivePath[len - 1] != '/' && len < radFileFilenameMax - 1 )
    {
        m_DrivePath[len] = '/';
        m_DrivePath[len + 1] = '\0';
    }

    // Привести к верхнему регистру имя, к нижнему — путь
    for ( char* p = m_DriveName; *p; ++p ) *p = (char)toupper((unsigned char)*p);
    for ( char* p = m_DrivePath; *p; ++p ) *p = (char)tolower((unsigned char)*p);

    m_Capabilities = ( radDriveWriteable | radDriveFile );
}

radPspDrive::~radPspDrive( void )
{
    if ( m_pMutex ) { m_pMutex->Release(); m_pMutex = NULL; }
    if ( m_pDriveThread ) { m_pDriveThread->Release(); m_pDriveThread = NULL; }
}

//=============================================================================
// Lock / Unlock
//=============================================================================
void radPspDrive::Lock( void )   { m_pMutex->Lock(); }
void radPspDrive::Unlock( void ) { m_pMutex->Unlock(); }

unsigned int radPspDrive::GetCapabilities( void ) { return m_Capabilities; }
const char* radPspDrive::GetDriveName( void )     { return m_DriveName; }

//=============================================================================
// Initialize
//=============================================================================
radDrive::CompletionStatus radPspDrive::Initialize( void )
{
    SetMediaInfo();
    m_LastError = Success;
    return Complete;
}

//=============================================================================
// OpenFile
//=============================================================================
radDrive::CompletionStatus radPspDrive::OpenFile
(
    const char*         fileName,
    radFileOpenFlags    flags,
    bool                writeAccess,
    radFileHandle*      pHandle,
    unsigned int*       pSize
)
{
    char fullName[ radFileFilenameMax + 1 ];
    BuildFileSpec( fileName, fullName, radFileFilenameMax + 1 );

    // PSP-флаги открытия
    int createFlags = writeAccess ? PSP_O_RDWR : PSP_O_RDONLY;
    switch ( flags )
    {
    case OpenExisting:  break;
    case OpenAlways:    createFlags |= PSP_O_CREAT;               break;
    case CreateAlways:  createFlags |= PSP_O_CREAT | PSP_O_TRUNC; break;
    default:
        rAssertMsg( false, "radPspDrive: unknown open flag" );
        return Error;
    }

    PDLog("[PSPDRIVE] OpenFile: %s (flags=%d)", fullName, createFlags);
    SceUID uid = sceIoOpen( fullName, createFlags, 0777 );
    if ( uid < 0 )
    {
        *pHandle  = (radFileHandle)0;
        m_LastError = FileNotFound;
        return Error;
    }

    *pHandle = (radFileHandle)(intptr_t)uid;
    m_OpenFiles++;
    *pSize = sceIoLseek( uid, 0, PSP_SEEK_END );
    m_LastError = Success;
    return Complete;
}

//=============================================================================
// CloseFile
//=============================================================================
radDrive::CompletionStatus radPspDrive::CloseFile( radFileHandle handle, const char* /*fileName*/ )
{
    sceIoClose( (SceUID)(intptr_t)handle );
    if ( m_OpenFiles > 0 ) m_OpenFiles--;
    return Complete;
}

//=============================================================================
// ReadFile
//=============================================================================
radDrive::CompletionStatus radPspDrive::ReadFile
(
    radFileHandle   handle,
    const char*     /*fileName*/,
    IRadFile::BufferedReadState /*buffState*/,
    unsigned int    position,
    void*           pData,
    unsigned int    bytesToRead,
    unsigned int*   bytesRead,
    radMemorySpace  pDataSpace
)
{
    rAssertMsg( pDataSpace == radMemorySpace_Local,
                "radPspDrive: only local memory reads supported" );

    SceUID uid = (SceUID)(intptr_t)handle;

    // sceIoPread отсутствует в PSP SDK — используем seek + read
    if ( sceIoLseek( uid, (SceOff)position, PSP_SEEK_SET ) < 0 )
    {
        m_LastError = FileNotFound;
        return Error;
    }

    PDLog("[PSPDRIVE] ReadFile: pos=%u, bytes=%u", position, bytesToRead);
    int rd = sceIoRead( uid, pData, bytesToRead );
    if ( rd < 0 )
    {
        m_LastError = FileNotFound;
        return Error;
    }

    *bytesRead = (unsigned int)rd;
    m_LastError = Success;
    return Complete;
}

//=============================================================================
// WriteFile
//=============================================================================
radDrive::CompletionStatus radPspDrive::WriteFile
(
    radFileHandle     handle,
    const char*       /*fileName*/,
    IRadFile::BufferedReadState /*buffState*/,
    unsigned int      position,
    const void*       pData,
    unsigned int      bytesToWrite,
    unsigned int*     bytesWritten,
    unsigned int*     pSize,
    radMemorySpace    pDataSpace
)
{
    if ( !( m_Capabilities & radDriveWriteable ) )
    {
        return Error;
    }

    rAssertMsg( pDataSpace == radMemorySpace_Local,
                "radPspDrive: only local memory writes supported" );

    SceUID uid = (SceUID)(intptr_t)handle;

    // sceIoPwrite отсутствует — используем seek + write
    if ( sceIoLseek( uid, (SceOff)position, PSP_SEEK_SET ) < 0 )
    {
        m_LastError = FileNotFound;
        return Error;
    }

    int wr = sceIoWrite( uid, pData, bytesToWrite );
    if ( wr < 0 )
    {
        m_LastError = FileNotFound;
        return Error;
    }

    *bytesWritten = (unsigned int)wr;
    *pSize = sceIoLseek( uid, 0, PSP_SEEK_END );
    m_LastError = Success;
    return Complete;
}

//=============================================================================
// SetMediaInfo — заглушка, всегда "media present"
//=============================================================================
void radPspDrive::SetMediaInfo( void )
{
    PDLog("[PSPDRIVE] SetMediaInfo");
    strncpy( m_MediaInfo.m_VolumeName, m_DriveName, sizeof( m_MediaInfo.m_VolumeName ) - 1 );
    m_MediaInfo.m_VolumeName[ sizeof( m_MediaInfo.m_VolumeName ) - 1 ] = '\0';
    m_MediaInfo.m_SectorSize  = PSP_DEFAULT_SECTOR_SIZE;
    m_MediaInfo.m_MediaState  = IRadDrive::MediaInfo::MediaPresent;
    m_MediaInfo.m_FreeSpace   = UINT_MAX;
    m_MediaInfo.m_FreeFiles   = m_MediaInfo.m_FreeSpace / m_MediaInfo.m_SectorSize;
    m_LastError = Success;
}

//=============================================================================
// BuildFileSpec — склеить путь + имя, заменить '\' на '/'
//=============================================================================
void radPspDrive::BuildFileSpec( const char* fileName, char* fullName, unsigned int size )
{
    std::string path( m_DrivePath );
    path += fileName;
    std::replace( path.begin(), path.end(), '\\', '/' );

    strncpy( fullName, path.c_str(), size - 1 );
    fullName[ size - 1 ] = '\0';
}
