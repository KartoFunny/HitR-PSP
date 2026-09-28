//=============================================================================
// gudev.cpp — реализация pguDevice
//=============================================================================
#include <pddi/pddi.hpp>
#include "gudev.hpp"
#include "gudisplay.hpp"
#include "gucon.hpp"

#include <cstring>

pguDevice::pguDevice()
    : m_display(nullptr), m_context(nullptr), m_refs(1)
{
}

pguDevice::~pguDevice()
{
    if (m_context) { m_context->Release(); m_context = nullptr; }
    if (m_display) { m_display->Release(); m_display = nullptr; }
}

void pguDevice::GetLibraryInfo(pddiLibInfo* info)
{
    if (!info) return;
    info->versionMajor = PDDI_VERSION_MAJOR;
    info->versionMinor = PDDI_VERSION_MINOR;
    info->versionBuild = 0;
    info->libID = PDDI_LIBID_STUB;
    strncpy(info->description, "PSP GU", sizeof(info->description) - 1);
}

void pguDevice::SetCurrentContext(pddiRenderContext* c) { m_context = static_cast<pguContext*>(c); }
pddiRenderContext* pguDevice::GetCurrentContext() { return m_context; }

pddiDisplay* pguDevice::NewDisplay(int)
{
    if (!m_display) m_display = new pguDisplay(this);
    return m_display;
}

pddiRenderContext* pguDevice::NewRenderContext(pddiDisplay* display)
{
    if (!m_context) m_context = new pguContext(display, this);
    return m_context;
}

pddiTexture*     pguDevice::NewTexture(pddiTextureDesc*) { return nullptr; }
pddiPrimBuffer*  pguDevice::NewPrimBuffer(pddiPrimBufferDesc*) { return nullptr; }
pddiShader*      pguDevice::NewShader(const char*, const char*) { return nullptr; }
void             pguDevice::AddCustomShader(const char*, const char*) { }
void             pguDevice::SetMessageCallback(MessageCallback*) { }

void pguDevice::AddRef()  { m_refs++; }
void pguDevice::Release() { if (--m_refs <= 0) delete this; }
