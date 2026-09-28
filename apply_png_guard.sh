#!/bin/bash
set -e
cd ~/psp/hitr-psp

python3 - <<'PY'
p="libs/pure3d/p3d/psppng.cpp"
s=open(p).read()

# 1. Trace helper
if "PngDbg" not in s:
    idx=s.find('#include <zlib.h>')
    end=s.find('\n',idx)
    helper='''
#include <pspiofilemgr.h>
static void PngDbg(const char* tag, unsigned v1, unsigned v2)
{
    SceUID fd = sceIoOpen("ms0:/hitr_png.log", PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0777);
    if (fd < 0) return;
    char b[128]; int i = 0;
    while (*tag) b[i++] = *tag++;
    b[i++] = ' ';
    {
        char t[12]; int k = 0; unsigned v = v1;
        if (v == 0) t[k++] = '0'; else { while (v) { t[k++] = '0' + v % 10; v /= 10; } }
        while (k > 0) b[i++] = t[--k];
    }
    b[i++] = ',';
    {
        char t[12]; int k = 0; unsigned v = v2;
        if (v == 0) t[k++] = '0'; else { while (v) { t[k++] = '0' + v % 10; v /= 10; } }
        while (k > 0) b[i++] = t[--k];
    }
    b[i++] = '\\n';
    sceIoWrite(fd, b, i); sceIoClose(fd);
}
'''
    s = s[:end+1] + helper + s[end+1:]
    print("PngDbg added")

# 2. Trace + sanity после IHDR
old="""    if (!gotIHDR || !gotIEND || idatSize == 0) { free(idat); return 0; }
    if (interlace != 0) { free(idat); return 0; }
    if (compression != 0 || filterMethod != 0) { free(idat); return 0; }"""
new="""    PngDbg("[PNG] parsed", (unsigned)width, (unsigned)height);
    PngDbg("[PNG] bitDepth/color", (unsigned)bitDepth, (unsigned)colorType);
    PngDbg("[PNG] idatSize", idatSize, 0);

    if (!gotIHDR || !gotIEND || idatSize == 0) { free(idat); return 0; }
    if (interlace != 0) { free(idat); return 0; }
    if (compression != 0 || filterMethod != 0) { free(idat); return 0; }
    // sanity: dimensions must be 1..4096
    if (width == 0 || height == 0 || width > 4096 || height > 4096) {
        PngDbg("[PNG] BAD dims", (unsigned)width, (unsigned)height);
        free(idat); return 0;
    }"""
assert old in s, "sanity anchor not found"
s=s.replace(old,new,1)
print("sanity added")

# 3. Trace перед uncompress
old="""    uLongf destLen = rawSize;
    int zr = uncompress(raw, &destLen, idat, idatSize);
    free(idat);
    if (zr != Z_OK || destLen != rawSize) { free(raw); return 0; }"""
new="""    PngDbg("[PNG] before uncompress rawSize", rawSize, idatSize);

    uLongf destLen = rawSize;
    int zr = uncompress(raw, &destLen, idat, idatSize);
    PngDbg("[PNG] uncompress ret", (unsigned)zr, (unsigned)destLen);
    free(idat);
    if (zr != Z_OK || destLen != rawSize) { free(raw); return 0; }"""
assert old in s, "uncompress anchor not found"
s=s.replace(old,new,1)
print("uncompress traced")

# 4. Trace перед фильтрами и после
old="""    unsigned srcPos = 0;
    unsigned char* prevRow = 0;
    for (unsigned y = 0; y < height; y++)"""
new="""    PngDbg("[PNG] before filters", bytesPerRow, height);

    unsigned srcPos = 0;
    unsigned char* prevRow = 0;
    for (unsigned y = 0; y < height; y++)"""
assert old in s
s=s.replace(old,new,1)

old="""    free(raw);

    // Convert to BGRA"""
new="""    free(raw);
    PngDbg("[PNG] filters done", width, height);

    // Convert to BGRA"""
assert old in s
s=s.replace(old,new,1)
print("filters traced")

# 5. Trace в psp_png_load
old="""    unsigned char* buf = (unsigned char*)malloc(size);
    if (!buf) return;
    file->GetData(buf, size, tFile::BYTE);

    int w = 0, h = 0;
    unsigned char* rgba = 0;
    if (psp_decode_png(buf, size, &w, &h, &rgba))
    {"""
new="""    PngDbg("[PNG] load enter size", size, 0);

    unsigned char* buf = (unsigned char*)malloc(size);
    if (!buf) return;
    file->GetData(buf, size, tFile::BYTE);

    PngDbg("[PNG] buf first 8", ((unsigned)buf[0]<<24)|((unsigned)buf[1]<<16)|((unsigned)buf[2]<<8)|buf[3],
           ((unsigned)buf[4]<<24)|((unsigned)buf[5]<<16)|((unsigned)buf[6]<<8)|buf[7]);

    int w = 0, h = 0;
    unsigned char* rgba = 0;
    if (psp_decode_png(buf, size, &w, &h, &rgba))
    {
        PngDbg("[PNG] decode OK", (unsigned)w, (unsigned)h);"""
assert old in s, "load enter anchor not found"
s=s.replace(old,new,1)
print("load traced")

open(p,"w").write(s)
PY

echo "=== rebuild ==="
docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build pspdev/pspdev:latest \
  bash -c "rm -f libs/pure3d/CMakeFiles/p3d.dir/p3d/psppng.cpp.obj && \
           make SRR2 -j2 2>&1 | tail -8"

docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build/code pspdev/pspdev:latest \
  bash -c "mksfoex -d MEMSIZE=2 'HitR' PARAM.SFO && \
           psp-strip SRR2 -o SRR2_strip.elf && \
           pack-pbp EBOOT.PBP PARAM.SFO NULL NULL NULL NULL NULL SRR2_strip.elf NULL"

ls -la build/code/EBOOT.PBP
rm -f ~/.config/ppsspp/hitr_*.log
echo "DONE — run EBOOT, wait 15s, kill"
