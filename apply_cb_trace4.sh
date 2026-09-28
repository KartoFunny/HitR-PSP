#!/bin/bash
set -e
cd ~/psp/hitr-psp

python3 - <<'PY'
p="libs/pure3d/p3d/loadmanager.cpp"
s=open(p).read()

# 1. Убрать всё, что мы насовали (helper + inline блок)
import re
# Удалить helper
s = re.sub(r'\n#ifdef RAD_PSP\n#include <pspiofilemgr\.h>\nstatic void CbTr[^}]*}\n#else\n#define CbTr[^\n]*\n#endif\n', '', s)
# Удалить inline блок из Done
s = re.sub(r'#ifdef RAD_PSP\n    \{\n        SceUID _fd = sceIoOpen\("ms0:/hitr_cb\.log"[^\n]*\n        if \(_fd >= 0\) \{ sceIoWrite\(_fd, "\[CB\] InternalCallback::Done enter\\\\n", 36\); sceIoClose\(_fd\); \}\n    \}\n#endif\n', '', s)

# 2. Добавить unconditional include в самый верх файла
lines = s.split('\n')
# Найти первую строку с #include
first_inc = -1
for i, ln in enumerate(lines[:30]):
    if ln.lstrip().startswith('#include'):
        first_inc = i
        break

if first_inc >= 0:
    include_block = [
        '#ifdef RAD_PSP',
        '#include <pspiofilemgr.h>',
        '#include <pspkernel.h>',
        '#endif',
        ''
    ]
    lines = lines[:first_inc] + include_block + lines[first_inc:]
    s = '\n'.join(lines)
    print("include block added at top")

# 3. Заменить на простой inline trace
old="""void tLoadRequest::InternalCallback::Done()
{
"""
new="""void tLoadRequest::InternalCallback::Done()
{
#ifdef RAD_PSP
    {
        SceUID _fd = sceIoOpen("ms0:/hitr_cb.log", 0x602, 0777);
        if (_fd >= 0) { sceIoWrite(_fd, "[CB] InternalCallback::Done enter\\n", 36); sceIoClose(_fd); }
    }
#endif
"""
assert old in s, "Done anchor not found"
s=s.replace(old,new,1)
print("Done traced inline")

open(p,"w").write(s)
PY

echo "=== rebuild ==="
docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build pspdev/pspdev:latest \
  bash -c "rm -f libs/pure3d/CMakeFiles/p3d.dir/p3d/loadmanager.cpp.obj && \
           make SRR2 -j2 2>&1 | tail -12"

docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build/code pspdev/pspdev:latest \
  bash -c "mksfoex -d MEMSIZE=2 'HitR' PARAM.SFO && \
           psp-strip SRR2 -o SRR2_strip.elf && \
           pack-pbp EBOOT.PBP PARAM.SFO NULL NULL NULL NULL NULL SRR2_strip.elf NULL"

ls -la build/code/EBOOT.PBP
rm -f ~/.config/ppsspp/hitr_*.log
echo "DONE — run EBOOT, wait 20s"
