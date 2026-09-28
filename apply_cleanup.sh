#!/bin/bash
set -e
cd ~/psp/hitr-psp

python3 - <<'PY'
import re, os

# Все файлы, куда мы вставляли трейсы
files = [
    "libs/pure3d/pddi/gu/guprim.cpp",
    "libs/pure3d/pddi/gu/gucon.cpp",
    "libs/pure3d/p3d/fileftt.cpp",
    "libs/pure3d/p3d/sprite.cpp",
    "libs/pure3d/p3d/texture.cpp",
    "libs/pure3d/p3d/png.cpp",
    "libs/pure3d/p3d/psppng.cpp",
    "libs/pure3d/p3d/loadmanager.cpp",
    "code/contexts/frontendcontext.cpp",
    "code/contexts/bootupcontext.cpp",
    "code/main/game.cpp",
    "code/main/pspplatform.cpp",
    "code/main/psp_globals.cpp",
    "code/presentation/gui/guisystem.cpp",
    "code/presentation/gui/frontend/guimanagerfrontend.cpp",
    "code/loading/loadingmanager.cpp",
    "code/loading/scroobyfilehandler.cpp",
    "code/loading/p3dfilehandler.cpp",
    "libs/radcore/src/radfile/psp/pspdrive.cpp",
    "libs/radcore/src/radfile/common/filesystem.cpp",
    "libs/radcore/src/radfile/common/drivethread.cpp",
    "libs/radcontent/src/radload/manager.cpp",
]

# === 1. Helper: static void XTr(...) { ... } → { } ===
# Разрешаем 3 уровня вложенности фигурных скобок
helper_re = re.compile(
    r'(static\s+void\s+\w+Tr\d*\s*\([^)]*\)\s*)\{'
    r'(?:[^{}]|\{[^{}]*\}|\{[^{}]*\{[^{}]*\}[^{}]*\}|\{[^{}]*\{[^{}]*\{[^{}]*\}[^{}]*\}[^{}]*\})*\}',
    re.DOTALL
)

# === 2. Inline блок с fopen ===
inline_fopen_re = re.compile(
    r'#ifdef\s+RAD_PSP\s*\{\s*FILE\s*\*\s*\w+\s*=\s*fopen\("ms0:/hitr_[^"]*"[^;]*;'
    r'(?:[^{}]|\{[^{}]*\})*\}\s*#endif',
    re.DOTALL
)

# === 3. Inline блок с sceIoOpen (многострочный) ===
inline_sceio_re = re.compile(
    r'#ifdef\s+RAD_PSP\s*\{\s*SceUID\s+\w+\s*=\s*sceIoOpen\("ms0:/hitr_[^"]*"[^;]*;'
    r'(?:[^{}]|\{[^{}]*\}|\{[^{}]*\{[^{}]*\}[^{}]*\})*\}\s*#endif',
    re.DOTALL
)

# === 4. Одиночный inline { ... } без #ifdef (если был вставлен без обёртки) ===
inline_fopen_re2 = re.compile(
    r'\{\s*FILE\s*\*\s*\w+\s*=\s*fopen\("ms0:/hitr_[^"]*"[^;]*;'
    r'(?:[^{}]|\{[^{}]*\})*\}',
    re.DOTALL
)

for f in files:
    if not os.path.exists(f):
        continue
    s = open(f).read()
    orig = s
    
    # Сначала #ifdef-обёртки
    s = inline_fopen_re.sub('', s)
    s = inline_sceio_re.sub('', s)
    
    # Потом одиночные
    s = inline_fopen_re2.sub('{ }', s)
    
    # Потом helper-функции
    s = helper_re.sub(r'\1{ }', s)
    
    if s != orig:
        open(f, 'w').write(s)
        print(f"cleaned: {f}")
PY

echo
echo "=== Проверка: остались ли fopen/sceIoOpen с hitr_ ==="
grep -rn 'fopen("ms0:/hitr_\|sceIoOpen("ms0:/hitr_' libs/ code/ --include='*.cpp' 2>/dev/null | grep -v "\.bak" | head -20

echo
echo "=== Rebuild ==="
docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build pspdev/pspdev:latest \
  bash -c "make SRR2 -j2 2>&1 | grep -E 'error|Built target SRR2' | tail -15"
