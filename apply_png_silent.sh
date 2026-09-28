#!/bin/bash
set -e
cd ~/psp/hitr-psp

python3 - <<'PY'
p="libs/pure3d/p3d/psppng.cpp"
s=open(p).read()

# 1. PngDbg — только первые 5 PNG
s = s.replace("if (++s_cnt > 50000) return;", "if (++s_cnt > 15) return;", 1)

# 2. Убрать trace "ok" вообще (это вдвое меньше syscall)
old="""        PngDbg("[PNG] ok", (unsigned)(w*10000 + h), 0);
        builder->BeginImage"""
new="""        builder->BeginImage"""
if old in s:
    s=s.replace(old,new,1)
    print("ok trace removed")

# 3. Trace "c/sz" только первые 15 — оставим (в PngDbg уже лимит)
# 4. Убираем trace "FAIL" тоже (PngDbg сработает только первые 15)
# это не важно — PngDbg ограничен

open(p,"w").write(s)
print("silent mode")
PY

# Также отключим PngDbg в других местах
python3 - <<'PY'
p="libs/pure3d/p3d/psppng.cpp"
s=open(p).read()
# Все PngDbg вызовы становятся no-op, кроме первых 15 — но они уже под лимитом
print("done")
PY

echo "=== rebuild ==="
docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build pspdev/pspdev:latest \
  bash -c "rm -f libs/pure3d/CMakeFiles/p3d.dir/p3d/psppng.cpp.obj && \
           make SRR2 -j2 2>&1 | tail -6"

docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build/code pspdev/pspdev:latest \
  bash -c "mksfoex -d MEMSIZE=2 'HitR' PARAM.SFO && \
           psp-strip SRR2 -o SRR2_strip.elf && \
           pack-pbp EBOOT.PBP PARAM.SFO NULL NULL NULL NULL NULL SRR2_strip.elf NULL"

ls -la build/code/EBOOT.PBP
rm -f ~/.config/ppsspp/hitr_*.log
echo "DONE — run EBOOT, wait 5 MINUTES (5 real minutes!)"
