#!/bin/bash
set -e
cd ~/psp/hitr-psp

python3 - <<'PY'
p="code/presentation/gui/guisystem.cpp"
s=open(p).read()

old="""        case GUI_MSG_RUN_FRONTEND:
        {
            // thaw frontend render layer
            GetRenderManager()->mpLayer(RenderEnums::GUI)->Thaw();

            m_pApp->SetProject( m_pProject );

            m_state = FRONTEND_ACTIVE;

            // start the frontend manager
            rAssert( m_pManagerFrontEnd );
            if( param1 != 0 )
            {
                m_pManagerFrontEnd->Start( static_cast<CGuiWindow::eGuiWindowID>( param1 ) );
            }
            else
            {
                m_pManagerFrontEnd->Start();
            }"""

new="""        case GUI_MSG_RUN_FRONTEND:
        {
#ifdef RAD_PSP
            // PSP: PNG broken, Scrooby pages not loaded, CGuiManagerFrontEnd
            // was never populated → m_pManagerFrontEnd has no windows. Skip
            // all of this; just set state and return.
            {
                FILE* _f = fopen("ms0:/hitr_force.log", "a");
                if (_f) { fputs("[CGS] PSP: RUN_FRONTEND SKIPPED\\n", _f); fclose(_f); }
            }
            m_state = FRONTEND_ACTIVE;
#else
            // thaw frontend render layer
            GetRenderManager()->mpLayer(RenderEnums::GUI)->Thaw();

            m_pApp->SetProject( m_pProject );

            m_state = FRONTEND_ACTIVE;

            // start the frontend manager
            rAssert( m_pManagerFrontEnd );
            if( param1 != 0 )
            {
                m_pManagerFrontEnd->Start( static_cast<CGuiWindow::eGuiWindowID>( param1 ) );
            }
            else
            {
                m_pManagerFrontEnd->Start();
            }
#endif"""

assert old in s, "RUN_FRONTEND anchor not found"
s=s.replace(old,new,1)
print("RUN_FRONTEND guarded")

# cstdio — для fopen
if "#include <cstdio>" not in s:
    idx=s.find('#include')
    s = s[:idx] + "#include <cstdio>\n" + s[idx:]
    print("cstdio added")

open(p,"w").write(s)
PY

echo "=== check ==="
grep -n "RUN_FRONTEND" code/presentation/gui/guisystem.cpp
sed -n '/case GUI_MSG_RUN_FRONTEND:/,/case GUI_MSG_INIT_MINIGAME/p' code/presentation/gui/guisystem.cpp | head -40

echo "=== rebuild ==="
docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build pspdev/pspdev:latest \
  bash -c "rm -f code/CMakeFiles/SRR2.dir/presentation/gui/guisystem.cpp.obj && \
           make SRR2 -j2 2>&1 | tail -8"

docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build/code pspdev/pspdev:latest \
  bash -c "mksfoex -d MEMSIZE=2 'HitR' PARAM.SFO && \
           psp-strip SRR2 -o SRR2_strip.elf && \
           pack-pbp EBOOT.PBP PARAM.SFO NULL NULL NULL NULL NULL SRR2_strip.elf NULL"

ls -la build/code/EBOOT.PBP
rm -f ~/.config/ppsspp/hitr_*.log
echo "DONE — run EBOOT, wait 20s"
