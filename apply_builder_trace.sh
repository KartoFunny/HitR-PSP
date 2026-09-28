#!/bin/bash
set -e
cd ~/psp/hitr-psp

python3 - <<'PY'
p="libs/pure3d/p3d/psppng.cpp"
s=open(p).read()

old="""    if (psp_decode_png(buf, size, &w, &h, &rgba))
    {
        PngDbg("[PNG] decode OK", (unsigned)w, (unsigned)h);
        builder->BeginImage(w, h, 32, tImageHandler::Builder::TOP, (pddiColour*)0);
        for (int y = 0; y < h; y++) {
            builder->ProcessScanline32((unsigned*)(rgba + (size_t)y * w * 4));
        }
        builder->EndImage();
        free(rgba);
    }"""

new="""    if (psp_decode_png(buf, size, &w, &h, &rgba))
    {
        PngDbg("[PNG] decode OK", (unsigned)w, (unsigned)h);
        PngDbg("[PNG] before BeginImage", (unsigned)w, (unsigned)h);
        builder->BeginImage(w, h, 32, tImageHandler::Builder::TOP, (pddiColour*)0);
        PngDbg("[PNG] after BeginImage", 0, 0);
        for (int y = 0; y < h; y++) {
            PngDbg("[PNG] scanline y", (unsigned)y, (unsigned)h);
            builder->ProcessScanline32((unsigned*)(rgba + (size_t)y * w * 4));
        }
        PngDbg("[PNG] before EndImage", 0, 0);
        builder->EndImage();
        PngDbg("[PNG] after EndImage", 0, 0);
        free(rgba);
    }"""

assert old in s, "builder anchor not found"
s=s.replace(old,new,1)
open(p,"w").write(s)
print("builder traced")
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
