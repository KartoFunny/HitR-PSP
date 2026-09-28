//=============================================================================
// guprimbuf.hpp — no-op primBuffer + stream for PSP GU backend
//=============================================================================
#ifndef GUPRIMBUF_HPP
#define GUPRIMBUF_HPP

#include <pddi/pddi.hpp>

//----- pguPrimBufferStream ---------------------------------------------------
class pguPrimBufferStream : public pddiPrimBufferStream
{
public:
    pguPrimBufferStream() {}
    virtual ~pguPrimBufferStream() {}

    void Position(float, float, float) {}
    void Normal  (float, float, float) {}
    void Binormal(float, float, float) {}
    void Tangent (float, float, float) {}
    void Colour  (pddiColour, int /*channel*/ = 0) {}
    void TexCoord1(float, int /*channel*/ = 0) {}
    void TexCoord2(float, float, int /*channel*/ = 0) {}
    void TexCoord3(float, float, float, int /*channel*/ = 0) {}
    void TexCoord4(float, float, float, float, int /*channel*/ = 0) {}
    void Specular(pddiColour) {}
    void SkinIndices(unsigned, unsigned = 0, unsigned = 0, unsigned = 0) {}
    void SkinWeights(float, float = 0.0f, float = 0.0f) {}
    void Vertex(rmt::Vector*, pddiColour) {}
    void Vertex(rmt::Vector*, rmt::Vector*) {}
    void Vertex(rmt::Vector*, rmt::Vector2*) {}
    void Vertex(rmt::Vector*, pddiColour, rmt::Vector2*) {}
    void Vertex(rmt::Vector*, rmt::Vector*, rmt::Vector2*) {}
    void Next() {}
};

//----- pguPrimBuffer ---------------------------------------------------------
class pguPrimBuffer : public pddiPrimBuffer
{
public:
    pguPrimBuffer(pddiPrimBufferDesc* desc)
        : m_refs(1), m_stream(0), m_desc(*desc) {}
    virtual ~pguPrimBuffer() { delete m_stream; }

    // pddiObject
    const char* GetType() { return "pguPrimBuffer"; }
    void AddRef()  { ++m_refs; }
    void Release() { if (--m_refs <= 0) delete this; }

    // pddiPrimBuffer
    pddiPrimBufferStream* Lock() {
        if (!m_stream) m_stream = new pguPrimBufferStream();
        return m_stream;
    }
    void Unlock(pddiPrimBufferStream*) {}
    void Finalize() {}

    unsigned char* LockIndexBuffer() { return 0; }
    void UnlockIndexBuffer(int /*count*/) {}
    void SetIndices(unsigned short* /*indices*/, int /*count*/) {}

    bool CheckMemImageVersion(int /*version*/) { return true; }
    void* LockMemImage(unsigned /*len*/) { return 0; }
    void UnlockMemImage() {}
    void SetMemImageParam(unsigned /*param*/, unsigned /*value*/) {}
    void SetUsedSize(int /*size*/) {}

    const pddiPrimBufferDesc& GetDesc() const { return m_desc; }

private:
    int m_refs;
    pguPrimBufferStream* m_stream;
    pddiPrimBufferDesc   m_desc;
};

#endif // GUPRIMBUF_HPP
