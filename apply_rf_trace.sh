#!/bin/bash
set -e
cd ~/psp/hitr-psp

python3 - <<'PY'
p="code/presentation/gui/guisystem.cpp"
s=open(p).read()

# Найти точное место и вставить trace пошагово
old="""        case GUI_MSG_RUN_FRONTEND:
        {
            // thaw frontend render layer
            GetRenderManager()->mpLayer(RenderEnums::GUI)->Thaw();

            m_pApp->SetProject( m_pProject );"""

new="""        case GUI_MSG_RUN_FRONTEND:
        {
#ifdef RAD_PSP
            { FILE* _f = fopen("ms0:/hitr_force.log", "a"); if (_f) { fputs("[CGS] RF enter\\n", _f); fclose(_f); } }
#endif
            // thaw frontend render layer
#ifdef RAD_PSP
            { FILE* _f = fopen("ms0:/hitr_force.log", "a"); if (_f) { fputs("[CGS] RF before Thaw\\n", _f); fclose(_f); } }
#endif
            GetRenderManager()->mpLayer(RenderEnums::GUI)->Thaw();
#ifdef RAD_PSP
            { FILE* _f = fopen("ms0:/hitr_force.log", "a"); if (_f) { fputs("[CGS] RF after Thaw\\n", _f); fclose(_f); } }
#endif

#ifdef RAD_PSP
            { FILE* _f = fopen("ms0:/hitr_force.log", "a"); if (_f) { fputs("[CGS] RF before SetProject\\n", _f); fclose(_f); } }
#endif
            m_pApp->SetProject( m_pProject );
#ifdef RAD_PSP
            { FILE* _f = fopen("ms0:/hitr_force.log", "a"); if (_f) { fputs("[CGS] RF after SetProject\\n", _f); fclose(_f); } }
#endif"""

assert old in s, "RUN_FRONTEND anchor not found"
s=s.replace(old,new,1)
print("RUN_FRONTEND traced")

open(p,"w").write(s)
PY

# cstdio нужен
python3 - <<'PY'
p="code/presentation/gui/guisystem.cpp"
s=open(p).read()
if "#include <cstdio>" not in s:
    idx=s.find('#include')
    s = s[:idx] + "#include <cstdio>\n" + s[idx:]
open(p,"w").write(s)
print("cstdio added")
PY

echo "=== rebuild ==="
docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build pspdev/pspdev:latest \
  bash -c "rm -f code/CMakeFiles/SRR2.dir/presentation/gui/guisystem.cpp.obj && \
           make SRR2 -j2 2>&1 | tail -10"

docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build/code pspdev/pspdev:latest \
  bash -c "mksfoex -d MEMSIZE=2 'HitR' PARAM.SFO && \
           psp-strip SRR2 -o SRR2_strip.elf && \
           pack-pbp EBOOT.PBP PARAM.SFO NULL NULL NULL NULL NULL SRR2_strip.elf NULL"

ls -la build/code/EBOOT.PBP
rm -f ~/.config/ppsspp/hitr_*.log
echo "DONE — run EBOOT, wait 20s"
