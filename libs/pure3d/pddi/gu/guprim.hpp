//=============================================================================
// guprim.hpp — pguPrimStream (immediate mode через GU)
//=============================================================================
#ifndef _PDDI_GU_PRIM_HPP_
#define _PDDI_GU_PRIM_HPP_

#include <pddi/pddi.hpp>
#include <pddi/pddipc.hpp>

const int PGU_MAX_VERTICES = 8192;

class pguPrimStream : public pddiPrimStream
{
public:
    pguPrimStream(pddiPrimType type, unsigned vertexFormat, int vertexCount);
    virtual ~pguPrimStream();

    // Атрибуты-накопители
    virtual void Coord(float x, float y, float z);
    virtual void Normal(float x, float y, float z);
    virtual void Colour(pddiColour c, int channel = 0);
    virtual void UV(float u, float v, int channel = 0);
    virtual void Specular(pddiColour c);

    // Фиксация вершины
    virtual void Vertex(pddiVector* v, pddiColour c);
    virtual void Vertex(pddiVector* v, pddiVector* n);
    virtual void Vertex(pddiVector* v, pddiVector2* uv);
    virtual void Vertex(pddiVector* v, pddiColour c, pddiVector2* uv);
    virtual void Vertex(pddiVector* v, pddiVector* n, pddiVector2* uv);

    // Вызывается из pguContext::EndPrims
    void Flush();

private:
    void PushVertex();

    pddiPrimType m_type;
    unsigned     m_format;
    int          m_max;
    int          m_count;
    unsigned     m_guPrimType;

    float m_px, m_py, m_pz;
    float m_nx, m_ny, m_nz;
    float m_u,  m_v;
    pddiColour m_col;
};

#endif
