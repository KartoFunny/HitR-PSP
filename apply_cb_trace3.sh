#!/bin/bash
set -e
cd ~/psp/hitr-psp

python3 - <<'PY'
p="libs/pure3d/p3d/loadmanager.cpp"
s=open(p).read()

# 1. Убрать плохой helper полностью
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
s = s.replace(bad_helper, "")

# 2. Inline trace в Done — без helper
old="""void tLoadRequest::InternalCallback::Done()
{
    CbTr("[CB] InternalCallback::Done enter");"""
new="""void tLoadRequest::InternalCallback::Done()
{
#ifdef RAD_PSP
    {
        SceUID _fd = sceIoOpen("ms0:/hitr_cb.log", PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0777);
        if (_fd >= 0) { sceIoWrite(_fd, "[CB] InternalCallback::Done enter\\n", 36); sceIoClose(_fd); }
    }
#endif"""

if old in s:
    s=s.replace(old,new,1)
    print("Done traced inline")
else:
    # Попробуем без прежнего trace
    old2="""void tLoadRequest::InternalCallback::Done()
{
"""
    new2="""void tLoadRequest::InternalCallback::Done()
{
#ifdef RAD_PSP
    {
        SceUID _fd = sceIoOpen("ms0:/hitr_cb.log", PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0777);
        if (_fd >= 0) { sceIoWrite(_fd, "[CB] InternalCallback::Done enter\\n", 36); sceIoClose(_fd); }
    }
#endif
"""
    if old2 in s:
        s=s.replace(old2,new2,1)
        print("Done traced inline (variant)")

# 3. Убедиться что pspiofilemgr.h подключён в этом файле
if "#include <pspiofilemgr.h>" not in s:
    idx=s.find('#include'); end=s.find('\n',idx)
    s=s[:end+1] + "\n#ifdef RAD_PSP\n#include <pspiofilemgr.h>\n#endif\n" + s[end+1:]
    print("pspiofilemgr.h added")

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
