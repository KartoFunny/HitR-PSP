//=============================================================================
// gucon.hpp — pguContext
//=============================================================================
#ifndef _PDDI_GU_CON_HPP_
#define _PDDI_GU_CON_HPP_

#include <pddi/pddi.hpp>
#include <pddi/base/basecontext.hpp>

class pguDevice;
class pguDisplay;

class pguContext : public pddiBaseContext
{
public:
    pguContext(pddiDisplay* disp, pddiDevice* dev);
    virtual ~pguContext();

    // Frame synchronisation — переопределяем для GU
    virtual void BeginFrame();
    virtual void EndFrame();

    // Clear — реальный вызов sceGuClear
    virtual void Clear(unsigned bufferMask);

    // 5 абстрактных методов из pddiBaseContext
    virtual void  BeginTiming();
    virtual float EndTiming();
    virtual void  LoadHardwareMatrix(pddiMatrixType id);
    virtual void  SetupHardwareProjection();
    virtual void  SetupHardwareLight(int handle);

    // Из pddiRenderContext
    virtual int   GetMaxTextureDimension();

    // Immediate mode rendering
    virtual pddiPrimStream* BeginPrims(pddiShader* material, pddiPrimType primType, unsigned vertexType, int vertexCount, unsigned pass = 0);
    virtual void EndPrims(pddiPrimStream* stream);

    // Ref counting
    virtual void AddRef();
    virtual void Release();

    static unsigned int* GetList();

private:
    int m_refs;
    bool m_inFrame;
    unsigned int m_frame_count;
};

#endif
