#!/bin/bash
set -e
cd ~/psp/hitr-psp

python3 - <<'PY'
p="code/contexts/frontendcontext.cpp"
s=open(p).read()

# В OnUpdate — форсировать StartFrontEnd через 30 кадров
old="""void FrontEndContext::OnUpdate( unsigned int elapsedTime )
{
#ifdef RAD_PSP
    {
        static int s_count = 0;
        if (++s_count <= 200) {"""

new="""void FrontEndContext::OnUpdate( unsigned int elapsedTime )
{
#ifdef RAD_PSP
    // PSP FORCE: after 30 frames, try to start the front end even if the
    // Scrooby async-load never reported completion (which is what we see).
    {
        static int s_force = 0;
        s_force++;
        if (s_force == 30) {
            FILE* f = fopen("ms0:/hitr_force.log", "a");
            if (f) { fputs("[FE] forcing StartFrontEnd at frame 30\\n", f); fclose(f); }
            this->StartFrontEnd( CGuiWindow::GUI_SCREEN_ID_SPLASH );
        }
    }
    {
        static int s_count = 0;
        if (++s_count <= 200) {"""

assert old in s, "OnUpdate anchor not found"
s=s.replace(old,new,1)
print("Force StartFrontEnd added")

open(p,"w").write(s)
PY

docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build pspdev/pspdev:latest \
  bash -c "rm -f code/CMakeFiles/SRR2.dir/contexts/frontendcontext.cpp.obj && \
           make SRR2 -j2 2>&1 | tail -8"

docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build/code pspdev/pspdev:latest \
  bash -c "mksfoex -d MEMSIZE=2 'HitR' PARAM.SFO && \
           psp-strip SRR2 -o SRR2_strip.elf && \
           pack-pbp EBOOT.PBP PARAM.SFO NULL NULL NULL NULL NULL SRR2_strip.elf NULL"

ls -la build/code/EBOOT.PBP
rm -f ~/.config/ppsspp/hitr_*.log
echo "DONE — run EBOOT, wait 20s"
