#!/bin/bash
set -e
cd ~/psp/hitr-psp

python3 - <<'PY'
import re

# Файлы, где мы наставили трейсов
files = [
    "code/main/game.cpp",
    "code/contexts/bootupcontext.cpp",
    "code/contexts/frontendcontext.cpp",
    "code/presentation/gui/guisystem.cpp",
    "code/loading/loadingmanager.cpp",
    "code/loading/scroobyfilehandler.cpp",
    "code/main/psp_globals.cpp",
    "libs/pure3d/p3d/sprite.cpp",
    "libs/pure3d/p3d/image.cpp",
    "libs/pure3d/p3d/png.cpp",
    "libs/pure3d/p3d/texture.cpp",
    "libs/pure3d/pddi/gu/gucon.cpp",
    "libs/pure3d/pddi/gu/guprim.cpp",
    "libs/radcore/src/radfile/psp/pspdrive.cpp",
    "libs/radcore/src/radfile/common/filesystem.cpp",
    "libs/radcore/src/radfile/common/drivethread.cpp",
    "libs/radcontent/src/radload/manager.cpp",
    "libs/scrooby/src/ResourceManager/FeResourceManager.cpp",
]

# Убираем всё что пишет trace в hitr_*.log
patterns = [
    # fopen-based trace blocks
    re.compile(r'#ifdef RAD_PSP\s*\{\s*FILE\* _f = fopen\("ms0:/hitr_[^"]*"[^}]*\}\s*#endif\n', re.DOTALL),
    # sceIoOpen-based inline blocks
    re.compile(r'#ifdef RAD_PSP\s*\{\s*SceUID _fd = sceIoOpen\("ms0:/hitr_[^}]*\}\s*#endif\n', re.DOTALL),
    # Our static helpers
    re.compile(r'#ifdef RAD_PSP\n#include <pspiofilemgr\.h>\nstatic void [A-Za-z]+Tr\([^)]*\)\s*\{[^}]*\}\n#else\n#define [A-Za-z]+Tr\(x\) \(\(void\)0\)\n#endif\n', re.DOTALL),
]

for fname in files:
    try:
        with open(fname) as f:
            s = f.read()
    except FileNotFoundError:
        continue
    orig = s
    for pat in patterns:
        s = pat.sub("", s)
    # Дополнительно: удалить вызовы Tr("[...]") на строке
    s = re.sub(r'^\s*[A-Za-z]+Tr\("[^"]*"\);\s*\n', '', s, flags=re.MULTILINE)
    s = re.sub(r'^\s*FEStTr\("[^"]*"\);\s*\n', '', s, flags=re.MULTILINE)
    if s != orig:
        with open(fname, "w") as f:
            f.write(s)
        print(f"stripped: {fname}")
PY

echo "=== rebuild ==="
docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build pspdev/pspdev:latest \
  bash -c "make SRR2 -j2 2>&1 | tail -8"

docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build/code pspdev/pspdev:latest \
  bash -c "mksfoex -d MEMSIZE=2 'HitR' PARAM.SFO && \
           psp-strip SRR2 -o SRR2_strip.elf && \
           pack-pbp EBOOT.PBP PARAM.SFO NULL NULL NULL NULL NULL SRR2_strip.elf NULL"

ls -la build/code/EBOOT.PBP
echo "DONE"
