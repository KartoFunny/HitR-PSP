//=============================================================================
// PspPlatform implementation (PSP port)
//=============================================================================

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspiofilemgr.h>
#include <pspctrl.h>
#include <pspgu.h>

#include <radmemory.hpp>
#include <radtime.hpp>
#include <radfile.hpp>
#include <raddebug.hpp>

#include <p3d/platform.hpp>
#include <p3d/context.hpp>
#include <p3d/utility.hpp>

#include <main/pspplatform.h>
#include <main/game.h>

#include <input/inputmanager.h>

#include <memory/memoryutilities.h>
#include <memory/srrmemory.h>

#include <radload/radload.hpp>
#include <radmovie2.hpp>
#include <radplatform.hpp>

PspPlatform* PspPlatform::spInstance = NULL;

// sceIo logger (свой локальный — чтобы не пересекаться с pspmain)
static SceUID g_plog = -1;
static void PLog(const char* fmt, ...)
{
    if (g_plog < 0) {
        g_plog = sceIoOpen("ms0:/hitr_platform.log",
                            PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
        if (g_plog < 0) return;
    }
    char buf[256];
    va_list a;
    va_start(a, fmt);
    vsnprintf(buf, sizeof(buf), fmt, a);
    va_end(a);
    int len = 0;
    while (buf[len]) len++;
    sceIoWrite(g_plog, buf, len);
}

//-----------------------------------------------------------------------------
PspPlatform::PspPlatform() : mpPlatform(nullptr), mpContext(nullptr) {}
PspPlatform::~PspPlatform() {}

PspPlatform* PspPlatform::CreateInstance()
{
    if (!spInstance) spInstance = new PspPlatform();
    return spInstance;
}
PspPlatform* PspPlatform::GetInstance() { return spInstance; }
void PspPlatform::DestroyInstance()
{
    if (spInstance) { delete spInstance; spInstance = nullptr; }
}

//-----------------------------------------------------------------------------
// Foundation Tech init
//-----------------------------------------------------------------------------

void PspPlatform::InitializeFoundation()
{
    PLog("[F] InitializeFoundation START\n");

    // radMemoryInitialize + radThreadInitialize уже вызваны через
    // constructor(101) в psp_early_init.cpp.

    // 1. Создаём heap'ы (PrepareHeapsStartup)
    PLog("[F] PrepareHeapsStartup\n");
    HeapMgr()->PrepareHeapsStartup();

    // 2. Закрепляем persistent heap на стеке
    PLog("[F] PushHeap(PERSISTENT)\n");
    HeapMgr()->PushHeap(GMA_PERSISTENT);

    // 3. Платформа (у нас — заглушка)
    PLog("[F] radPlatformInitialize\n");
    ::radPlatformInitialize();

    // 4. Время
    PLog("[F] radTimeInitialize\n");
    ::radTimeInitialize();

    // 5. Файловая система — КРИТИЧНО
    PLog("[F] radFileInitialize\n");
    ::radFileInitialize(50, 32, GMA_PERSISTENT);

    // 6. Загрузчик контента — КРИТИЧНО
    PLog("[F] radLoadInitialize\n");
    ::radLoadInitialize();

    // 7. Монтируем диск — КРИТИЧНО
    PLog("[F] radDriveMount(NULL)\n");
    ::radDriveMount(NULL, GMA_PERSISTENT);

    // 8. Movie player (опционально, но пусть будет)
    PLog("[F] radMovieInitialize2\n");
    ::radMovieInitialize2(GMA_PERSISTENT);

    PLog("[F] PopHeap(PERSISTENT)\n");
    HeapMgr()->PopHeap(GMA_PERSISTENT);

    PLog("[F] InitializeFoundation DONE\n");
}

#include <memory/memoryutilities.h>
#include <memory/srrmemory.h>

// PSP: declared in libs/pure3d/p3d/loaders.cpp
namespace p3d { void InstallDefaultLoaders(); }


// gMemorySystemInitialized объявлен в srrmemory.h
extern bool gMemorySystemInitialized;

void PspPlatform::InitializeMemory()
{
/*
    static bool s_done = false;
    if (s_done) return;
    s_done = true;

    PLog("[F] InitializeMemory: first and only call\n");

    PLog("[F] HeapMgr()->PrepareHeapsStartup\n");
    HeapMgr()->PrepareHeapsStartup();

    // КРИТИЧНО: этот флаг проверяется в operator new() (см. srrmemory.cpp).
    // Если он false, КАЖДЫЙ new запускает INIT_MEM() → бесконечный цикл.
    gMemorySystemInitialized = true;

    PLog("[F] heaps ready, gMemorySystemInitialized = true\n");
*/
}

void PspPlatform::ShutdownMemory() {}

void PspPlatform::InitializeFoundationDrive() {}

void PspPlatform::ShutdownFoundation()
{
    radTimeTerminate();
    radMemoryTerminate();
}

//-----------------------------------------------------------------------------
// GU init
//-----------------------------------------------------------------------------
static unsigned int __attribute__((aligned(16))) s_guList[262144];

void PspPlatform::InitializePlatform()
{
    PLog("[P] InitializePlatform START\n");

    // Всё делаем под GMA_PERSISTENT, как в Win32Platform::InitializePlatform()
    HeapMgr()->PushHeap(GMA_PERSISTENT);

    // 1. Создаём Pure3D платформу и контекст
    //    (заполняет p3d::platform, p3d::context, p3d::pddi, p3d::device, p3d::display)
    PLog("[P] InitializePure3D\n");
    InitializePure3D();

    // 2. Монтируем драйв (пока no-op, потому что PSP-драйва ещё нет)
    PLog("[P] InitializeFoundationDrive\n");
    InitializeFoundationDrive();

    // 3. Инициализируем ввод
    #ifdef RAD_PSP
    	PLog("[P] GetInputManager()->Init() SKIPPED for PSP\n");
    	// PSP: InputManager::Init() валится в radController (stub). Пропускаем.
    #else
    	PLog("[P] GetInputManager()->Init()\n");
    	GetInputManager()->Init();
    #endif
    HeapMgr()->PopHeap(GMA_PERSISTENT);

    PLog("[P] InitializePlatform DONE\n");
}

void PspPlatform::ShutdownPlatform() { sceGuTerm(); }

void PspPlatform::LaunchDashboard() {}
void PspPlatform::ResetMachine() {}

void PspPlatform::DisplaySplashScreen( SplashScreen, const char*, float, float, float, tColour, int ) {}
void PspPlatform::DisplaySplashScreen( const char*, const char*, float, float, float, tColour, int ) {}
void PspPlatform::OnControllerError(const char*) {}

//-----------------------------------------------------------------------------
// Pure3D init — создаём tPlatform и tContext
//-----------------------------------------------------------------------------
void PspPlatform::InitializePure3D()
{
    PLog("[3D] tPlatform::Create\n");
    mpPlatform = tPlatform::Create();
    PLog("[3D] platform = %p\n", mpPlatform);

    if (mpPlatform)
    {
        PLog("[3D] CreateContext\n");
        tContextInitData initData;
        initData.xsize = 480;
        initData.ysize = 272;
        initData.bpp   = 32;

        mpContext = mpPlatform->CreateContext(&initData);
        PLog("[3D] context = %p\n", mpContext);

        if (mpContext)
        {
            mpPlatform->SetActiveContext(mpContext);
            PLog("[3D] active context set\n");

#ifdef RAD_PSP
            // PSP: install default Pure3D file loaders (p3d, png, tga, bmp
            // + all chunk loaders). On other platforms this is done by
            // *platform.cpp when it installs its platform loaders. We have
            // to do it here, after p3d::loadManager and p3d::context are set.
            PLog("[3D] InstallDefaultLoaders\n");
            p3d::InstallDefaultLoaders();
            PLog("[3D] loaders installed\n");
#endif
        }
    }
}

void PspPlatform::ShutdownPure3D()
{
    if (mpPlatform) {
        tPlatform::Destroy(mpPlatform);
        mpPlatform = nullptr;
    }
}
