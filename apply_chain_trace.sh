#!/bin/bash
set -e
cd ~/psp/hitr-psp

python3 - <<'PY'
import re

# 1. tSpriteLoader — trace в LoadObject и LoadTexture
p="libs/pure3d/p3d/sprite.cpp"
s=open(p).read()

# helper
if "ChTr" not in s:
    idx=s.find('#include'); end=s.find('\n',idx)
    helper='''

#ifdef RAD_PSP
#include <cstdio>
static void ChTr(const char* tag) {
    static int cnt = 0; if (cnt > 1000) return; cnt++;
    FILE* f = fopen("ms0:/hitr_chain.log", "a");
    if (!f) return;
    fputs(tag, f); fputs("\\n", f); fclose(f);
}
#else
#define ChTr(x) ((void)0)
#endif
'''
    s = s[:end+1] + helper + s[end+1:]
    print("ChTr added")

# trace в LoadObject
old="""tEntity* tSpriteLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{
"""
new="""tEntity* tSpriteLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{
    ChTr("[Spr] LoadObject enter");
"""
if "ChTr(\"[Spr] LoadObject enter\")" not in s:
    assert old in s, "LoadObject anchor not found"
    s=s.replace(old,new,1)
    print("LoadObject traced")

# trace в LoadTexture (real one)
old2="""tTexture* tSpriteLoader::LoadTexture(tChunkFile* f, int depth /*=32*/)
{
"""
new2="""tTexture* tSpriteLoader::LoadTexture(tChunkFile* f, int depth /*=32*/)
{
    ChTr("[Spr] LoadTexture enter");
"""
if "ChTr(\"[Spr] LoadTexture enter\")" not in s:
    if old2 in s:
        s=s.replace(old2,new2,1)
        print("LoadTexture traced")
    else:
        print("WARN: LoadTexture anchor not found")

# trace перед первым imageLoader->LoadImage
old3="image = imageLoader->LoadImage(f, 32);"
new3='ChTr("[Spr] before imageLoader->LoadImage"); image = imageLoader->LoadImage(f, 32); ChTr("[Spr] after imageLoader->LoadImage");'
if "[Spr] before imageLoader" not in s:
    if old3 in s:
        s=s.replace(old3,new3,1)
        print("imageLoader->LoadImage traced")

open(p,"w").write(s)
PY

# 2. Trace в tImageFactory::ParseAsTexture
python3 - <<'PY'
p="libs/pure3d/p3d/imagefactory.cpp"
s=open(p).read()

if "ChTr2" not in s:
    idx=s.find('#include'); end=s.find('\n',idx)
    helper='''

#ifdef RAD_PSP
#include <cstdio>
static void ChTr2(const char* tag) {
    static int cnt = 0; if (cnt > 1000) return; cnt++;
    FILE* f = fopen("ms0:/hitr_chain.log", "a");
    if (!f) return;
    fputs(tag, f); fputs("\\n", f); fclose(f);
}
#else
#define ChTr2(x) ((void)0)
#endif
'''
    s = s[:end+1] + helper + s[end+1:]
    print("ChTr2 added")

old="""    handler->CreateImage(file, &builder);
    tTexture* texture = builder.GetTexture();"""
new="""    ChTr2("[ImgF] before handler->CreateImage");
    handler->CreateImage(file, &builder);
    ChTr2("[ImgF] after handler->CreateImage");
    tTexture* texture = builder.GetTexture();"""
if "[ImgF] before handler->CreateImage" not in s:
    if old in s:
        s=s.replace(old,new,1)
        print("ParseAsTexture traced")

open(p,"w").write(s)
PY

echo "=== rebuild ==="
docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build pspdev/pspdev:latest \
  bash -c "rm -f libs/pure3d/CMakeFiles/p3d.dir/p3d/sprite.cpp.obj \
                    libs/pure3d/CMakeFiles/p3d.dir/p3d/imagefactory.cpp.obj \
                    libs/pure3d/CMakeFiles/p3d.dir/p3d/psppng.cpp.obj && \
           make SRR2 -j2 2>&1 | tail -10"

docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build/code pspdev/pspdev:latest \
  bash -c "mksfoex -d MEMSIZE=2 'HitR' PARAM.SFO && \
           psp-strip SRR2 -o SRR2_strip.elf && \
           pack-pbp EBOOT.PBP PARAM.SFO NULL NULL NULL NULL NULL SRR2_strip.elf NULL"

rm -f ~/.config/ppsspp/hitr_*.log
echo "DONE — run EBOOT, wait 60s, kill"
