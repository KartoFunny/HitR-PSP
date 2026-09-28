//=============================================================================
// gudev.hpp — pguDevice
//=============================================================================
#ifndef _PDDI_GU_DEV_HPP_
#define _PDDI_GU_DEV_HPP_

#include <pddi/pddi.hpp>

class pguDisplay;
class pguContext;

class pguDevice : public pddiDevice
{
public:
    pguDevice();
    virtual ~pguDevice();

    void GetLibraryInfo(pddiLibInfo* info);
    void SetCurrentContext(pddiRenderContext* context);
    pddiRenderContext* GetCurrentContext();

    pddiDisplay* NewDisplay(int id);
    pddiRenderContext* NewRenderContext(pddiDisplay* display);
    pddiTexture* NewTexture(pddiTextureDesc* desc);
    pddiPrimBuffer* NewPrimBuffer(pddiPrimBufferDesc* desc);
    pddiShader* NewShader(const char* name, const char* aux = 0);
    void AddCustomShader(const char* name, const char* aux);
    void SetMessageCallback(MessageCallback* c);

    void AddRef();
    void Release();

private:
    pguDisplay* m_display;
    pguContext* m_context;
    int m_refs;
};

#endif
