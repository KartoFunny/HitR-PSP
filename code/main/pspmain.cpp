//=============================================================================
// PspMain — entry point for PSP
//=============================================================================

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspiofilemgr.h>
#include <pspgu.h>
#include <string.h>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>

#include <raddebug.hpp>
#include <radobject.hpp>
#include <radmemory.hpp>
#include <radtime.hpp>

#include <main/game.h>
#include <main/pspplatform.h>
#include <main/singletons.h>
#include <main/commandlineoptions.h>
#include <memory/srrmemory.h>

PSP_MODULE_INFO("HitR_PSP", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
// КРИТИЧНО: без этого malloc зависает на первом вызове,
// потому что newlib heap по умолчанию нулевой.
PSP_HEAP_SIZE_KB(-1);  // PSP: динамический heap из системного partition   // 2 МБ newlib heap

static SceUID g_log = -1;

static void Log(const char* fmt, ...)
{
    if (g_log < 0) return;
    char buf[512];
    va_list a;
    va_start(a, fmt);
    vsnprintf(buf, sizeof(buf), fmt, a);
    va_end(a);
    int len = 0;
    while (buf[len]) len++;
    sceIoWrite(g_log, buf, len);
}

static int exit_cb(int, int, void*) { sceKernelExitGame(); return 0; }
static int cb_thread(SceSize, void*)
{
    int cbid = sceKernelCreateCallback("Exit CB", exit_cb, nullptr);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

int main(int argc, char* argv[])
{
    g_log = sceIoOpen("ms0:/hitr_boot.log",
                       PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    Log("=== HitR PSP boot ===\n");

    Log("[0] callback thread\n");
    int th = sceKernelCreateThread("cb", cb_thread, 0x11, 0xFA0, 0, 0);
    if (th >= 0) sceKernelStartThread(th, 0, 0);

    Log("[1] CommandLineOptions::InitDefaults\n");
    CommandLineOptions::InitDefaults();

    Log("[2] PspPlatform::InitializeFoundation\n");
    PspPlatform::InitializeFoundation();

    Log("[3] srand\n");
    srand( Game::GetRandomSeed() );	
    
    Log("[4] PushHeap(GMA_PERSISTENT)\n");
    HeapMgr()->PushHeap( GMA_PERSISTENT );

    Log("[5] CreateSingletons\n");
    CreateSingletons();

    Log("[6] PspPlatform::CreateInstance\n");
    PspPlatform* pPlatform = PspPlatform::CreateInstance();
    Log("[6] platform = %p\n", pPlatform);

    Log("[7] Game::CreateInstance\n");
    Game* pGame = Game::CreateInstance( pPlatform );
    Log("[7] game = %p\n", pGame);

    Log("[8] pGame->Initialize()\n");
    pGame->Initialize();
    Log("[8] Game::Initialize returned\n");

    Log("[9] PopHeap\n");
    HeapMgr()->PopHeap( GMA_PERSISTENT );

    Log("[10] pGame->Run()\n");
    pGame->Run();
    Log("[10] Game::Run returned\n");

    Log("[11] pGame->Terminate()\n");
    pGame->Terminate();

    DestroySingletons();
    Game::DestroyInstance();
    pPlatform->ShutdownPlatform();
    PspPlatform::DestroyInstance();
    PspPlatform::ShutdownMemory();

    Log("[12] exit\n");
    if (g_log >= 0) sceIoClose(g_log);
    sceKernelExitGame();
    return 0;
}
