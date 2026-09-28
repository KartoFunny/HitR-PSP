#!/bin/bash
set -e
cd ~/psp/hitr-psp

python3 - <<'PY'
# 1. Возвращаем psp_png_load к рабочему варианту
p="libs/pure3d/p3d/psppng.cpp"
s=open(p).read()

start = s.find("void psp_png_load(tFile* file, tImageHandler::Builder* builder)")
assert start >= 0
brace = s.find('{', start)
depth = 1
pos = brace + 1
while depth > 0 and pos < len(s):
    if s[pos] == '{': depth += 1
    elif s[pos] == '}': depth -= 1
    pos += 1
end = pos

new_func = r'''void psp_png_load(tFile* file, tImageHandler::Builder* builder)
{
    // Trace: is this function even being called?
    {
        static int s_entered = 0;
        if (++s_entered <= 10) {
            PngDbg("[PNG] load enter", (unsigned)s_entered, 0);
        }
    }

    static const unsigned MAX_READ = 65536;
    unsigned char* buf = (unsigned char*)malloc(MAX_READ);
    if (!buf) return;

    unsigned total = 0;
    bool found_end = false;
    while (!found_end && total < MAX_READ)
    {
        unsigned chunk = MAX_READ - total;
        if (chunk > 2048) chunk = 2048;

        file->GetData(buf + total, chunk, tFile::BYTE);
        unsigned prev = total;
        total += chunk;

        unsigned scanStart = (prev >= 7) ? (prev - 7) : 0;
        for (unsigned i = scanStart; i + 8 <= total; ++i)
        {
            if (buf[i]   == 'I' && buf[i+1] == 'E' &&
                buf[i+2] == 'N' && buf[i+3] == 'D')
            {
                total = i + 8;
                found_end = true;
                break;
            }
        }
    }

    if (!found_end || total < 16) {
        static int s_no_iend = 0;
        if (++s_no_iend <= 20) PngDbg("[PNG] no IEND", total, 0);
        free(buf);
        return;
    }

    // Progress: log every 100th PNG
    {
        static int s_count = 0;
        if (++s_count % 100 == 0) PngDbg("[PNG] count", (unsigned)s_count, total);
    }

    int w = 0, h = 0;
    unsigned char* rgba = 0;
    if (psp_decode_png(buf, total, &w, &h, &rgba))
    {
        builder->BeginImage(w, h, 32, tImageHandler::Builder::TOP, (pddiColour*)0);
        for (int y = 0; y < h; y++) {
            builder->ProcessScanline32((unsigned*)(rgba + (size_t)y * w * 4));
        }
        builder->EndImage();
        free(rgba);
    }
    free(buf);
}'''
s = s[:start] + new_func + s[end:]
open(p,"w").write(s)
print("psp_png_load restored")
PY

# 2. Трейсы в FrontEndContext::OnUpdate
python3 - <<'PY'
p="code/contexts/frontendcontext.cpp"
s=open(p).read()

old="""void FrontEndContext::OnUpdate( unsigned int elapsedTime )
{"""
new="""#ifdef RAD_PSP
#include <cstdio>
static void FEUpdTr(const char* tag) {
    static int cnt = 0; if (cnt > 600) return; cnt++;
    FILE* f = fopen("ms0:/hitr_fe2.log", "a");
    if (!f) return;
    fputs(tag, f); fputs("\\n", f); fclose(f);
}
#else
#define FEUpdTr(x) ((void)0)
#endif

void FrontEndContext::OnUpdate( unsigned int elapsedTime )
{
    FEUpdTr("[FE2] OnUpdate enter");"""
assert old in s, "OnUpdate anchor not found"
s=s.replace(old,new,1)

old2="""    GetGameDataManager()->Update( elapsedTime );

    // update GUI system
    //
    GetGuiSystem()->Update( elapsedTime );

    //Chuck: adding this so that the rewards manager reflects changes found in the charactersheet.
    GetRewardsManager()->SynchWithCharacterSheet();"""
new2="""    FEUpdTr("[FE2] before GDM");
    GetGameDataManager()->Update( elapsedTime );
    FEUpdTr("[FE2] before GuiSystem");
    GetGuiSystem()->Update( elapsedTime );
    FEUpdTr("[FE2] after GuiSystem");
    GetRewardsManager()->SynchWithCharacterSheet();
    FEUpdTr("[FE2] end");"""
assert old2 in s, "OnUpdate body anchor not found"
s=s.replace(old2,new2,1)
print("FE OnUpdate traced")
open(p,"w").write(s)
PY

# 3. Лимит PngDbg поднять
python3 - <<'PY'
p="libs/pure3d/p3d/psppng.cpp"
s=open(p).read()
s = s.replace("if (++s_cnt > 10000) return;", "if (++s_cnt > 10000) return;", 1)
open(p,"w").write(s)
print("PngDbg limit ok")
PY

echo "=== rebuild ==="
docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build pspdev/pspdev:latest \
  bash -c "rm -f libs/pure3d/CMakeFiles/p3d.dir/p3d/psppng.cpp.obj \
                    code/CMakeFiles/SRR2.dir/contexts/frontendcontext.cpp.obj && \
           make SRR2 -j2 2>&1 | tail -10"

docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build/code pspdev/pspdev:latest \
  bash -c "mksfoex -d MEMSIZE=2 'HitR' PARAM.SFO && \
           psp-strip SRR2 -o SRR2_strip.elf && \
           pack-pbp EBOOT.PBP PARAM.SFO NULL NULL NULL NULL NULL SRR2_strip.elf NULL"

ls -la build/code/EBOOT.PBP
rm -f ~/.config/ppsspp/hitr_*.log
echo "DONE — run EBOOT, wait 60s"
