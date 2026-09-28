//=============================================================================
// gushader.hpp — no-op pddiShader for PSP GU backend
//=============================================================================
#ifndef GUSHADER_HPP
#define GUSHADER_HPP

#include <pddi/pddi.hpp>

class pguShader : public pddiShader
{
public:
    pguShader() : m_refs(1) {}
    virtual ~pguShader() {}

    const char* GetType() { return "pguShader"; }

    bool SetTexture(unsigned /*param*/, pddiTexture* /*tex*/) { return true; }
    bool SetInt    (unsigned /*param*/, int           /*v*/)   { return true; }
    bool SetFloat  (unsigned /*param*/, float         /*v*/)   { return true; }
    bool SetColour (unsigned /*param*/, pddiColour    /*c*/)   { return true; }
    bool SetVector (unsigned /*param*/, const rmt::Vector& /*v*/) { return true; }
    bool SetMatrix (unsigned /*param*/, const rmt::Matrix& /*m*/) { return true; }

    void AddRef()  { ++m_refs; }
    void Release() { if (--m_refs <= 0) delete this; }

private:
    int m_refs;
};

#endif // GUSHADER_HPP
