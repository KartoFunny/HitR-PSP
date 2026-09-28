#!/bin/bash
set -e
cd ~/psp/hitr-psp

python3 - <<'PY'
p="libs/pure3d/p3d/loadmanager.cpp"
s=open(p).read()

# 1. Убрать плохой helper
import re
bad_helper = '''
#ifdef RAD_PSP
#include <pspiofilemgr.h>
static void CbTr(const char* tag) {
    static int cnt = 0; if (cnt > 500) return; cnt++;
    SceUID fd = sceIoOpen("ms0:/hitr_cb.log", PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0777);
    if (fd < 0) return;
    int n=0; while(tag[n]) n++;
    sceIoWrite(fd, tag, n); sceIoWrite(fd, "\\n", 1); sceIoClose(fd);
}
#else
#define CbTr(x) ((void)0)
#endif
'''
if bad_helper in s:
    s=s.replace(bad_helper,"",1)
    print("bad helper removed")

# 2. Вставить нормальный helper — ДО namespace p3d
# Найти первое "namespace p3d" и вставить перед ним
helper = '''
#ifdef RAD_PSP
#include <pspiofilemgr.h>
static void CbTr(const char* tag) {
    static int cnt = 0; if (cnt > 500) return; cnt++;
    SceUID fd = sceIoOpen("ms0:/hitr_cb.log", PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0777);
    if (fd < 0) return;
    int n=0; while(tag[n]) n++;
    sceIoWrite(fd, tag, n); sceIoWrite(fd, "\\n", 1); sceIoClose(fd);
}
#else
#define CbTr(x) ((void)0)
#endif

'''
if "static void CbTr" not in s:
    ns_pos = s.find("namespace p3d")
    if ns_pos < 0:
        ns_pos = s.find("#include")
    if ns_pos >= 0:
        s = s[:ns_pos] + helper + s[ns_pos:]
        print("helper placed before namespace p3d")
    else:
        raise SystemExit("no good insertion point")

# 3. Убедиться что вызов CbTr на месте
if "CbTr(\"[CB] InternalCallback::Done enter\")" not in s:
    old="""void tLoadRequest::InternalCallback::Done()
{
"""
    new="""void tLoadRequest::InternalCallback::Done()
{
    CbTr("[CB] InternalCallback::Done enter");
"""
    if old in s:
        s=s.replace(old,new,1)
        print("Done traced")

open(p,"w").write(s)
PY

echo "=== rebuild ==="
docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build pspdev/pspdev:latest \
  bash -c "rm -f libs/pure3d/CMakeFiles/p3d.dir/p3d/loadmanager.cpp.obj && \
           make SRR2 -j2 2>&1 | tail -12"

docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build/code pspdev/pspdev:latest \
  bash -c "mksfoex -d MEMSIZE=2 'HitR' PARAM.SFO && \
           psp-strip SRR2 -o SRR2_strip.elf && \
           pack-pbp EBOOT.PBP PARAM.SFO NULL NULL NULL NULL NULL SRR2_strip.elf NULL"

ls -la build/code/EBOOT.PBP
rm -f ~/.config/ppsspp/hitr_*.log
echo "DONE — run EBOOT, wait 20s"
