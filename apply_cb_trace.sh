#!/bin/bash
set -e
cd ~/psp/hitr-psp

# 1. Увеличить лимиты RunTr до 20000
python3 - <<'PY'
p="code/main/game.cpp"
s=open(p).read()
s=s.replace("static int cnt = 0; if (cnt > 2000) return; cnt++;",
            "static int cnt = 0; if (cnt > 20000) return; cnt++;",1)
s=s.replace("static int cnt = 0; if (cnt > 1000) return; cnt++;",
            "static int cnt = 0; if (cnt > 20000) return; cnt++;",1)
open(p,"w").write(s)
print("RunTr limit raised")
PY

# 2. Расширить FE OnUpdate до 200 кадров
python3 - <<'PY'
p="code/contexts/frontendcontext.cpp"
s=open(p).read()
old="if (++s_count <= 5) {"
new="if (++s_count <= 200) {"
s=s.replace(old,new,1)
# И trace каждые 30 кадров вместо 60 для точности
s=s.replace("if ((++s_frm % 60) == 0)","if ((++s_frm % 30) == 0)",1)
open(p,"w").write(s)
print("FE OnUpdate limit raised")
PY

# 3. Trace в tLoadRequest::InternalCallback::Done
python3 - <<'PY'
p="libs/pure3d/p3d/loadmanager.cpp"
s=open(p).read()

helper='''
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
if "CbTr" not in s:
    idx=s.find('#include'); end=s.find('\n',idx)
    s = s[:end+1] + helper + s[end+1:]
    print("CbTr helper added to loadmanager.cpp")

# Trace в InternalCallback::Done
old="""void tLoadRequest::InternalCallback::Done()
{
"""
new="""void tLoadRequest::InternalCallback::Done()
{
    CbTr("[CB] InternalCallback::Done enter");
"""
if old in s:
    s=s.replace(old,new,1)
    print("InternalCallback::Done traced")

open(p,"w").write(s)
PY

# 4. Trace в ScroobyFileHandler::OnProjectLoadComplete и CGuiSystem::OnProjectLoadComplete
python3 - <<'PY'
p="code/loading/scroobyfilehandler.cpp"
s=open(p).read()
if "SFHTr" not in s:
    idx=s.find('#include'); end=s.find('\n',idx)
    helper='''
#ifdef RAD_PSP
#include <pspiofilemgr.h>
static void SFHTr(const char* tag) {
    static int cnt = 0; if (cnt > 100) return; cnt++;
    SceUID fd = sceIoOpen("ms0:/hitr_cb.log", PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0777);
    if (fd < 0) return;
    int n=0; while(tag[n]) n++;
    sceIoWrite(fd, tag, n); sceIoWrite(fd, "\\n", 1); sceIoClose(fd);
}
#else
#define SFHTr(x) ((void)0)
#endif
'''
    s = s[:end+1] + helper + s[end+1:]

old="""void ScroobyFileHandler::OnProjectLoadComplete( Scrooby::Project* pProject )
{
    rAssert( mpCallback );"""
new="""void ScroobyFileHandler::OnProjectLoadComplete( Scrooby::Project* pProject )
{
    SFHTr("[SFH] OnProjectLoadComplete enter");
    rAssert( mpCallback );"""
if old in s:
    s=s.replace(old,new,1)
    print("ScroobyFileHandler::OnProjectLoadComplete traced")

open(p,"w").write(s)
PY

python3 - <<'PY'
p="code/presentation/gui/guisystem.cpp"
s=open(p).read()
if "CGSTr" not in s:
    idx=s.find('#include'); end=s.find('\n',idx)
    helper='''
#ifdef RAD_PSP
#include <pspiofilemgr.h>
static void CGSTr(const char* tag) {
    static int cnt = 0; if (cnt > 100) return; cnt++;
    SceUID fd = sceIoOpen("ms0:/hitr_cb.log", PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0777);
    if (fd < 0) return;
    int n=0; while(tag[n]) n++;
    sceIoWrite(fd, tag, n); sceIoWrite(fd, "\\n", 1); sceIoClose(fd);
}
#else
#define CGSTr(x) ((void)0)
#endif
'''
    s = s[:end+1] + helper + s[end+1:]

old="""void CGuiSystem::OnProjectLoadComplete( Scrooby::Project* pProject )
{
    //LOGI("GUI: OnProjectLoadComplete entered. pProject=%p state=%d", pProject, (int)m_state);"""
new="""void CGuiSystem::OnProjectLoadComplete( Scrooby::Project* pProject )
{
    CGSTr("[CGS] OnProjectLoadComplete enter");
    //LOGI("GUI: OnProjectLoadComplete entered. pProject=%p state=%d", pProject, (int)m_state);"""
if old in s:
    s=s.replace(old,new,1)
    print("CGuiSystem::OnProjectLoadComplete traced")

open(p,"w").write(s)
PY

echo "=== rebuild ==="
docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build pspdev/pspdev:latest \
  bash -c "rm -f code/CMakeFiles/SRR2.dir/main/game.cpp.obj \
                    code/CMakeFiles/SRR2.dir/contexts/frontendcontext.cpp.obj \
                    code/CMakeFiles/SRR2.dir/loading/scroobyfilehandler.cpp.obj \
                    code/CMakeFiles/SRR2.dir/presentation/gui/guisystem.cpp.obj \
                    libs/pure3d/CMakeFiles/p3d.dir/p3d/loadmanager.cpp.obj && \
           make SRR2 -j2 2>&1 | tail -12"

docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build/code pspdev/pspdev:latest \
  bash -c "mksfoex -d MEMSIZE=2 'HitR' PARAM.SFO && \
           psp-strip SRR2 -o SRR2_strip.elf && \
           pack-pbp EBOOT.PBP PARAM.SFO NULL NULL NULL NULL NULL SRR2_strip.elf NULL"

ls -la build/code/EBOOT.PBP
rm -f ~/.config/ppsspp/hitr_*.log
echo "DONE — run EBOOT, wait 20s"
