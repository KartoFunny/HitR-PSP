//=============================================================================
// platform.cpp — PSP-реализация tPlatform для Pure3D.
//=============================================================================

#include <p3d/platform/psp/platform.hpp>
#include <p3d/platform/psp/plat_filemap.hpp>
#include <p3d/error.hpp>
#include <p3d/context.hpp>
#include <p3d/utility.hpp>
#include <constants/version.hpp>

#include <pspkernel.h>

#include <cstdio>
#include <cstdlib>
#include <ctime>

//-----------------------------------------------------------------------------
tContextInitData::tContextInitData()
{
    xsize         = 480;
    ysize         = 272;
    bpp           = 32;
    displayMode   = PDDI_DISPLAY_FULLSCREEN;
    nColourBuffer = 2;
    bufferMask    = PDDI_BUFFER_COLOUR | PDDI_BUFFER_DEPTH;
}

//-----------------------------------------------------------------------------
tPlatform* tPlatform::currentPlatform = NULL;

tPlatform::tPlatform()
{
    currentContext = NULL;
    nContexts = 0;

    for (int i = 0; i < P3D_MAX_TASKS; ++i)
    {
        taskEntries[i].handle = 0;
        taskEntries[i].task   = NULL;
        taskEntries[i].active = false;
    }

    taskEntries[0].handle = (unsigned)sceKernelGetThreadId();
    taskEntries[0].active = true;
    taskCurrent = 0;
}

tPlatform::~tPlatform()
{
}

tPlatform*
tPlatform::Create()
{
    if (!currentPlatform)
    {
        p3d::UsePermanentMem(true);
        currentPlatform = new tPlatform();
        p3d::platform = currentPlatform;
        p3d::UsePermanentMem(false);
    }
    return currentPlatform;
}

void
tPlatform::Destroy(tPlatform* plat)
{
    P3DASSERT(plat == currentPlatform);
    delete currentPlatform;
    currentPlatform = NULL;
}

tContext*
tPlatform::CreateContext(tContextInitData* d)
{
    P3DASSERT(nContexts < P3D_MAX_CONTEXTS);

    PDDICREATEPROC PddiCreate;
    pddiDevice* device;
    pddiDisplay* display;
    pddiRenderContext* context;

    p3d::printf("Pure3D v%s, released %s\n", ATG_VERSION, ATG_RELEASE_DATE);

    p3d::UsePermanentMem(true);
    PddiCreate = pddiCreate;

    int success = PddiCreate(PDDI_VERSION_MAJOR, PDDI_VERSION_MINOR, &device);
    if (success != PDDI_OK)
    {
        if (success == PDDI_VERSION_ERROR)
            P3DVERIFY(0, "Cannot initialize PDDI library due to version mismatch");
        else
            P3DVERIFY(0, "Cannot initialize PDDI library, unknown error.");
    }

    tDebug::CapturePDDIMessages(device);

        display = device->NewDisplay(0);
    pddiDisplayInit initData;
    initData.xsize = d->xsize;
    initData.ysize = d->ysize;
    initData.bpp   = d->bpp;
    display->InitDisplay(&initData);

    context = device->NewRenderContext(display);
    P3DVERIFY(context != NULL, "NewRenderContext() failed");

    for (int find = 0; find < P3D_MAX_CONTEXTS; ++find)
    {
        if (!contexts[find].context)
        {
            contexts[find].context = new tContext(device, display, context);

            if (!currentContext)
                SetActiveContext(contexts[find].context);

            contexts[find].context->Setup();
            ++nContexts;

            p3d::UsePermanentMem(false);
            return contexts[find].context;
        }
    }

    p3d::UsePermanentMem(false);
    return NULL;
}

void
tPlatform::DestroyContext(tContext* context)
{
    int foundHandle = -1;
    for (int i = 0; i < nContexts; ++i)
        if (contexts[i].context == context)
            foundHandle = i;

    P3DASSERT(foundHandle != -1);

    context->Shutdown();

    contexts[foundHandle].windowHandle = NULL;
    delete context;
    contexts[foundHandle].context = NULL;
    --nContexts;
}

void
tPlatform::SetActiveContext(tContext* context)
{
    currentContext  = context;
    p3d::context    = context;
    p3d::inventory  = context->GetInventory();
    p3d::stack      = context->GetMatrixStack();
    p3d::loadManager= context->GetLoadManager();
    p3d::pddi       = context->GetContext();
    p3d::device     = context->GetDevice();
    p3d::display    = context->GetDisplay();
}

tFile*
tPlatform::OpenFile(const char* filename)
{
    tPspFileMap* file = new tPspFileMap(filename);
    if (!file->IsOpen())
    {
        file->Release();
        return NULL;
    }
    return file;
}

tPlatform*
tPlatform::GetPlatform(void)
{
    return currentPlatform;
}

void tPlatform::AddTask(tTask* /*t*/)    { /* TODO */ }
void tPlatform::RemoveTask(tTask* /*t*/) { /* TODO */ }
void tPlatform::CycleTasks(void)         { /* TODO */ }

#include <p3d/anim/polyskin.hpp>

tPolySkinLoader* tPolySkinLoaderCreate()
{
    return new tPolySkinLoader();
}

tPolySkinLoader* tPlatform::CreatePolySkinLoader(void)
{
    return tPolySkinLoaderCreate();
}
