#!/bin/bash
set -e
cd ~/psp/hitr-psp

# 1. Обернуть m_pManagerFrontEnd->Start в guisystem.cpp
python3 - <<'PY'
p="code/presentation/gui/guisystem.cpp"
s=open(p).read()

old="""            m_state = FRONTEND_ACTIVE;

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
new="""            m_state = FRONTEND_ACTIVE;

            // start the frontend manager
            rAssert( m_pManagerFrontEnd );
#ifdef RAD_PSP
            // PSP: Populate was skipped → no windows registered, Start() would
            // crash trying to activate a null window. Skip.
            {
                FILE* _f = fopen("ms0:/hitr_force.log", "a");
                if (_f) { fputs("[CGS] PSP: FE Start SKIPPED\\n", _f); fclose(_f); }
            }
#else
            if( param1 != 0 )
            {
                m_pManagerFrontEnd->Start( static_cast<CGuiWindow::eGuiWindowID>( param1 ) );
            }
            else
            {
                m_pManagerFrontEnd->Start();
            }
#endif"""
assert old in s, "Start anchor not found"
s=s.replace(old,new,1)
print("FrontEnd Start skipped")
open(p,"w").write(s)
PY

# 2. Также обернуть в case GUI_MSG_RUN_INGAME/BACKEND/MINIGAME на всякий случай — они тоже могут сработать
python3 - <<'PY'
p="code/presentation/gui/guisystem.cpp"
s=open(p).read()

# GUI_MSG_RUN_INGAME
old="""            m_state = INGAME_ACTIVE;

            // start the in-game manager
            rAssert( m_pManagerInGame );
            m_pManagerInGame->Start();"""
new="""            m_state = INGAME_ACTIVE;

            // start the in-game manager
            rAssert( m_pManagerInGame );
#ifdef RAD_PSP
            // PSP: Populate skipped for ingame manager too
#else
            m_pManagerInGame->Start();
#endif"""
if old in s:
    s=s.replace(old,new,1)
    print("InGame Start guarded")

open(p,"w").write(s)
PY

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
echo "DONE — run EBOOT, wait 25s"
