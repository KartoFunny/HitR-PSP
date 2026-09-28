#!/bin/bash
set -e
cd ~/psp/hitr-psp

python3 - <<'PY'
p="libs/pure3d/p3d/psppng.cpp"
s=open(p).read()

# Заменяем функцию psp_png_load целиком — читаем одним куском и пересканируем только хвост
import re
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
    // PSP: read PNG in large chunks, scan for IEND only in the newly-read
    // region (avoids O(n^2) full-buffer rescans). UI PNGs are typically < 8KB.
    static const unsigned MAX_READ = 65536;

    unsigned char* buf = (unsigned char*)malloc(MAX_READ);
    if (!buf) return;

    // Peek first 8 bytes to validate PNG signature.
    file->GetData(buf, 8, tFile::BYTE);
    if (buf[0] != 0x89 || buf[1] != 'P' || buf[2] != 'N' || buf[3] != 'G') {
        // Not PNG — skip.
        free(buf);
        return;
    }

    unsigned total = 8;
    bool found_end = false;

    while (!found_end && total < MAX_READ)
    {
        unsigned chunk = MAX_READ - total;
        if (chunk > 4096) chunk = 4096;

        file->GetData(buf + total, chunk, tFile::BYTE);
        unsigned prev = total;
        total += chunk;

        // Scan only the new region (+7 bytes of overlap to catch split "IEND").
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

    if (!found_end || total < 16) {
        PngDbg("[PNG] no IEND", total, 0);
        free(buf);
        return;
    }

    int w = 0, h = 0;
    unsigned char* rgba = 0;
    if (psp_decode_png(buf, total, &w, &h, &rgba))
    {
        builder->BeginImage(w, h, 32, tImageHandler::Builder::TOP, (pddiColour*)0);
        for (int y = 0; y < h; y++) {
            builder->ProcessScanline32((unsigned*)(rgba + (size_t)y * w * 4));
        }
        builder->EndImage();
        free(rgba);
    }
    free(buf);
}'''

s = s[:start] + new_func + s[end:]

# Убираем trace из decode (только начало)
s = s.replace('    PngDbg("[PNG] load enter size", size, 0);\n', '')
s = s.replace('    PngDbg("[PNG] buf first 8", ((unsigned)buf[0]<<24)|((unsigned)buf[1]<<16)|((unsigned)buf[2]<<8)|buf[3],\n           ((unsigned)buf[4]<<24)|((unsigned)buf[5]<<16)|((unsigned)buf[6]<<8)|buf[7]);\n', '')

open(p,"w").write(s)
print("psp_png_load speedup done")
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
echo "DONE — run EBOOT, wait 60s"
