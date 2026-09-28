#!/bin/bash
set -e
cd ~/psp/hitr-psp

python3 - <<'PY'
p="libs/pure3d/p3d/png.cpp"
s=open(p).read()

# Проверим нет ли уже стаба
if "PSP: libpng 1.0.3 fails to initialize" in s:
    print("stub already present")
    raise SystemExit(0)

# Найти CreateImage и обернуть stub в начале
old="""void tPNGHandler::CreateImage(tFile* file, tImageHandler::Builder* builder)
{"""

new="""void tPNGHandler::CreateImage(tFile* file, tImageHandler::Builder* builder)
{
#ifdef RAD_PSP
    // PSP: libpng 1.0.3 fails to initialize on this platform.
    // Skip PNG decoding entirely; builder stays empty, caller gets NULL texture.
    (void)file; (void)builder;
    return;
#else"""

assert old in s, "CreateImage anchor not found"
s=s.replace(old,new,1)

# Найти конец CreateImage — вставить #endif перед закрывающей }
start = s.find("void tPNGHandler::CreateImage")
brace = s.find('{', start)
depth = 1
pos = brace + 1
while depth > 0 and pos < len(s):
    if s[pos] == '{': depth += 1
    elif s[pos] == '}': depth -= 1
    pos += 1
func_end = pos
# Вставить #endif прямо перед последней }
s = s[:func_end-1] + "#endif  // RAD_PSP\n" + s[func_end-1:]

open(p,"w").write(s)
print("clean PNG stub installed")
PY

echo "=== rebuild ==="
docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build pspdev/pspdev:latest \
  bash -c "rm -f libs/pure3d/CMakeFiles/p3d.dir/p3d/png.cpp.obj && \
           make SRR2 -j2 2>&1 | tail -12"

docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build/code pspdev/pspdev:latest \
  bash -c "mksfoex -d MEMSIZE=2 'HitR' PARAM.SFO && \
           psp-strip SRR2 -o SRR2_strip.elf && \
           pack-pbp EBOOT.PBP PARAM.SFO NULL NULL NULL NULL NULL SRR2_strip.elf NULL"

ls -la build/code/EBOOT.PBP
rm -f ~/.config/ppsspp/hitr_*.log
echo "DONE — run EBOOT, wait 15s"
