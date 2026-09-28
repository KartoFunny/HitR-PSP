#!/bin/bash
set -e
cd ~/psp/hitr-psp

python3 - <<'PY'
p="code/contexts/frontendcontext.cpp"
s=open(p).read()

old="""void FrontEndContext::StartFrontEnd( unsigned int initialScreen )
{
    FEStTr("[FEC] StartFrontEnd enter");
    // Start up GUI frontend manager
    GetGuiSystem()->HandleMessage( GUI_MSG_RUN_FRONTEND, initialScreen );

    //
    // Notify the sound system that the front end is starting
    //
    GetSoundManager()->OnFrontEndStart();
}"""

new="""void FrontEndContext::StartFrontEnd( unsigned int initialScreen )
{
    FEStTr("[FEC] StartFrontEnd enter");
    // Start up GUI frontend manager
    GetGuiSystem()->HandleMessage( GUI_MSG_RUN_FRONTEND, initialScreen );

#ifdef RAD_PSP
    // PSP: SoundManager is a stub returning nullptr; skip.
    FEStTr("[FEC] StartFrontEnd done (PSP: sound skipped)");
#else
    //
    // Notify the sound system that the front end is starting
    //
    GetSoundManager()->OnFrontEndStart();
#endif
}"""

assert old in s, "StartFrontEnd anchor not found"
s=s.replace(old,new,1)
print("StartFrontEnd patched")

open(p,"w").write(s)
PY

echo "=== rebuild ==="
docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build pspdev/pspdev:latest \
  bash -c "rm -f code/CMakeFiles/SRR2.dir/contexts/frontendcontext.cpp.obj && \
           make SRR2 -j2 2>&1 | tail -8"

docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build/code pspdev/pspdev:latest \
  bash -c "mksfoex -d MEMSIZE=2 'HitR' PARAM.SFO && \
           psp-strip SRR2 -o SRR2_strip.elf && \
           pack-pbp EBOOT.PBP PARAM.SFO NULL NULL NULL NULL NULL SRR2_strip.elf NULL"

ls -la build/code/EBOOT.PBP
rm -f ~/.config/ppsspp/hitr_*.log
echo "DONE — run EBOOT, wait 25s"
