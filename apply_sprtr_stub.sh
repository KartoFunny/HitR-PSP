#!/bin/bash
set -e
cd ~/psp/hitr-psp

python3 - <<'PY'
p="libs/pure3d/p3d/sprite.cpp"
s=open(p).read()

# Если helper удалён — добавим пустую заглушку
if "#define SprTr" not in s:
    idx = s.find('#include')
    if idx >= 0:
        # Найти конец первого include блока
        end = s.find('\n', idx)
        # Вставить после всех подряд идущих #include
        pos = end + 1
        while pos < len(s) and s[pos:pos+8] == '#include':
            end = s.find('\n', pos)
            pos = end + 1
        stub = "\n// hitr stripped — empty trace stub\n#define SprTr(x) ((void)0)\n"
        s = s[:pos] + stub + s[pos:]
        print("SprTr stub added")
    else:
        raise SystemExit("no #include found")

open(p,"w").write(s)
PY

# Тот же приём для всех остальных файлов с оставшимися Tr() вызовами
for f in \
  libs/pure3d/p3d/image.cpp \
  libs/pure3d/p3d/png.cpp \
  libs/pure3d/p3d/texture.cpp \
  libs/pure3d/pddi/gu/gucon.cpp \
  libs/pure3d/pddi/gu/guprim.cpp \
  libs/radcore/src/radfile/common/filesystem.cpp \
  libs/radcore/src/radfile/common/drivethread.cpp \
  libs/radcontent/src/radload/manager.cpp \
  libs/radcore/src/radfile/psp/pspdrive.cpp \
  code/main/pspplatform.cpp \
  code/contexts/bootupcontext.cpp \
  code/contexts/frontendcontext.cpp \
  code/presentation/gui/guisystem.cpp \
  code/loading/loadingmanager.cpp \
  code/loading/scroobyfilehandler.cpp \
  libs/scrooby/src/ResourceManager/FeResourceManager.cpp ; do
  if [ -f "$f" ]; then
    python3 - "$f" <<'PY'
import sys
p = sys.argv[1]
s = open(p).read()

# Найти все наши уникальные Tr() теги
import re
tags = set(re.findall(r'\b([A-Z][A-Za-z]+Tr)\(', s))
stubs = []
for t in tags:
    if f"#define {t}" not in s:
        stubs.append(f"#define {t}(x) ((void)0)")

if stubs:
    idx = s.find('#include')
    if idx >= 0:
        end = s.find('\n', idx)
        pos = end + 1
        while pos < len(s) and s[pos:pos+8] == '#include':
            end = s.find('\n', pos)
            pos = end + 1
        stub_block = "\n// hitr stripped — empty trace stubs\n" + "\n".join(stubs) + "\n"
        s = s[:pos] + stub_block + s[pos:]
        open(p, "w").write(s)
        print(f"{p}: added {len(stubs)} stubs: {stubs}")
PY
  fi
done

echo "=== rebuild ==="
docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build pspdev/pspdev:latest \
  bash -c "make SRR2 -j2 2>&1 | tail -15"

docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build/code pspdev/pspdev:latest \
  bash -c "mksfoex -d MEMSIZE=2 'HitR' PARAM.SFO && \
           psp-strip SRR2 -o SRR2_strip.elf && \
           pack-pbp EBOOT.PBP PARAM.SFO NULL NULL NULL NULL NULL SRR2_strip.elf NULL"

ls -la build/code/EBOOT.PBP
rm -f ~/.config/ppsspp/hitr_*.log
echo "DONE — run EBOOT, wait 15s"
