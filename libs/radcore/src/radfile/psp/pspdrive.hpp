//=============================================================================
// pspdrive.hpp — PSP native drive for radfile
// Порт vitadrive.hpp на PSP (sceIo* API)
//=============================================================================
#ifndef PSPDRIVE_HPP
#define PSPDRIVE_HPP

#include "../common/drive.hpp"
#include "../common/drivethread.hpp"

// Сектор по умолчанию (Memory Stick)
#define PSP_DEFAULT_SECTOR_SIZE  512

// Фабрика драйва — должна быть вызвана из PlatformDrivesFactory
void radPspDriveFactory( radDrive** ppDrive, const char* driveSpec, radMemoryAllocator alloc );

class radPspDrive : public radDrive
{
public:
    radPspDrive( const char* pdrivespec, radMemoryAllocator alloc );
    virtual ~radPspDrive( void );

    void Lock( void );
    void Unlock( void );

    unsigned int GetCapabilities( void );
    const char* GetDriveName( void );

    CompletionStatus Initialize( void );

    CompletionStatus OpenFile( const char*      fileName,
                               radFileOpenFlags flags,
                               bool             writeAccess,
                               radFileHandle*   pHandle,
                               unsigned int*    pSize );

    CompletionStatus CloseFile( radFileHandle handle, const char* fileName );

    CompletionStatus ReadFile( radFileHandle      handle,
                               const char*        fileName,
                               IRadFile::BufferedReadState buffState,
                               unsigned int       position,
                               void*              pData,
                               unsigned int       bytesToRead,
                               unsigned int*      bytesRead,
                               radMemorySpace     pDataSpace );

    CompletionStatus WriteFile( radFileHandle     handle,
                                const char*       fileName,
                                IRadFile::BufferedReadState buffState,
                                unsigned int      position,
                                const void*       pData,
                                unsigned int      bytesToWrite,
                                unsigned int*     bytesWritten,
                                unsigned int*     size,
                                radMemorySpace    pDataSpace );

private:
    void SetMediaInfo( void );
    void BuildFileSpec( const char* fileName, char* fullName, unsigned int size );

    unsigned int m_Capabilities;
    unsigned int m_OpenFiles;
    char         m_DriveName[ radFileDrivenameMax + 1 ];
    char         m_DrivePath[ radFileFilenameMax + 1 ];

    IRadThreadMutex* m_pMutex;
};

#endif // PSPDRIVE_HPP
