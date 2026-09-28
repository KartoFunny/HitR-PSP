#!/bin/bash
set -e
cd ~/psp/hitr-psp

python3 - <<'PY'
p="libs/pure3d/p3d/loadmanager.cpp"
s=open(p).read()

# helper
if "STr" not in s or "static void STr" not in s:
    idx=s.find('#include'); end=s.find('\n',idx)
    helper='''

#ifdef RAD_PSP
#include <cstdio>
static void STr(const char* tag) {
    static int cnt = 0; if (cnt > 500) return; cnt++;
    FILE* f = fopen("ms0:/hitr_st.log", "a");
    if (!f) return;
    fputs(tag, f); fputs("\\n", f); fclose(f);
}
#else
#define STr(x) ((void)0)
#endif
'''
    s = s[:end+1] + helper + s[end+1:]
    print("STr helper added")

# trace внутрь SwitchTask — найти по сигнатуре
if "[ST] enter" not in s:
    old="void tLoadManager::SwitchTask(void)\n{"
    new="void tLoadManager::SwitchTask(void)\n{\n    STr(\"[ST] enter\");"
    assert old in s, "SwitchTask anchor not found"
    s=s.replace(old,new,1)
    print("SwitchTask traced")

# trace вокруг radLoad->Service
if "[ST] before radLoad->Service" not in s:
    old2="    radLoad->Service();"
    new2="    STr(\"[ST] before radLoad->Service\");\n    radLoad->Service();\n    STr(\"[ST] after radLoad->Service\");"
    assert old2 in s, "radLoad->Service anchor not found"
    s=s.replace(old2,new2,1)
    print("radLoad->Service traced")

open(p,"w").write(s)
PY

echo "=== count every 10 in psppng ==="
python3 - <<'PY'
p="libs/pure3d/p3d/psppng.cpp"
s=open(p).read()
if "s_count % 10 ==" not in s and "s_count % 100 ==" in s:
    s = s.replace("s_count % 100 ==", "s_count % 10 ==", 1)
    open(p,"w").write(s)
    print("count every 10")
else:
    print("already 10 or not found")
PY

echo "=== rebuild ==="
docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build pspdev/pspdev:latest \
  bash -c "rm -f libs/pure3d/CMakeFiles/p3d.dir/p3d/loadmanager.cpp.obj \
                    libs/pure3d/CMakeFiles/p3d.dir/p3d/psppng.cpp.obj && \
           make SRR2 -j2 2>&1 | tail -8"

docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build/code pspdev/pspdev:latest \
  bash -c "mksfoex -d MEMSIZE=2 'HitR' PARAM.SFO && \
           psp-strip SRR2 -o SRR2_strip.elf && \
           pack-pbp EBOOT.PBP PARAM.SFO NULL NULL NULL NULL NULL SRR2_strip.elf NULL"

ls -la build/code/EBOOT.PBP
rm -f ~/.config/ppsspp/hitr_*.log
echo "DONE — run EBOOT, wait 60s"
