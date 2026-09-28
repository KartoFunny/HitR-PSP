#!/bin/bash
set -e
cd ~/psp/hitr-psp

# === 1. Полная очистка loadmanager.cpp ===
python3 - <<'PY'
p="libs/pure3d/p3d/loadmanager.cpp"
with open(p) as f:
    lines = f.readlines()

out = []
i = 0
removed = 0
while i < len(lines):
    ln = lines[i]
    # Начало блока #ifdef RAD_PSP с hitr_cb/sceIo
    if ln.strip().startswith("#ifdef RAD_PSP"):
        j = i + 1
        depth = 1
        body = []
        while j < len(lines):
            body.append(lines[j])
            if lines[j].strip().startswith("#ifdef"):
                depth += 1
            elif lines[j].strip().startswith("#endif"):
                depth -= 1
                if depth == 0:
                    break
            j += 1
        b = ''.join(body)
        if 'hitr_cb' in b or 'sceIo' in b or 'SceUID' in b:
            removed += 1
            i = j + 1
            continue
    out.append(ln)
    i += 1

with open(p, "w") as f:
    f.writelines(out)
print(f"cleaned {removed} blocks")
PY

echo "=== check clean ==="
grep -n "hitr_cb\|sceIo\|SceUID" libs/pure3d/p3d/loadmanager.cpp || echo "(clean)"

# === 2. Trace в 4 точках ===
python3 - <<'PY'
# 2a. FeResourceManager::P3DCallback::Done
p="libs/scrooby/src/ResourceManager/FeResourceManager.cpp"
s=open(p).read()

# helper
if "FRMTr" not in s:
    idx=s.find('#include'); end=s.find('\n',idx)
    helper='''
#include <cstdio>
static void FRMTr(const char* tag) {
    static int cnt = 0; if (cnt > 200) return; cnt++;
    FILE* f = fopen("ms0:/hitr_cb.log", "a");
    if (!f) return;
    fputs(tag, f); fputs("\\n", f); fclose(f);
}
'''
    s = s[:end+1] + helper + s[end+1:]
    print("FRMTr helper added")

# Найти P3DCallback::Done
import re
m = re.search(r'void\s+FeResourceManager::P3DCallback::Done\s*\(\s*\)\s*\{', s)
if not m:
    # Возможно класс внутри FeResourceManager — ищем иначе
    m = re.search(r'void\s+P3DCallback::Done\s*\(\s*\)\s*\{', s)
if m:
    pos = m.end()
    s = s[:pos] + '\n    FRMTr("[P3DCB] Done enter");' + s[pos:]
    print("P3DCallback::Done traced")
else:
    print("WARN: P3DCallback::Done not found")

open(p,"w").write(s)
PY

python3 - <<'PY'
# 2b. ScroobyFileHandler::OnProjectLoadComplete
p="code/loading/scroobyfilehandler.cpp"
s=open(p).read()

if "SFHTr" not in s:
    idx=s.find('#include'); end=s.find('\n',idx)
    helper='''
#include <cstdio>
static void SFHTr(const char* tag) {
    FILE* f = fopen("ms0:/hitr_cb.log", "a");
    if (!f) return;
    fputs(tag, f); fputs("\\n", f); fclose(f);
}
'''
    s = s[:end+1] + helper + s[end+1:]

old="void ScroobyFileHandler::OnProjectLoadComplete( Scrooby::Project* pProject )\n{\n"
new="""void ScroobyFileHandler::OnProjectLoadComplete( Scrooby::Project* pProject )
{
    SFHTr("[SFH] OnProjectLoadComplete enter");
"""
if old in s and "[SFH] OnProjectLoadComplete enter" not in s:
    s=s.replace(old,new,1)
    print("SFH::OnProjectLoadComplete traced")

open(p,"w").write(s)
PY

python3 - <<'PY'
# 2c. CGuiSystem::OnProjectLoadComplete
p="code/presentation/gui/guisystem.cpp"
s=open(p).read()

if "CGSTr" not in s:
    idx=s.find('#include'); end=s.find('\n',idx)
    helper='''
#include <cstdio>
static void CGSTr(const char* tag) {
    FILE* f = fopen("ms0:/hitr_cb.log", "a");
    if (!f) return;
    fputs(tag, f); fputs("\\n", f); fclose(f);
}
'''
    s = s[:end+1] + helper + s[end+1:]

old="void CGuiSystem::OnProjectLoadComplete( Scrooby::Project* pProject )\n{\n"
new="""void CGuiSystem::OnProjectLoadComplete( Scrooby::Project* pProject )
{
    CGSTr("[CGS] OnProjectLoadComplete enter");
"""
if old in s and "[CGS] OnProjectLoadComplete enter" not in s:
    s=s.replace(old,new,1)
    print("CGuiSystem::OnProjectLoadComplete traced")

# 2d. FrontEndContext::StartFrontEnd
open(p,"w").write(s)
PY

python3 - <<'PY'
p="code/contexts/frontendcontext.cpp"
s=open(p).read()

if "FEStTr" not in s:
    idx=s.find('#include'); end=s.find('\n',idx)
    helper='''
#include <cstdio>
static void FEStTr(const char* tag) {
    FILE* f = fopen("ms0:/hitr_cb.log", "a");
    if (!f) return;
    fputs(tag, f); fputs("\\n", f); fclose(f);
}
'''
    s = s[:end+1] + helper + s[end+1:]

# StartFrontEnd
old="void FrontEndContext::StartFrontEnd( unsigned int initialScreen )\n{\n"
new="""void FrontEndContext::StartFrontEnd( unsigned int initialScreen )
{
    FEStTr("[FEC] StartFrontEnd enter");
"""
if old in s and "[FEC] StartFrontEnd enter" not in s:
    s=s.replace(old,new,1)
    print("FE::StartFrontEnd traced")

# OnProcessRequestsComplete
old2="void FrontEndContext::OnProcessRequestsComplete( void* pUserData )\n{\n"
new2="""void FrontEndContext::OnProcessRequestsComplete( void* pUserData )
{
    FEStTr("[FEC] OnProcessRequestsComplete enter");
"""
if old2 in s and "[FEC] OnProcessRequestsComplete enter" not in s:
    s=s.replace(old2,new2,1)
    print("FE::OnProcessRequestsComplete traced")

open(p,"w").write(s)
PY

echo "=== rebuild ==="
docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build pspdev/pspdev:latest \
  bash -c "rm -f libs/pure3d/CMakeFiles/p3d.dir/p3d/loadmanager.cpp.obj \
                    libs/scrooby/CMakeFiles/scrooby.dir/src/ResourceManager/FeResourceManager.cpp.obj \
                    code/CMakeFiles/SRR2.dir/loading/scroobyfilehandler.cpp.obj \
                    code/CMakeFiles/SRR2.dir/presentation/gui/guisystem.cpp.obj \
                    code/CMakeFiles/SRR2.dir/contexts/frontendcontext.cpp.obj && \
           make SRR2 -j2 2>&1 | tail -12"

docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build/code pspdev/pspdev:latest \
  bash -c "mksfoex -d MEMSIZE=2 'HitR' PARAM.SFO && \
           psp-strip SRR2 -o SRR2_strip.elf && \
           pack-pbp EBOOT.PBP PARAM.SFO NULL NULL NULL NULL NULL SRR2_strip.elf NULL"

ls -la build/code/EBOOT.PBP
rm -f ~/.config/ppsspp/hitr_*.log
echo "DONE — run EBOOT, wait 25s"
