//=============================================================================
// guprim.cpp — реализация pguPrimStream через GU
//=============================================================================
#include <pddi/pddi.hpp>
#include <pddi/pddipc.hpp>
#include "guprim.hpp"

#include <pspgu.h>
#include <pspgum.h>
#include <cstring>

// Накопитель вершин
struct PguVertexSink
{
    unsigned int  colours[PGU_MAX_VERTICES];
    float         positions[PGU_MAX_VERTICES][3];
    int           count;
};
static PguVertexSink g_sink;

pguPrimStream::pguPrimStream(pddiPrimType type, unsigned vertexFormat, int /*vertexCount*/)
    : m_type(type), m_format(vertexFormat), m_max(PGU_MAX_VERTICES), m_count(0),
      m_px(0), m_py(0), m_pz(0),
      m_nx(0), m_ny(0), m_nz(1),
      m_u(0), m_v(0),
      m_col(255, 255, 255, 255)
{
    switch (type)
    {
        case PDDI_PRIM_TRIANGLES:  m_guPrimType = GU_TRIANGLES;       break;
        case PDDI_PRIM_TRISTRIP:   m_guPrimType = GU_TRIANGLE_STRIP;  break;
        case PDDI_PRIM_LINES:      m_guPrimType = GU_LINES;           break;
        case PDDI_PRIM_LINESTRIP:  m_guPrimType = GU_LINE_STRIP;      break;
        case PDDI_PRIM_POINTS:     m_guPrimType = GU_POINTS;          break;
        default:                   m_guPrimType = GU_TRIANGLES;       break;
    }
    g_sink.count = 0;
}

pguPrimStream::~pguPrimStream() {}

void pguPrimStream::Coord(float x, float y, float z) { m_px = x; m_py = y; m_pz = z; }
void pguPrimStream::Normal(float x, float y, float z) { m_nx = x; m_ny = y; m_nz = z; }
void pguPrimStream::Colour(pddiColour c, int)         { m_col = c; }
void pguPrimStream::UV(float u, float v, int)         { m_u = u; m_v = v; }
void pguPrimStream::Specular(pddiColour)              { /* TODO */ }

void pguPrimStream::PushVertex()
{
    if (m_count >= m_max) return;
    g_sink.colours[m_count] = m_col.c;
    g_sink.positions[m_count][0] = m_px;
    g_sink.positions[m_count][1] = m_py;
    g_sink.positions[m_count][2] = m_pz;
    m_count++;
    g_sink.count = m_count;
}

void pguPrimStream::Vertex(pddiVector* v, pddiColour c)
{
    m_px = v->x; m_py = v->y; m_pz = v->z;
    m_col = c;
    PushVertex();
}
void pguPrimStream::Vertex(pddiVector* v, pddiVector* n)
{
    m_px = v->x; m_py = v->y; m_pz = v->z;
    m_nx = n->x; m_ny = n->y; m_nz = n->z;
    PushVertex();
}
void pguPrimStream::Vertex(pddiVector* v, pddiVector2* uv)
{
    m_px = v->x; m_py = v->y; m_pz = v->z;
    m_u = uv->u; m_v = uv->v;
    PushVertex();
}
void pguPrimStream::Vertex(pddiVector* v, pddiColour c, pddiVector2* uv)
{
    m_px = v->x; m_py = v->y; m_pz = v->z;
    m_col = c;
    m_u = uv->u; m_v = uv->v;
    PushVertex();
}
void pguPrimStream::Vertex(pddiVector* v, pddiVector* n, pddiVector2* uv)
{
    m_px = v->x; m_py = v->y; m_pz = v->z;
    m_nx = n->x; m_ny = n->y; m_nz = n->z;
    m_u = uv->u; m_v = uv->v;
    PushVertex();
}

void pguPrimStream::Flush()
{
    if (m_count < 2) return;

    struct V { float x, y, z; unsigned int c; };
    V* gu_v = (V*)sceGuGetMemory(m_count * sizeof(V));
    if (!gu_v) return;

    for (int i = 0; i < m_count; ++i)
    {
        gu_v[i].x = g_sink.positions[i][0];
        gu_v[i].y = g_sink.positions[i][1];
        gu_v[i].z = g_sink.positions[i][2];
        gu_v[i].c = g_sink.colours[i];
    }

    sceGuDisable(GU_TEXTURE_2D);
    sceGuDisable(GU_LIGHTING);
    sceGuDisable(GU_FOG);

    sceGuDrawArray(m_guPrimType,
                   GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                   m_count, 0, gu_v);
}
