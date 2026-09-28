//=============================================================================
// guprim.cpp — реализация pguPrimStream через GU
//=============================================================================
#include <pddi/pddi.hpp>


// RenderTr helper
#ifdef RAD_PSP
#include <pspiofilemgr.h>
static void RenderTr(const char* tag, int primType, int count, float x0, float y0, float z0, unsigned int col0) {
    static int cnt = 0; if (cnt > 400) return; cnt++;
    SceUID fd = sceIoOpen("ms0:/hitr_render.log", PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0777);
    if (fd < 0) return;
    char b[200]; int i = 0;
    const char* m = tag; while (*m) b[i++] = *m++;
    b[i++] = ' '; b[i++] = 'p'; b[i++] = '='; b[i++] = '0' + (primType % 10);
    b[i++] = ' '; b[i++] = 'n'; b[i++] = '='; 
    int v = count; char t[8]; int k = 0; while (v > 0) { t[k++] = '0' + (v % 10); v /= 10; }
    while (k > 0) b[i++] = t[--k];
    b[i++] = ' '; b[i++] = '(';
    int vv = (int)x0; char s1[8]; int k1 = 0; if (vv<0){b[i++]='-';vv=-vv;} if(vv==0){s1[k1++]='0';} else {while(vv>0){s1[k1++]='0'+(vv%10);vv/=10;}} while(k1>0)b[i++]=s1[--k1];
    b[i++] = ','; vv = (int)y0; char s2[8]; int k2 = 0; if(vv<0){b[i++]='-';vv=-vv;} if(vv==0){s2[k2++]='0';} else {while(vv>0){s2[k2++]='0'+(vv%10);vv/=10;}} while(k2>0)b[i++]=s2[--k2];
    b[i++] = ','; vv = (int)z0; char s3[8]; int k3 = 0; if(vv<0){b[i++]='-';vv=-vv;} if(vv==0){s3[k3++]='0';} else {while(vv>0){s3[k3++]='0'+(vv%10);vv/=10;}} while(k3>0)b[i++]=s3[--k3];
    b[i++] = ')'; b[i++] = ' '; b[i++] = 'c'; b[i++] = '=';
    unsigned int cv = col0; char hex[10]; int h = 0;
    const char* H = "0123456789ABCDEF";
    for (int z = 28; z >= 0; z -= 4) hex[h++] = H[(cv >> z) & 0xF];
    for (int z = 0; z < 8; ++z) b[i++] = hex[z];
    b[i++] = '\n';
    sceIoWrite(fd, b, i); sceIoClose(fd);
}
#else
#define RenderTr(a,b,c,d,e,f,g) ((void)0)
#endif
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
#ifdef RAD_PSP
    {
        SceUID fd = sceIoOpen("ms0:/hitr_flush.log", PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0777);
        if (fd >= 0) {
            char b[80]; int i=0;
            const char* m = "[F] flush n="; while(*m) b[i++]=*m++;
            int v=m_count; char t[8]; int k=0; if(v==0){t[k++]='0';} else {while(v>0){t[k++]='0'+(v%10);v/=10;}}
            while(k>0)b[i++]=t[--k];
            b[i++]=' '; b[i++]='p'; b[i++]='='; b[i++]='0'+(m_guPrimType%10);
            b[i++]='\n';
            sceIoWrite(fd, b, i); sceIoClose(fd);
        }
    }
#endif
    if (m_count < 2) return;
    RenderTr("[R] Flush", (int)m_guPrimType, m_count,
             g_sink.positions[0][0], g_sink.positions[0][1], g_sink.positions[0][2],
             g_sink.colours[0]);

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
