//=============================================================================
// psp_sdl_shim.hpp
//
// Заменяет SDL mutex / semaphore / thread API на PSP-вызовы.
// Включается только под RAD_PSP, до любых других radthread-заголовков.
//=============================================================================

#ifndef PSP_SDL_SHIM_HPP
#define PSP_SDL_SHIM_HPP

#ifdef RAD_PSP

#include <pspthreadman.h>
#include <cstdlib>
#include <cstdint>
#include <cstddef>

// Прикидываемся SDL2 — код выбирает простой путь:
//   SDL_CreateThreadWithStackSize + SDL_SetThreadPriority
//   вместо SDL3-пути с properties.
#ifndef SDL_MAJOR_VERSION
#define SDL_MAJOR_VERSION 2
#endif

// --- SDL_Mutex -------------------------------------------------------------
struct SDL_Mutex {
    int uid;   // SceUID семафора с count=1
};

static inline SDL_Mutex* SDL_CreateMutex(void)
{
    SDL_Mutex* m = (SDL_Mutex*)malloc(sizeof(SDL_Mutex));
    if (!m) return nullptr;
    m->uid = sceKernelCreateSema("radMutex", 0, 1, 1, nullptr);
    return m;
}

static inline void SDL_DestroyMutex(SDL_Mutex* m)
{
    if (!m) return;
    if (m->uid > 0) sceKernelDeleteSema(m->uid);
    free(m);
}

static inline void SDL_LockMutex(SDL_Mutex* m)
{
    if (m && m->uid > 0) sceKernelWaitSema(m->uid, 1, nullptr);
}

static inline void SDL_UnlockMutex(SDL_Mutex* m)
{
    if (m && m->uid > 0) sceKernelSignalSema(m->uid, 1);
}

// --- SDL_Semaphore ---------------------------------------------------------
struct SDL_Semaphore {
    int uid;   // SceUID семафора
};

static inline SDL_Semaphore* SDL_CreateSemaphore(unsigned int /*initial*/)
{
    SDL_Semaphore* s = (SDL_Semaphore*)malloc(sizeof(SDL_Semaphore));
    if (!s) return nullptr;
    s->uid = sceKernelCreateSema("radSem", 0, 0, 0x7FFFFFFF, nullptr);
    return s;
}

static inline void SDL_DestroySemaphore(SDL_Semaphore* s)
{
    if (!s) return;
    if (s->uid > 0) sceKernelDeleteSema(s->uid);
    free(s);
}

static inline int SDL_WaitSemaphore(SDL_Semaphore* s)
{
    if (s && s->uid > 0) sceKernelWaitSema(s->uid, 1, nullptr);
    return 0;
}

static inline int SDL_SignalSemaphore(SDL_Semaphore* s)
{
    if (s && s->uid > 0) sceKernelSignalSema(s->uid, 1);
    return 0;
}

// --- SDL2-имена (код игры использует их из-за SDL_MAJOR_VERSION < 3) ---
static inline int SDL_SemWait(SDL_Semaphore* s)   { return SDL_WaitSemaphore(s); }
static inline int SDL_SemPost(SDL_Semaphore* s)   { return SDL_SignalSemaphore(s); }
static inline SDL_Semaphore* SDL_CreateSemaphore2(unsigned int init) { return SDL_CreateSemaphore(init); }

// Совместимость с SDL2-именами типов
typedef SDL_Mutex     SDL_mutex;
typedef SDL_Semaphore SDL_sem;

// --- SDL_ThreadPriority / SDL_THREAD_PRIORITY_* ----------------------------
enum SDL_ThreadPriority {
    SDL_THREAD_PRIORITY_LOW          = 0,
    SDL_THREAD_PRIORITY_NORMAL       = 1,
    SDL_THREAD_PRIORITY_HIGH         = 2,
    SDL_THREAD_PRIORITY_TIME_CRITICAL = 3
};

// --- SDL_Thread ------------------------------------------------------------
typedef int (*SDL_ThreadFunction)(void* data);

struct SDL_Thread {
    int uid;                  // SceUID потока PSP
    SDL_ThreadFunction entry; // пользовательский entry
    void* userdata;
    int exitCode;
};

static inline unsigned long SDL_ThreadID(void)
{
    return (unsigned long)sceKernelGetThreadId();
}

static inline void SDL_Delay(unsigned int ms)
{
    // PSP принимает микросекунды
    sceKernelDelayThread(ms * 1000);
}

// Трамплин: sceKernelCreateThread ожидает int(SceSize, void*), а SDL — int(void*).
static inline int PspThreadTrampoline(SceSize /*args*/, void* argp)
{
    SDL_Thread* t = (SDL_Thread*)argp;
    int rc = (t && t->entry) ? t->entry(t->userdata) : 0;
    if (t) t->exitCode = rc;
    return rc;
}

static inline SDL_Thread* SDL_CreateThreadWithStackSize(
    SDL_ThreadFunction fn, const char* /*name*/, size_t stackSize, void* data)
{
    SDL_Thread* t = (SDL_Thread*)malloc(sizeof(SDL_Thread));
    if (!t) return nullptr;
    t->entry     = fn;
    t->userdata  = data;
    t->exitCode  = 0;

    // 0x20 — обычный пользовательский приоритет на PSP.
    // PSP_THREAD_ATTR_USER = 0x80000000, определён в pspthreadman.h.
    t->uid = sceKernelCreateThread("radThread", PspThreadTrampoline, 0x20,
                                   (int)stackSize, PSP_THREAD_ATTR_USER, nullptr);
    if (t->uid < 0) {
        free(t);
        return nullptr;
    }

    // Передаём указатель на SDL_Thread через argp.
    sceKernelStartThread(t->uid, sizeof(SDL_Thread*), &t);
    return t;
}

static inline void SDL_WaitThread(SDL_Thread* t, int* status)
{
    if (!t) return;
    sceKernelWaitThreadEnd(t->uid, nullptr);
    if (status) *status = t->exitCode;
    sceKernelDeleteThread(t->uid);
    free(t);
}

static inline void SDL_SetThreadPriority(SDL_ThreadPriority p)
{
    // PSP: 0x10 — выше обычного, 0x20 — обычный, 0x30 — ниже обычного.
    int pri = 0x20;
    switch (p) {
        case SDL_THREAD_PRIORITY_LOW:           pri = 0x30; break;
        case SDL_THREAD_PRIORITY_NORMAL:        pri = 0x20; break;
        case SDL_THREAD_PRIORITY_HIGH:          pri = 0x18; break;
        case SDL_THREAD_PRIORITY_TIME_CRITICAL: pri = 0x10; break;
    }
    // 0 = текущий поток
    sceKernelChangeThreadPriority(0, pri);
}

static inline void SDL_SetCurrentThreadPriority(SDL_ThreadPriority p) { SDL_SetThreadPriority(p); }

#endif // RAD_PSP
#endif // PSP_SDL_SHIM_HPP
