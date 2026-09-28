//=============================================================================
// PoC v0.2: runtime test radcore + radmath на PSP
//=============================================================================

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspiofilemgr.h>
#include <cstdio>
#include <cstdarg>
#include <cstring>

#include <radmemory.hpp>
#include <radtime.hpp>
#include <radmath/radmath.hpp>

PSP_MODULE_INFO("HitR_PoC2", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static SceUID g_fd = -1;

// ---- файловый логгер через sceIo ----
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

// ---- exit callback ----
static int exit_cb(int, int, void*) { sceKernelExitGame(); return 0; }
static int cb_thread(SceSize, void*) {
    int cbid = sceKernelCreateCallback("Exit", exit_cb, nullptr);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

//=============================================================================
// TEST 1: radmath
//=============================================================================
static bool TestRadMath() {
    Log("\n--- radmath ---\n");

    rmt::Matrix m;
    m.Identity();
    Log("Identity row0: [%.2f %.2f %.2f %.2f]\n",
        m.m[0][0], m.m[0][1], m.m[0][2], m.m[0][3]);
    Log("Identity row3: [%.2f %.2f %.2f %.2f]\n",
        m.m[3][0], m.m[3][1], m.m[3][2], m.m[3][3]);

    rmt::Matrix t;
    t.Identity();
    t.FillTranslate(rmt::Vector(10.0f, 20.0f, 30.0f));
    Log("Translate -> row3: [%.1f %.1f %.1f]\n",
        t.m[3][0], t.m[3][1], t.m[3][2]);

    // Matrix multiply через Mult(a, b)
    rmt::Matrix r;
    r.Mult(m, t);
    Log("I * T -> row3: [%.1f %.1f %.1f]\n",
        r.m[3][0], r.m[3][1], r.m[3][2]);

    bool ok = (m.m[0][0] == 1.0f) &&
              (m.m[3][3] == 1.0f) &&
              (t.m[3][0] == 10.0f) &&
              (r.m[3][0] == 10.0f) &&
              (r.m[3][1] == 20.0f) &&
              (r.m[3][2] == 30.0f);
    Log("radmath: %s\n", ok ? "OK" : "FAIL");
    return ok;
}

//=============================================================================
// TEST 2: radtime
//=============================================================================
static bool TestRadTime() {
    Log("\n--- radtime ---\n");

    Log("calling radTimeGetMilliseconds...\n");
    unsigned int t0 = radTimeGetMilliseconds();
    Log("  t0 = %u\n", t0);

    volatile unsigned long sum = 0;
    for (unsigned long i = 0; i < 5000000UL; i++) sum += i;

    unsigned int t1 = radTimeGetMilliseconds();
    Log("  t1 = %u (delta=%u ms)\n", t1, t1 - t0);

    Log("calling radTimeGetMicroseconds...\n");
    unsigned int us = radTimeGetMicroseconds();
    Log("  us = %u\n", us);

    Log("calling radTimeGetMicroseconds64...\n");
    radTime64 us64 = radTimeGetMicroseconds64();
    Log("  us64 = %llu\n", (unsigned long long)us64);

    bool ok = (t1 >= t0);
    Log("radtime: %s\n", ok ? "OK" : "FAIL");
    return ok;
}

//=============================================================================
// TEST 3: radmemory
//=============================================================================
static bool TestRadMemory() {
    Log("\n--- radmemory ---\n");

    Log("calling radMemoryAlloc(DEFAULT, 1024)...\n");
    void* p1 = radMemoryAlloc(RADMEMORY_ALLOC_DEFAULT, 1024);
    Log("  p1 = %p\n", p1);

    Log("calling radMemoryAlloc(TEMP, 2048)...\n");
    void* p2 = radMemoryAlloc(RADMEMORY_ALLOC_TEMP, 2048);
    Log("  p2 = %p\n", p2);

    if (p1) memset(p1, 0xAB, 1024);
    if (p2) memset(p2, 0xCD, 2048);
    Log("  memsets done\n");

    if (p1) { Log("free p1...\n"); radMemoryFree(p1); }
    if (p2) { Log("free p2...\n"); radMemoryFree(p2); }
    Log("  frees done\n");

    bool ok = (p1 != nullptr) && (p2 != nullptr);
    Log("radmemory: %s\n", ok ? "OK" : "FAIL");
    return ok;
}

//=============================================================================
// main
//=============================================================================
int main() {
    g_fd = sceIoOpen("ms0:/psp_poc2.log",
                     PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);

    Log("=== HitR PSP PoC v0.2 ===\n");
    Log("Runtime test for radcore + radmath\n");

    Log("[s1] creating callback thread...\n");
    int th = sceKernelCreateThread("cb", cb_thread, 0x11, 0xFA0, 0, 0);
    if (th >= 0) {
        sceKernelStartThread(th, 0, 0);
        Log("[s1] thread started\n");
    } else {
        Log("[s1] FAILED: %d\n", th);
    }

    Log("[s2] radMemoryInitialize()...\n");
    radMemoryInitialize();
    Log("[s2] radMemoryInitialize OK\n");

    Log("[s3] radTimeInitialize()...\n");
    radTimeInitialize();
    Log("[s3] radTimeInitialize OK\n");

    Log("\n[s4] running tests...\n");
    int passed = 0, total = 0;

    Log("[test1] radmath...\n");
    total++;
    if (TestRadMath()) passed++;

    Log("[test2] radtime...\n");
    total++;
    if (TestRadTime()) passed++;

    Log("[test3] radmemory...\n");
    total++;
    if (TestRadMemory()) passed++;

    Log("\n==============================\n");
    Log("Results: %d / %d tests passed\n", passed, total);
    Log("==============================\n");

    Log("\nPress CIRCLE to exit.\n");

    SceCtrlData pad;
    while (1) {
        sceCtrlReadBufferPositive(&pad, 1);
        if (pad.Buttons & PSP_CTRL_CIRCLE) break;
        sceDisplayWaitVblankStart();
    }

    Log("\nShutting down...\n");
    radTimeTerminate();
    Log("radTimeTerminate OK\n");
    radMemoryTerminate();
    Log("radMemoryTerminate OK\n");

    if (g_fd >= 0) sceIoClose(g_fd);
    sceKernelExitGame();
    return 0;
}
