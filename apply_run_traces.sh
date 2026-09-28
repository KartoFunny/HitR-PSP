#!/bin/bash
set -e
cd ~/psp/hitr-psp

# 1. count каждые 10 в psppng
python3 - <<'PY'
p="libs/pure3d/p3d/psppng.cpp"
s=open(p).read()
s = s.replace("if (++s_count % 100 == 0) PngDbg(\"[PNG] count\", (unsigned)s_count, total);",
              "if (++s_count % 10 == 0) PngDbg(\"[PNG] count\", (unsigned)s_count, total);", 1)
open(p,"w").write(s)
print("count every 10")
PY

# 2. Трейсы в Game::Run
python3 - <<'PY'
p="code/main/game.cpp"
s=open(p).read()

# helper
if "RunTr2" not in s:
    idx=s.find('#include'); end=s.find('\n',idx)
    helper='''

#ifdef RAD_PSP
#include <cstdio>
static void RunTr2(const char* tag) {
    static int cnt = 0; if (cnt > 200) return; cnt++;
    FILE* f = fopen("ms0:/hitr_run2.log", "a");
    if (!f) return;
    fputs(tag, f); fputs("\\n", f); fclose(f);
}
#else
#define RunTr2(x) ((void)0)
#endif
'''
    s = s[:end+1] + helper + s[end+1:]

# Оборачиваем RenderFlow и SwitchTask
old="""        if ( !mpPlatform->PausedForErrors() )
        {
            DEMOPROFILE( g_DemoProfiler.Start(PROFILE_CHANNEL_AI); )
            mpTimerList->Service(); //nv
            mpGameFlow->OnTimerDone(elapsed, NULL);
            DEMOPROFILE( g_DemoProfiler.Stop(PROFILE_CHANNEL_AI); )

            if( !mExitNow )
            {
                DEMOPROFILE( g_DemoProfiler.Start(PROFILE_CHANNEL_RENDER); )
                mpRenderFlow->OnTimerDone(elapsed, NULL);
                DEMOPROFILE( g_DemoProfiler.Stop(PROFILE_CHANNEL_RENDER); )
            }
        }"""
new="""        RunTr2("[Run2] top");
        if ( !mpPlatform->PausedForErrors() )
        {
            RunTr2("[Run2] before GameFlow");
            DEMOPROFILE( g_DemoProfiler.Start(PROFILE_CHANNEL_AI); )
            mpTimerList->Service(); //nv
            mpGameFlow->OnTimerDone(elapsed, NULL);
            DEMOPROFILE( g_DemoProfiler.Stop(PROFILE_CHANNEL_AI); )
            RunTr2("[Run2] after GameFlow");

            if( !mExitNow )
            {
                RunTr2("[Run2] before RenderFlow");
                DEMOPROFILE( g_DemoProfiler.Start(PROFILE_CHANNEL_RENDER); )
                mpRenderFlow->OnTimerDone(elapsed, NULL);
                DEMOPROFILE( g_DemoProfiler.Stop(PROFILE_CHANNEL_RENDER); )
                RunTr2("[Run2] after RenderFlow");
            }
        }"""
assert old in s, "Game::Run anchor not found"
s=s.replace(old,new,1)

# SwitchTask блок
old2="""        ::radFileService();
        ::radDbgComService();
        ::radDebugConsoleService();"""
new2="""        RunTr2("[Run2] before radFileService");
        ::radFileService();
        RunTr2("[Run2] after radFileService");
        ::radDbgComService();
        ::radDebugConsoleService();"""
if old2 in s:
    s=s.replace(old2,new2,1)
    print("radFileService traced")

old3="""        p3d::loadManager->SwitchTask();"""
new3="""        RunTr2("[Run2] before SwitchTask");
        p3d::loadManager->SwitchTask();
        RunTr2("[Run2] after SwitchTask");"""
if old3 in s and "[Run2] before SwitchTask" not in s:
    s=s.replace(old3,new3,1)
    print("SwitchTask traced")

open(p,"w").write(s)
PY

echo "=== rebuild ==="
docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build pspdev/pspdev:latest \
  bash -c "rm -f libs/pure3d/CMakeFiles/p3d.dir/p3d/psppng.cpp.obj \
                    code/CMakeFiles/SRR2.dir/main/game.cpp.obj && \
           make SRR2 -j2 2>&1 | tail -10"

docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build/code pspdev/pspdev:latest \
  bash -c "mksfoex -d MEMSIZE=2 'HitR' PARAM.SFO && \
           psp-strip SRR2 -o SRR2_strip.elf && \
           pack-pbp EBOOT.PBP PARAM.SFO NULL NULL NULL NULL NULL SRR2_strip.elf NULL"

ls -la build/code/EBOOT.PBP
rm -f ~/.config/ppsspp/hitr_*.log
echo "DONE — run EBOOT, wait 60s"
