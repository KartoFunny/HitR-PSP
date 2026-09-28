//=============================================================================
// gudisplay.cpp — реализация pguDisplay для PSP
//=============================================================================
#include <pddi/pddi.hpp>
#include "gudisplay.hpp"
#include "gudev.hpp"

#include <pspgu.h>
#include <pspdisplay.h>
#include <cstring>

// Фрейм-буферы PSP: 2 x 512x272 x 4 байта = ~1 МБ
#define GU_BUF_WIDTH 512
#define GU_SCR_WIDTH 480
#define GU_SCR_HEIGHT 272

// Списки команд GU — 256 KB на каждый
static unsigned int __attribute__((aligned(16))) s_frame_list[65536];
static unsigned int __attribute__((aligned(16))) s_display_list[65536];

// VRAM-адреса буферов (в 16 МБ VRAM PSP)
static const unsigned int VRAM_DRAW_BUF0 = 0x00000000;
static const unsigned int VRAM_DRAW_BUF1 = 0x00088000;
static const unsigned int VRAM_DEPTH_BUF = 0x00110000;

pguDisplay::pguDisplay(pguDevice* dev)
    : m_dev(dev), m_width(GU_SCR_WIDTH), m_height(GU_SCR_HEIGHT), m_depth(32),
      m_mode(PDDI_DISPLAY_FULLSCREEN), m_refs(1), m_gu_initialized(false)
{
}

pguDisplay::~pguDisplay()
{
    if (m_gu_initialized)
    {
        sceGuTerm();
        m_gu_initialized = false;
    }
}

bool pguDisplay::InitDisplay(int x, int y, int bpp)
{
    m_width = x ? x : GU_SCR_WIDTH;
    m_height = y ? y : GU_SCR_HEIGHT;
    m_depth = bpp ? bpp : 32;

    // Инициализация GU — реальный PSP GPU
    sceGuInit();
    m_gu_initialized = true;

    // Настройка буферов
    sceGuStart(GU_DIRECT, s_frame_list);
    sceGuDrawBuffer(GU_PSM_8888, (void*)VRAM_DRAW_BUF0, GU_BUF_WIDTH);
    sceGuDispBuffer(GU_SCR_WIDTH, GU_SCR_HEIGHT, (void*)VRAM_DRAW_BUF1, GU_BUF_WIDTH);
    sceGuDepthBuffer((void*)VRAM_DEPTH_BUF, GU_BUF_WIDTH);

    sceGuOffset(2048 - (GU_SCR_WIDTH / 2), 2048 - (GU_SCR_HEIGHT / 2));
    sceGuViewport(2048, 2048, GU_SCR_WIDTH, GU_SCR_HEIGHT);
    sceGuDepthRange(0xC000, 0xFFFF);
    sceGuScissor(0, 0, GU_SCR_WIDTH, GU_SCR_HEIGHT);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuDepthFunc(GU_GEQUAL);
    sceGuFrontFace(GU_CW);
    sceGuShadeModel(GU_SMOOTH);
    sceGuDisable(GU_TEXTURE_2D);
    sceGuFinish();
    sceGuSync(0, 0);

    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);

    return true;
}

bool pguDisplay::InitDisplay(const pddiDisplayInit* initData)
{
    if (!initData) return InitDisplay(0, 0, 0);
    return InitDisplay(initData->xsize, initData->ysize, initData->bpp);
}

int pguDisplay::GetHeight() { return m_height; }
int pguDisplay::GetWidth() { return m_width; }
int pguDisplay::GetDepth() { return m_depth; }
pddiDisplayMode pguDisplay::GetDisplayMode() { return m_mode; }
int pguDisplay::GetNumColourBuffer() { return 2; }
unsigned pguDisplay::GetBufferMask() { return PDDI_BUFFER_COLOUR | PDDI_BUFFER_DEPTH; }
unsigned pguDisplay::GetFreeTextureMem() { return 4 * 1024 * 1024; }   // 4 МБ текстурной памяти

void pguDisplay::SwapBuffers()
{
    // Завершить кадр и отдать GPU
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuSwapBuffers();
}

unsigned pguDisplay::Screenshot(pddiColour*, int) { return 0; }

void pguDisplay::AddRef()  { m_refs++; }
void pguDisplay::Release() { if (--m_refs <= 0) delete this; }
