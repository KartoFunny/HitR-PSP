#include <pspkernel.h>
#include <pspiofilemgr.h>

PSP_MODULE_INFO("HitR_Min", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

int main(int, char**) {
    SceUID fd = sceIoOpen("ms0:/min.log",
                          PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    if (fd >= 0) {
        const char* msg = "loaded\n";
        sceIoWrite(fd, msg, 7);
        sceIoClose(fd);
    }
    sceKernelExitGame();
    return 0;
}
