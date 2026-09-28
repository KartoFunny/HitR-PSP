//=============================================================================
// gudisplay.hpp — pguDisplay
//=============================================================================
#ifndef _PDDI_GU_DISPLAY_HPP_
#define _PDDI_GU_DISPLAY_HPP_

#include <pddi/pddi.hpp>

class pguDevice;

class pguDisplay : public pddiDisplay
{
public:
    pguDisplay(pguDevice* dev);
    virtual ~pguDisplay();

    bool InitDisplay(int x, int y, int bpp);
    bool InitDisplay(const pddiDisplayInit* initData);

    int GetHeight();
    int GetWidth();
    int GetDepth();
    pddiDisplayMode GetDisplayMode();
    int GetNumColourBuffer();
    unsigned GetBufferMask();
    unsigned GetFreeTextureMem();
    void SwapBuffers();
    unsigned Screenshot(pddiColour* buffer, int nBytes);

    void AddRef();
    void Release();

private:
    pguDevice* m_dev;
    int m_width, m_height, m_depth;
    pddiDisplayMode m_mode;
    int m_refs;
    bool m_gu_initialized;
};

#endif
