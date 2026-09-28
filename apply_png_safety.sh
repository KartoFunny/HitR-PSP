#!/bin/bash
set -e
cd ~/psp/hitr-psp

python3 - <<'PY'
p="libs/pure3d/p3d/psppng.cpp"
s=open(p).read()

# 1. Уменьшить MAX_READ до 16 КБ
s = s.replace("static const unsigned MAX_READ = 65536;",
              "static const unsigned MAX_READ = 16384;", 1)

# 2. Уменьшить размер chunk до 64 байт (не 2048)
s = s.replace("        if (chunk > 2048) chunk = 2048;",
              "        if (chunk > 64) chunk = 64;", 1)

# 3. Убрать trace на каждый scanline (слишком много логов)
s = s.replace('        PngDbg("[PNG] after BeginImage", 0, 0);\n', '')
s = s.replace('            PngDbg("[PNG] scanline y", (unsigned)y, (unsigned)h);\n', '')
s = s.replace('        PngDbg("[PNG] before EndImage", 0, 0);\n', '')
s = s.replace('        PngDbg("[PNG] after EndImage", 0, 0);\n', '')

# 4. Убрать trace внутри psp_decode_png (оставим только начало/конец)
s = s.replace('    PngDbg("[PNG] parsed", (unsigned)width, (unsigned)height);\n', '')
s = s.replace('    PngDbg("[PNG] bitDepth/color", (unsigned)bitDepth, (unsigned)colorType);\n', '')
s = s.replace('    PngDbg("[PNG] idatSize", idatSize, 0);\n', '')
s = s.replace('    PngDbg("[PNG] before uncompress rawSize", rawSize, idatSize);\n', '')
s = s.replace('    PngDbg("[PNG] uncompress ret", (unsigned)zr, (unsigned)destLen);\n', '')
s = s.replace('    PngDbg("[PNG] before filters", bytesPerRow, height);\n', '')
s = s.replace('    PngDbg("[PNG] filters done", width, height);\n', '')

# 5. Проверить что read total всё ещё логируется
if 'PngDbg("[PNG] read total"' not in s:
    print("WARN: read total trace missing")

open(p,"w").write(s)
print("png.cpp trimmed")
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
