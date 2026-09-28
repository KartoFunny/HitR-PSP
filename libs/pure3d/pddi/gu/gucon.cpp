//=============================================================================
// gucon.cpp — реализация pguContext (GU render context)
//=============================================================================
#include <pddi/pddi.hpp>
#include "gucon.hpp"
#include "gudev.hpp"
#include "gudisplay.hpp"
#include "guprim.hpp"

#include <pspgu.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <cstring>

// Общий список команд GU (используется display + context)
static unsigned int __attribute__((aligned(16))) s_ctx_list[65536];

unsigned int* pguContext::GetList()
{
    return s_ctx_list;
}

pguContext::pguContext(pddiDisplay* disp, pddiDevice* dev)
    : pddiBaseContext(disp, dev), m_refs(1), m_inFrame(false), m_frame_count(0)
{
}

pguContext::~pguContext()
{
}

void pguContext::AddRef()  { m_refs++; }
void pguContext::Release() { if (--m_refs <= 0) delete this; }

//=============================================================================
// Frame sync
//=============================================================================
void pguContext::BeginFrame()
{
    pddiBaseContext::BeginFrame();

    // Начинаем GU список команд
    sceGuStart(GU_DIRECT, s_ctx_list);

    // Очищаем экран цветом из state (по умолчанию — чёрный)
#ifdef RAD_PSP
    // FORCE black — we want to see sprites, not background colour
    sceGuClearColor(0xFF000000);
#else
    pddiColour bg = state.viewState->clearColour;
    sceGuClearColor(bg.c);
#endif
    sceGuClearDepth(0);
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

    m_inFrame = true;
}

void pguContext::EndFrame()
{
    // PSP: debug rectangles removed — we want clean screen for sprites.
    sceGuFinish();
    sceGuSync(0, 0);

    m_inFrame = false;
    m_frame_count++;

    pddiBaseContext::EndFrame();
}


void pguContext::Clear(unsigned bufferMask)
{
    unsigned gu_mask = 0;
    if (bufferMask & PDDI_BUFFER_COLOUR)  gu_mask |= GU_COLOR_BUFFER_BIT;
    if (bufferMask & PDDI_BUFFER_DEPTH)   gu_mask |= GU_DEPTH_BUFFER_BIT;
    if (bufferMask & PDDI_BUFFER_STENCIL) gu_mask |= GU_STENCIL_BUFFER_BIT;

    if (gu_mask) sceGuClear(gu_mask);
}

//=============================================================================
// Абстрактные методы pddiBaseContext
//=============================================================================
void pguContext::BeginTiming() { }
float pguContext::EndTiming()  { return 0.0f; }

void pguContext::LoadHardwareMatrix(pddiMatrixType id)
{
    // Пока заглушка — GU matrices реализуем в этапе 2
    (void)id;
}

void pguContext::SetupHardwareProjection() { }
void pguContext::SetupHardwareLight(int)   { }

int pguContext::GetMaxTextureDimension()
{
    // PSP: аппаратный максимум для текстур — 512x512
    return 512;
}

//=============================================================================
// Immediate mode rendering
//=============================================================================
pddiPrimStream* pguContext::BeginPrims(pddiShader* /*material*/, pddiPrimType primType, unsigned vertexType, int vertexCount, unsigned /*pass*/)
{
    return new pguPrimStream(primType, vertexType, vertexCount);
}

void pguContext::EndPrims(pddiPrimStream* stream)
{
    if (!stream) return;
    pguPrimStream* gs = static_cast<pguPrimStream*>(stream);
    gs->Flush();
    delete gs;
}
