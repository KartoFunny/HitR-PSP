//=============================================================================
// gutexture.hpp — stub pddiTexture for PSP GU backend
//=============================================================================
#ifndef GUTEXTURE_HPP
#define GUTEXTURE_HPP

#include <pddi/pddi.hpp>
#include <stdlib.h>
#include <string.h>

class pguTexture : public pddiTexture
{
public:
    pguTexture(pddiTextureDesc* desc)
        : m_refs(1), m_bits(0), m_size(0)
    {
        memset(&m_desc, 0, sizeof(m_desc));
        m_desc.width     = desc ? (int)desc->GetSizeX() : 0;
        m_desc.height    = desc ? (int)desc->GetSizeY() : 0;
        m_desc.depth     = desc ? (int)desc->GetSizeZ() : 1;
        m_desc.volDepth  = 1;
        m_desc.format    = PDDI_PIXEL_UNKNOWN;
        m_desc.native    = false;
        m_desc.pitch     = m_desc.width * 4;
        m_desc.slice     = m_desc.pitch * m_desc.height;

        // Allocate a fake pixel buffer so Lock() returns valid memory.
        m_size = (unsigned)(m_desc.slice ? m_desc.slice : (m_desc.width * m_desc.height * 4));
        if (m_size == 0) m_size = 4;
        m_bits = calloc(1, m_size);
        m_desc.bits = m_bits;
    }

    virtual ~pguTexture() { if (m_bits) free(m_bits); }

    // pddiObject
    const char* GetType() { return "pguTexture"; }
    void AddRef()  { ++m_refs; }
    void Release() { if (--m_refs <= 0) delete this; }

    // pddiTexture
    pddiPixelFormat GetPixelFormat() { return m_desc.format; }
    int GetWidth()        { return m_desc.width; }
    int GetHeight()       { return m_desc.height; }
    int GetDepth()        { return m_desc.depth; }
    int GetAlphaDepth()   { return 8; }
    int GetNumMipMaps()   { return 1; }

    int  GetNumPaletteEntries() { return 0; }
    void SetPalette(int, pddiColour*) {}
    int  GetPalette(pddiColour*) { return 0; }

    pddiLockInfo* Lock(int /*mipLevel*/, pddiRect* /*rect*/ = 0) { return &m_desc; }
    void Unlock(int /*mipLevel*/) {}
    void SetCompressedData(int, const char* const, int) {}

    void Prefetch() {}
    void Discard() {}
    void SetPriority(int) {}
    int  GetPriority() { return 0; }

private:
    int            m_refs;
    pddiLockInfo   m_desc;
    void*          m_bits;
    unsigned       m_size;
};

#endif // GUTEXTURE_HPP
