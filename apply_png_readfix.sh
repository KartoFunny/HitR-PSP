#!/bin/bash
set -e
cd ~/psp/hitr-psp

python3 - <<'PY'
p="libs/pure3d/p3d/psppng.cpp"
s=open(p).read()

# Заменяем функцию psp_png_load целиком
import re
start = s.find("void psp_png_load(tFile* file, tImageHandler::Builder* builder)")
assert start >= 0, "psp_png_load not found"
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
    // PSP: instead of reading the whole tFile (which may be 4.8 MB for a
    // P3D container), read in small chunks and stop at the PNG "IEND" chunk.
    // Each UI PNG is typically < 8 KB.
    static const unsigned MAX_READ = 65536;  // hard cap: 64 KB per PNG
    unsigned char* buf = (unsigned char*)malloc(MAX_READ);
    if (!buf) return;

    unsigned total = 0;
    bool found_end = false;

    while (!found_end && total < MAX_READ)
    {
        unsigned chunk = MAX_READ - total;
        if (chunk > 2048) chunk = 2048;

        // tFile::GetData reads into buffer; assume it fills 'chunk' bytes
        // or stops at EOF.
        file->GetData(buf + total, chunk, tFile::BYTE);
        total += chunk;

        // Look for IEND + 4-byte CRC — that's the natural end of a PNG.
        // Need at least 8 bytes from each candidate position.
        for (unsigned i = 0; i + 8 <= total; ++i)
        {
            if (buf[i]   == 'I' && buf[i+1] == 'E' &&
                buf[i+2] == 'N' && buf[i+3] == 'D')
            {
                total = i + 8;   // include "IEND" + 4-byte CRC
                found_end = true;
                break;
            }
        }
    }

    PngDbg("[PNG] read total", total, found_end ? 1u : 0u);

    if (total < 16) { free(buf); return; }

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
open(p,"w").write(s)
print("psp_png_load rewritten with IEND scan")
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
echo "DONE — run EBOOT, wait 30s"
