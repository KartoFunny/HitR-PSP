#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspgu.h>
#include <pspiofilemgr.h>
#include <cstdio>
#include <cstdarg>

PSP_MODULE_INFO("HitR_Min", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static unsigned int __attribute__((aligned(16))) list[262144];
static SceUID g_fd = -1;

// Прямой sceIo — не требует инициализации newlib, работает всегда
static void Log(const char* fmt, ...) {
    if (g_fd < 0) return;

    char buf[400];
    va_list a;
    va_start(a, fmt);
    vsnprintf(buf, sizeof(buf), fmt, a);
    va_end(a);

    int len = 0;
    while (buf[len]) len++;
    sceIoWrite(g_fd, buf, len);
}

int main() {
    g_fd = sceIoOpen("ms0:/psp_min.log",
                     PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);

    Log("=== PSP min test started ===\n");
    Log("If you see this — sceIo works.\n");

    sceGuInit();
    Log("sceGuInit OK\n");

    sceGuStart(GU_DIRECT, list);
    sceGuDrawBuffer(GU_PSM_8888, (void*)0, 512);
    sceGuDispBuffer(480, 272, (void*)0x88000, 512);
    sceGuDepthBuffer((void*)0x110000, 512);
    sceGuOffset(2048 - 240, 2048 - 136);
    sceGuViewport(2048, 2048, 480, 272);
    sceGuScissor(0, 0, 480, 272);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuFinish();
    sceGuSync(0, 0);
    Log("GU buffers configured\n");

    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
    Log("Red screen should appear now.\n");

    SceCtrlData pad;
    int frame = 0;
    while (1) {
        sceGuStart(GU_DIRECT, list);
        sceGuClearColor(0xFF0000FF);
        sceGuClear(GU_COLOR_BUFFER_BIT);
        sceGuFinish();
        sceGuSync(0, 0);

        frame++;
        if (frame == 60)  Log("Alive: 60 frames\n");
        if (frame == 600) Log("Alive: 600 frames (10s)\n");

        sceCtrlReadBufferPositive(&pad, 1);
        if (pad.Buttons & PSP_CTRL_CIRCLE) break;

        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
    }

    Log("Exiting normally.\n");
    if (g_fd >= 0) sceIoClose(g_fd);
    sceGuTerm();
    sceKernelExitGame();
    return 0;
}
