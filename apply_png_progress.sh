#!/bin/bash
set -e
cd ~/psp/hitr-psp

python3 - <<'PY'
p="libs/pure3d/p3d/psppng.cpp"
s=open(p).read()

# 1. Увеличить лимит PngDbg с 200 до 10000
s = s.replace("if (++s_cnt > 200) return;", "if (++s_cnt > 10000) return;", 1)

# 2. Вернуть trace "read total" в psp_png_load
old="""    if (!found_end || total < 16) {
        PngDbg("[PNG] no IEND", total, 0);
        free(buf);
        return;
    }"""
new="""    // Count PNG loads for progress tracking (only every 100th)
    {
        static int s_png_count = 0;
        if (++s_png_count % 100 == 0) {
            PngDbg("[PNG] count", (unsigned)s_png_count, total);
        }
    }

    if (!found_end || total < 16) {
        PngDbg("[PNG] no IEND", total, 0);
        free(buf);
        return;
    }"""
assert old in s, "no IEND anchor not found"
s=s.replace(old,new,1)
print("trace added")

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
echo "DONE — run EBOOT, wait 120s (2 minutes!)"
