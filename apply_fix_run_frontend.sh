#!/bin/bash
set -e
cd ~/psp/hitr-psp

python3 - <<'PY'
p="code/presentation/gui/guisystem.cpp"
s=open(p).read()

# Найти case GUI_MSG_RUN_FRONTEND и добавить PSP-блок
old="""        case GUI_MSG_RUN_FRONTEND:
        {
            // thaw frontend render layer
            GetRenderManager()->mpLayer(RenderEnums::GUI)->Thaw();

            m_pApp->SetProject( m_pProject );"""

new="""        case GUI_MSG_RUN_FRONTEND:
        {
#ifdef RAD_PSP
            // PSP: async load completion never reached us, so the FrontEnd
            // manager was never created. Try to create it here from whatever
            // project is already in the inventory / Scrooby global.
            if ( m_pManagerFrontEnd == NULL )
            {
                FILE* _lf = fopen("ms0:/hitr_force.log", "a");
                if (_lf) { fputs("[CGS] creating m_pManagerFrontEnd in RUN_FRONTEND\\n", _lf); fclose(_lf); }

                Scrooby::Project* proj = NULL;
                if ( m_pProject ) {
                    proj = m_pProject;
                } else {
                    // Try global Scrooby project retrieval
                    proj = Scrooby::App::GetInstance()->GetProject();
                }
                if ( proj != NULL )
                {
                    m_pProject = proj;
                    m_state = FRONTEND_LOADING;
                    this->OnProjectLoadComplete( proj );
                    if (_lf) { _lf = fopen("ms0:/hitr_force.log", "a"); if (_lf) { fputs("[CGS] OnProjectLoadComplete done\\n", _lf); fclose(_lf); } }
                } else {
                    if (_lf) { _lf = fopen("ms0:/hitr_force.log", "a"); if (_lf) { fputs("[CGS] no project found!\\n", _lf); fclose(_lf); } }
                }
            }
#endif
            // thaw frontend render layer
            GetRenderManager()->mpLayer(RenderEnums::GUI)->Thaw();

            m_pApp->SetProject( m_pProject );"""

assert old in s, "RUN_FRONTEND anchor not found"
s=s.replace(old,new,1)
print("HandleMessage patched")

open(p,"w").write(s)
PY

# Проверим, есть ли GetProject в Scrooby::App
echo "=== Scrooby::App::GetProject? ==="
grep -n "GetProject\|m_pProject\b" libs/scrooby/src/FeApp.cpp libs/scrooby/inc/FeApp.hpp 2>/dev/null | head -10
grep -rn "class App\b\|Scrooby::App::GetInstance" libs/scrooby/inc --include='*.hpp' | head -5

echo "=== rebuild ==="
docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build pspdev/pspdev:latest \
  bash -c "rm -f code/CMakeFiles/SRR2.dir/presentation/gui/guisystem.cpp.obj && \
           make SRR2 -j2 2>&1 | tail -15"

docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build/code pspdev/pspdev:latest \
  bash -c "mksfoex -d MEMSIZE=2 'HitR' PARAM.SFO && \
           psp-strip SRR2 -o SRR2_strip.elf && \
           pack-pbp EBOOT.PBP PARAM.SFO NULL NULL NULL NULL NULL SRR2_strip.elf NULL"

ls -la build/code/EBOOT.PBP
rm -f ~/.config/ppsspp/hitr_*.log
echo "DONE — run EBOOT, wait 20s"
