#!/bin/bash
set -e
cd ~/psp/hitr-psp

python3 - <<'PY'
p="code/presentation/gui/frontend/guimanagerfrontend.cpp"
s=open(p).read()

# 1. Populate
old="""void CGuiManagerFrontEnd::Populate()
{
MEMTRACK_PUSH_GROUP( "CGUIManagerFrontEnd" );"""
new="""void CGuiManagerFrontEnd::Populate()
{
#ifdef RAD_PSP
    // PSP: Scrooby pages are not loaded (PNG stub broke inner elements), so
    // building CGuiScreenXxx objects would crash on null pages. Skip.
    {
        FILE* _f = fopen("ms0:/hitr_force.log", "a");
        if (_f) { fputs("[CGS] PSP: FE Populate SKIPPED\\n", _f); fclose(_f); }
    }
    return;
#endif
MEMTRACK_PUSH_GROUP( "CGUIManagerFrontEnd" );"""
assert old in s, "Populate anchor not found"
s=s.replace(old,new,1)
print("Populate skipped")

open(p,"w").write(s)
PY

# 2. Проверим есть ли Start в этом файле и пропустим его
echo "=== Start в guimanagerfrontend.cpp ==="
grep -n "::Start\|void Start\|virtual.*Start" code/presentation/gui/frontend/guimanagerfrontend.cpp code/presentation/gui/frontend/guimanagerfrontend.h 2>/dev/null | head -10

echo "=== Start в базовом CGuiManager ==="
grep -rn "CGuiManager::Start\|void.*::Start" code/presentation/gui/*.cpp | grep -i manager | head -10

echo "=== rebuild ==="
docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build pspdev/pspdev:latest \
  bash -c "rm -f code/CMakeFiles/SRR2.dir/presentation/gui/frontend/guimanagerfrontend.cpp.obj && \
           make SRR2 -j2 2>&1 | tail -8"

docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build/code pspdev/pspdev:latest \
  bash -c "mksfoex -d MEMSIZE=2 'HitR' PARAM.SFO && \
           psp-strip SRR2 -o SRR2_strip.elf && \
           pack-pbp EBOOT.PBP PARAM.SFO NULL NULL NULL NULL NULL SRR2_strip.elf NULL"

ls -la build/code/EBOOT.PBP
rm -f ~/.config/ppsspp/hitr_*.log
echo "DONE — run EBOOT, wait 20s"
