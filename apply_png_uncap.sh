#!/bin/bash
set -e
cd ~/psp/hitr-psp

# 1. Убираем fileftt trace (не сработал)
python3 - <<'PY'
p="libs/pure3d/p3d/fileftt.cpp"
s=open(p).read()
s = s.replace("FTTr(\"[FTT] loop\", GetFilename()); p3d::loadManager->SwitchTask();",
              "p3d::loadManager->SwitchTask();")
open(p,"w").write(s)
print("fileftt cleaned")
PY

# 2. Переписываем psp_png_load с динамическим буфером
python3 - <<'PY'
p="libs/pure3d/p3d/psppng.cpp"
s=open(p).read()

start = s.find("void psp_png_load(tFile* file, tImageHandler::Builder* builder)")
assert start >= 0
brace = s.find('{', start)
depth = 1
pos = brace + 1
while depth > 0 and pos < len(s):
    if s[pos] == '{': depth += 1
    elif s[pos] == '}': depth -= 1
    pos += 1
end = pos

new_func = r'''void psp_png_load(tFile* file, tImageHandler::Builder* builder)
{
    // Dynamic buffer — grows as we read. Handles PNG up to 8 MB.
    static const unsigned HARD_CAP = 8 * 1024 * 1024;
    static const unsigned START_SIZE = 4096;

    unsigned cap = START_SIZE;
    unsigned char* buf = (unsigned char*)malloc(cap);
    if (!buf) return;

    unsigned total = 0;
    bool found_end = false;

    // Progress trace every 20th PNG
    static int s_png_count = 0;
    int myId = ++s_png_count;
    bool trace = (myId <= 5) || (myId % 20 == 0);

    while (!found_end && total < HARD_CAP)
    {
        unsigned chunk = 4096;
        if (total + chunk > cap) {
            unsigned newCap = cap * 2;
            if (newCap > HARD_CAP) newCap = HARD_CAP;
            if (newCap <= cap) break;
            unsigned char* nb = (unsigned char*)realloc(buf, newCap);
            if (!nb) break;
            buf = nb;
            cap = newCap;
        }

        file->GetData(buf + total, chunk, tFile::BYTE);
        unsigned prev = total;
        total += chunk;

        unsigned scanStart = (prev >= 7) ? (prev - 7) : 0;
        for (unsigned i = scanStart; i + 8 <= total; ++i)
        {
            if (buf[i]   == 'I' && buf[i+1] == 'E' &&
                buf[i+2] == 'N' && buf[i+3] == 'D')
            {
                total = i + 8;
                found_end = true;
                break;
            }
        }
    }

    if (trace) PngDbg("[PNG] count", (unsigned)myId, total);
    if (trace) PngDbg("[PNG] end", found_end ? 1u : 0u, cap);

    if (!found_end || total < 16) {
        free(buf);
        return;
    }

    // Remember size of PNG for later debugging
    if (trace) PngDbg("[PNG] size", total, 0);

    int w = 0, h = 0;
    unsigned char* rgba = 0;
    if (psp_decode_png(buf, total, &w, &h, &rgba))
    {
        if (trace) PngDbg("[PNG] decoded", (unsigned)w, (unsigned)h);
        builder->BeginImage(w, h, 32, tImageHandler::Builder::TOP, (pddiColour*)0);
        for (int y = 0; y < h; y++) {
            builder->ProcessScanline32((unsigned*)(rgba + (size_t)y * w * 4));
        }
        builder->EndImage();
        free(rgba);
    } else {
        if (trace) PngDbg("[PNG] decode failed", total, 0);
    }
    free(buf);
}'''
s = s[:start] + new_func + s[end:]
open(p,"w").write(s)
print("psp_png_load rewritten with dynamic buffer")
PY

echo "=== rebuild ==="
docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build pspdev/pspdev:latest \
  bash -c "rm -f libs/pure3d/CMakeFiles/p3d.dir/p3d/psppng.cpp.obj \
                    libs/pure3d/CMakeFiles/p3d.dir/p3d/fileftt.cpp.obj && \
           make SRR2 -j2 2>&1 | tail -10"

docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build/code pspdev/pspdev:latest \
  bash -c "mksfoex -d MEMSIZE=2 'HitR' PARAM.SFO && \
           psp-strip SRR2 -o SRR2_strip.elf && \
           pack-pbp EBOOT.PBP PARAM.SFO NULL NULL NULL NULL NULL SRR2_strip.elf NULL"

ls -la build/code/EBOOT.PBP
rm -f ~/.config/ppsspp/hitr_*.log
echo "DONE — run EBOOT, wait 90s (подольше, у нас теперь декодируется больших PNG)"
