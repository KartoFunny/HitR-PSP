#!/bin/bash
set -e
cd ~/psp/hitr-psp

# Глобальный disable всех трейсов
python3 - <<'PY'
import re, os

# Файлы где есть трейсы
files = [
    "libs/pure3d/p3d/psppng.cpp",
    "libs/pure3d/p3d/sprite.cpp",
    "libs/pure3d/p3d/texture.cpp",
    "libs/pure3d/p3d/png.cpp",
    "libs/pure3d/pddi/gu/gucon.cpp",
    "libs/pure3d/pddi/gu/guprim.cpp",
    "libs/pure3d/pddi/gu/gudev.cpp",
    "libs/radcore/src/radfile/psp/pspdrive.cpp",
    "libs/radcore/src/radfile/common/filesystem.cpp",
    "libs/radcore/src/radfile/common/drivethread.cpp",
    "libs/radcontent/src/radload/manager.cpp",
    "libs/pure3d/p3d/loadmanager.cpp",
    "code/main/game.cpp",
    "code/main/pspplatform.cpp",
    "code/main/psp_globals.cpp",
    "code/contexts/bootupcontext.cpp",
    "code/contexts/frontendcontext.cpp",
    "code/presentation/gui/guisystem.cpp",
    "code/loading/loadingmanager.cpp",
    "code/loading/scroobyfilehandler.cpp",
    "code/loading/p3dfilehandler.cpp",
]

# Регекспы для удаления helper-функций и их вызовов
helper_pattern = re.compile(
    r'\n#ifdef RAD_PSP\n(?:#include <[^>]+>\n)*static void (\w+Tr\d*|PDLog|PngDbg|ChTr\d*|FEUpdTr|SFHTr|CGSTr|UpdTr|RunTr\d*|SndTr|P3DTr|LMTr|SprTr|ImgTr|TexTr|RenderTr|PspTrLD|PspTrFS|PspTrDT|FTTr)\([^)]*\)\s*\{(?:[^{}]|\{[^{}]*\})*\}\n#else\n#define \1\([^\n]*\n#endif\n',
    re.DOTALL)

for f in files:
    if not os.path.exists(f):
        continue
    with open(f) as fh:
        s = fh.read()
    orig = s
    s = helper_pattern.sub("", s)
    if s != orig:
        with open(f, "w") as fh:
            fh.write(s)
        print(f"cleaned helpers in: {f}")
PY

# В psppng.cpp — сделать PngDbg no-op в один вызов
python3 - <<'PY'
p="libs/pure3d/p3d/psppng.cpp"
s=open(p).read()

# Найти определение PngDbg и сделать его пустым
idx = s.find("static void PngDbg(")
if idx >= 0:
    brace = s.find('{', idx)
    depth = 1
    pos = brace + 1
    while depth > 0 and pos < len(s):
        if s[pos] == '{': depth += 1
        elif s[pos] == '}': depth -= 1
        pos += 1
    new_body = '''static void PngDbg(const char* tag, unsigned v1, unsigned v2) { (void)tag; (void)v1; (void)v2; }'''
    s = s[:idx] + new_body + s[pos:]
    print("PngDbg nop")

open(p,"w").write(s)
PY

# В psppng.cpp — убрать вызовы PngDbg
python3 - <<'PY'
import re
p="libs/pure3d/p3d/psppng.cpp"
s=open(p).read()
s = re.sub(r'^\s*PngDbg\("[^"]*",[^;]*\);\s*\n', '', s, flags=re.MULTILINE)
open(p,"w").write(s)
print("PngDbg calls removed")
PY

# В pspdrive.cpp — оставить только Factory, убрать PROBE/OpenFile trace
python3 - <<'PY'
import re
p="libs/radcore/src/radfile/psp/pspdrive.cpp"
s=open(p).read()
s = re.sub(r'^\s*PDLog\([^;]*\);\s*\n', '', s, flags=re.MULTILINE)
s = re.sub(r'^\s*PDLOG\([^)]*\);\s*\n', '', s, flags=re.MULTILINE)
open(p,"w").write(s)
print("pspdrive cleaned")
PY

echo "=== verify ==="
for f in libs/pure3d/p3d/psppng.cpp libs/radcore/src/radfile/psp/pspdrive.cpp; do
  echo "--- $f ---"
  grep -c "PngDbg\|PDLog\|fopen.*hitr" "$f" 2>/dev/null || echo 0
done

echo "=== rebuild ==="
docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build pspdev/pspdev:latest \
  bash -c "make SRR2 -j2 2>&1 | tail -6"

docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build/code pspdev/pspdev:latest \
  bash -c "mksfoex -d MEMSIZE=2 'HitR' PARAM.SFO && \
           psp-strip SRR2 -o SRR2_strip.elf && \
           pack-pbp EBOOT.PBP PARAM.SFO NULL NULL NULL NULL NULL SRR2_strip.elf NULL"

ls -la build/code/EBOOT.PBP
rm -f ~/.config/ppsspp/hitr_*.log
echo
echo "DONE — run EBOOT, wait 5 MINUTES (real, wall clock)"
echo "Watch PPSSPP window: does FPS counter move at all?"
