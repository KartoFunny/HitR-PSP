//=============================================================================
// platform.hpp — PSP-версия tPlatform для Pure3D.
//=============================================================================

#ifndef _PLATFORM_HPP
#define _PLATFORM_HPP

#include <p3d/platform/psp/plat_types.hpp>
#include <pddi/pddi.hpp>

static const int P3D_MAX_CONTEXTS = 4;
static const int P3D_MAX_TASKS = 4;

class tContext;
class tFile;
class tPolySkinLoader;
class tTask;

class tContextInitData
{
public:
    int               xsize;         // x resolution (480 по умолчанию)
    int               ysize;         // y resolution (272 по умолчанию)
    int               bpp;           // bits per pixel
    pddiDisplayMode   displayMode;   // у PSP всегда fullscreen
    unsigned          bufferMask;
    int               nColourBuffer;

    tContextInitData();
};

class tPlatformContext
{
public:
    tContext* context;
    void* windowHandle;
    void* pddiLib;

    tPlatformContext() { context = 0; windowHandle = 0; pddiLib = 0; }
};

class tPlatform
{
public:
    static tPlatform* Create();
    static void Destroy(tPlatform*);
    static tPlatform* GetPlatform(void);

    tContext* CreateContext(tContextInitData*);
    void DestroyContext(tContext*);

    void SetActiveContext(tContext*);
    tContext* GetActiveContext(void) { return currentContext; }

    tFile* OpenFile(const char* filename);

    void AddTask(tTask*);
    void RemoveTask(tTask*);
    void CycleTasks(void);

    tPolySkinLoader* CreatePolySkinLoader(void);

protected:
    tPlatform();
    ~tPlatform();

    static tPlatform* InternalCreate();
    static tPlatform* currentPlatform;

    tContext* currentContext;
    int       nContexts;
    tPlatformContext contexts[P3D_MAX_CONTEXTS];

    struct TaskEntry
    {
        unsigned handle;
        tTask*   task;
        bool     active;
    };
    TaskEntry taskEntries[P3D_MAX_TASKS];
    int       taskCurrent;
};

#endif
