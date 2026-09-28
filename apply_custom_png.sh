#!/bin/bash
set -e
cd ~/psp/hitr-psp

# ============ 1. Создаём psppng.cpp ============
cat > libs/pure3d/p3d/psppng.cpp <<'EOF'
//=============================================================================
// psppng.cpp — minimal PNG decoder for PSP (no libpng).
// Uses zlib's uncompress() to inflate IDAT, then applies PNG filters.
// Output format matches what tPNGHandler::LoadPNG32 expects: BGRA (AARRGGBB).
//=============================================================================
#ifdef RAD_PSP

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <zlib.h>

#include <p3d/imagefactory.hpp>
#include <p3d/file.hpp>
#include <pddi/pddi.hpp>

//-----------------------------------------------------------------------------
static unsigned png_rd32be(const unsigned char* p)
{
    return ((unsigned)p[0] << 24) | ((unsigned)p[1] << 16) |
           ((unsigned)p[2] << 8)  |  (unsigned)p[3];
}

static inline int png_paeth(int a, int b, int c)
{
    int p = a + b - c;
    int pa = p > a ? p - a : a - p;
    int pb = p > b ? p - b : b - p;
    int pc = p > c ? p - c : c - p;
    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
}

//-----------------------------------------------------------------------------
// Decode PNG into freshly malloc'd RGBA buffer (BGRA byte order).
// Returns 1 on success, 0 on failure.
//-----------------------------------------------------------------------------
static int psp_decode_png(const unsigned char* data, unsigned size,
                          int* outW, int* outH, unsigned char** outRGBA)
{
    static const unsigned char sig[8] = { 0x89,'P','N','G','\r','\n',0x1a,'\n' };
    if (size < 8 || memcmp(data, sig, 8) != 0) return 0;

    unsigned pos = 8;
    unsigned width = 0, height = 0;
    int bitDepth = 0, colorType = 0, interlace = 0, compression = 0, filterMethod = 0;
    int gotIHDR = 0, gotIEND = 0;

    unsigned idatCap = 1024, idatSize = 0;
    unsigned char* idat = (unsigned char*)malloc(idatCap);
    unsigned char palette[256 * 3];
    int paletteSize = 0;
    unsigned char trans[256];
    int transSize = 0;

    while (pos + 12 <= size)
    {
        unsigned len = png_rd32be(data + pos);
        const char* type = (const char*)(data + pos + 4);
        const unsigned char* chunk = data + pos + 8;

        if (pos + 12 + len > size) break;

        if (memcmp(type, "IHDR", 4) == 0 && len == 13)
        {
            width         = png_rd32be(chunk);
            height        = png_rd32be(chunk + 4);
            bitDepth      = chunk[8];
            colorType     = chunk[9];
            compression   = chunk[10];
            filterMethod  = chunk[11];
            interlace     = chunk[12];
            gotIHDR = 1;
        }
        else if (memcmp(type, "PLTE", 4) == 0)
        {
            paletteSize = len / 3;
            if (paletteSize > 256) paletteSize = 256;
            memcpy(palette, chunk, paletteSize * 3);
        }
        else if (memcmp(type, "tRNS", 4) == 0)
        {
            transSize = len;
            if (transSize > 256) transSize = 256;
            memcpy(trans, chunk, transSize);
        }
        else if (memcmp(type, "IDAT", 4) == 0)
        {
            if (idatSize + len > idatCap) {
                idatCap = (idatSize + len) * 2;
                unsigned char* n = (unsigned char*)realloc(idat, idatCap);
                if (!n) { free(idat); return 0; }
                idat = n;
            }
            memcpy(idat + idatSize, chunk, len);
            idatSize += len;
        }
        else if (memcmp(type, "IEND", 4) == 0)
        {
            gotIEND = 1;
            break;
        }

        pos += 12 + len;
    }

    if (!gotIHDR || !gotIEND || idatSize == 0) { free(idat); return 0; }
    if (interlace != 0) { free(idat); return 0; }
    if (compression != 0 || filterMethod != 0) { free(idat); return 0; }

    int channels = 0;
    switch (colorType) {
        case 0: channels = 1; break;   // grayscale
        case 2: channels = 3; break;   // RGB
        case 3: channels = 1; break;   // palette index
        case 4: channels = 2; break;   // gray+alpha
        case 6: channels = 4; break;   // RGBA
        default: free(idat); return 0;
    }

    // Bytes per (unfiltered) scanline
    unsigned bytesPerRow;
    int filterBPP;  // "bpp" in PNG filter spec = bytes per pixel, min 1
    if (colorType == 3 && bitDepth == 4) {
        bytesPerRow = (width + 1) / 2;
        filterBPP = 1;
    } else if (colorType == 3 && bitDepth == 8) {
        bytesPerRow = width;
        filterBPP = 1;
    } else if (bitDepth == 8) {
        bytesPerRow = width * channels;
        filterBPP = channels;
    } else {
        // 16-bit or sub-byte grayscale — skip for now
        free(idat); return 0;
    }

    unsigned rawSize = height * (1 + bytesPerRow);
    unsigned char* raw = (unsigned char*)malloc(rawSize);
    if (!raw) { free(idat); return 0; }

    uLongf destLen = rawSize;
    int zr = uncompress(raw, &destLen, idat, idatSize);
    free(idat);
    if (zr != Z_OK || destLen != rawSize) { free(raw); return 0; }

    // Unfilter
    unsigned char* unfilt = (unsigned char*)malloc(height * bytesPerRow);
    if (!unfilt) { free(raw); return 0; }

    unsigned srcPos = 0;
    unsigned char* prevRow = 0;
    for (unsigned y = 0; y < height; y++)
    {
        unsigned char ftype = raw[srcPos++];
        unsigned char* src = raw + srcPos;
        unsigned char* dst = unfilt + y * bytesPerRow;

        for (unsigned x = 0; x < bytesPerRow; x++)
        {
            int a = (x >= (unsigned)filterBPP) ? dst[x - filterBPP] : 0;
            int b = prevRow ? prevRow[x] : 0;
            int c = (prevRow && x >= (unsigned)filterBPP) ? prevRow[x - filterBPP] : 0;
            int v = src[x];
            switch (ftype) {
                case 0: break;                                  // None
                case 1: v = (v + a) & 0xff; break;               // Sub
                case 2: v = (v + b) & 0xff; break;               // Up
                case 3: v = (v + ((a + b) >> 1)) & 0xff; break;  // Average
                case 4: v = (v + png_paeth(a, b, c)) & 0xff; break; // Paeth
                default: free(raw); free(unfilt); return 0;
            }
            dst[x] = (unsigned char)v;
        }
        srcPos += bytesPerRow;
        prevRow = dst;
    }
    free(raw);

    // Convert to BGRA (matches libpng png_set_bgr + png_set_filler layout)
    unsigned char* rgba = (unsigned char*)malloc((size_t)width * height * 4);
    if (!rgba) { free(unfilt); return 0; }

    for (unsigned y = 0; y < height; y++)
    {
        const unsigned char* src = unfilt + y * bytesPerRow;
        unsigned char* dst = rgba + y * width * 4;

        for (unsigned x = 0; x < width; x++)
        {
            unsigned char R = 0, G = 0, B = 0, A = 0xff;

            if (colorType == 0) {
                R = G = B = src[x];
            }
            else if (colorType == 2) {
                R = src[x*3+0]; G = src[x*3+1]; B = src[x*3+2];
            }
            else if (colorType == 3) {
                int idx;
                if (bitDepth == 8) idx = src[x];
                else idx = (x & 1) ? (src[x>>1] & 0x0f) : ((src[x>>1] >> 4) & 0x0f);
                if (idx >= paletteSize) idx = 0;
                R = palette[idx*3+0]; G = palette[idx*3+1]; B = palette[idx*3+2];
                if (idx < transSize) A = trans[idx];
            }
            else if (colorType == 4) {
                R = G = B = src[x*2+0]; A = src[x*2+1];
            }
            else if (colorType == 6) {
                R = src[x*4+0]; G = src[x*4+1]; B = src[x*4+2]; A = src[x*4+3];
            }

            // BGRA byte order in memory (little endian = uint32 AARRGGBB)
            dst[x*4+0] = B;
            dst[x*4+1] = G;
            dst[x*4+2] = R;
            dst[x*4+3] = A;
        }
    }
    free(unfilt);

    *outW = (int)width;
    *outH = (int)height;
    *outRGBA = rgba;
    return 1;
}

//-----------------------------------------------------------------------------
// Public entry: called from tPNGHandler::CreateImage under RAD_PSP.
//-----------------------------------------------------------------------------
void psp_png_load(tFile* file, tImageHandler::Builder* builder)
{
    unsigned size = (unsigned)file->GetSize();
    if (size == 0 || size > 16 * 1024 * 1024) return;  // sanity cap 16 MB

    unsigned char* buf = (unsigned char*)malloc(size);
    if (!buf) return;
    file->GetData(buf, size, tFile::BYTE);

    int w = 0, h = 0;
    unsigned char* rgba = 0;
    if (psp_decode_png(buf, size, &w, &h, &rgba))
    {
        builder->BeginImage(w, h, 32, tImageHandler::Builder::TOP, (pddiColour*)0);
        for (int y = 0; y < h; y++) {
            builder->ProcessScanline32((unsigned*)(rgba + (size_t)y * w * 4));
        }
        builder->EndImage();
        free(rgba);
    }
    free(buf);
}

#endif // RAD_PSP
EOF

echo "created psppng.cpp"

# ============ 2. Патч CreateImage ============
python3 - <<'PY'
p="libs/pure3d/p3d/png.cpp"
s=open(p).read()

old="""void tPNGHandler::CreateImage(tFile* file, tImageHandler::Builder* builder)
{
#ifdef RAD_PSP
    // PSP: libpng 1.0.3 fails to initialize on this platform.
    // Skip PNG decoding entirely; builder stays empty, caller gets NULL texture.
    (void)file; (void)builder;
    return;
#else"""

new="""#ifdef RAD_PSP
extern void psp_png_load(tFile* file, tImageHandler::Builder* builder);
#endif

void tPNGHandler::CreateImage(tFile* file, tImageHandler::Builder* builder)
{
#ifdef RAD_PSP
    // PSP: libpng 1.0.3 fails to initialize; use our minimal decoder.
    psp_png_load(file, builder);
    return;
#else"""

assert old in s, "CreateImage anchor not found"
s=s.replace(old,new,1)
print("CreateImage patched")

open(p,"w").write(s)
PY

# ============ 3. CMakeLists: добавить psppng.cpp ============
python3 - <<'PY'
p="libs/pure3d/CMakeLists.txt"
s=open(p).read()

if "psppng.cpp" not in s:
    # Найти строку с png.cpp
    if "p3d/png.cpp" in s:
        s = s.replace("p3d/png.cpp", "p3d/png.cpp\n    p3d/psppng.cpp", 1)
        print("added psppng.cpp to CMakeLists")
    else:
        # Fallback — искать любое упоминание png.cpp
        idx = s.find("png.cpp")
        if idx >= 0:
            end = s.find('\n', idx)
            s = s[:end+1] + "    p3d/psppng.cpp\n" + s[end+1:]
            print("added via fallback")
        else:
            print("WARN: png.cpp not found in CMakeLists")
open(p,"w").write(s)
PY

echo
echo "=== verify ==="
grep -n "psppng" libs/pure3d/CMakeLists.txt

# ============ 4. rebuild ============
echo "=== rebuild ==="
docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build pspdev/pspdev:latest \
  bash -c "cmake .. > /dev/null 2>&1; \
           rm -f libs/pure3d/CMakeFiles/p3d.dir/p3d/png.cpp.obj \
                 libs/pure3d/CMakeFiles/p3d.dir/p3d/psppng.cpp.obj && \
           make SRR2 -j2 2>&1 | tail -20"

docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build/code pspdev/pspdev:latest \
  bash -c "mksfoex -d MEMSIZE=2 'HitR' PARAM.SFO && \
           psp-strip SRR2 -o SRR2_strip.elf && \
           pack-pbp EBOOT.PBP PARAM.SFO NULL NULL NULL NULL NULL SRR2_strip.elf NULL"

ls -la build/code/EBOOT.PBP
rm -f ~/.config/ppsspp/hitr_*.log
echo
echo "DONE — run EBOOT, wait 25s"
