#!/bin/bash
set -e
cd ~/psp/hitr-psp

python3 - <<'PY'
p="libs/pure3d/p3d/psppng.cpp"
s=open(p).read()

# 1. HARD_CAP = 2 МБ (вместо 8)
s = s.replace("static const unsigned HARD_CAP = 8 * 1024 * 1024;",
              "static const unsigned HARD_CAP = 2 * 1024 * 1024;", 1)

# 2. chunk = 16 КБ (вместо 4)
s = s.replace("        unsigned chunk = 4096;",
              "        unsigned chunk = 16384;", 1)

# 3. trace на каждый из первых 200
s = s.replace('bool trace = (myId <= 5) || (myId % 20 == 0);',
              'bool trace = (myId <= 200);', 1)

open(p,"w").write(s)
print("psppng traced every call")
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
