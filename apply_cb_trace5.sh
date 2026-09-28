#!/bin/bash
set -e
cd ~/psp/hitr-psp

python3 - <<'PY'
import re
p="libs/pure3d/p3d/loadmanager.cpp"
s=open(p).read()

# 1. Убрать все следы наших вставок про sceIo
s = re.sub(r'#ifdef RAD_PSP\n#include <pspiofilemgr\.h>\n#include <pspkernel\.h>\n#endif\n\n', '', s)
s = re.sub(r'#ifdef RAD_PSP\n#include <pspiofilemgr\.h>\n#endif\n', '', s)
s = re.sub(r'#ifdef RAD_PSP\n    \{\n        SceUID _fd = sceIoOpen\("ms0:/hitr_cb\.log"[^}]*\}\n#endif\n', '', s)
s = re.sub(r'\n#ifdef RAD_PSP\n    \{\n        SceUID _fd = sceIoOpen\("ms0:/hitr_cb\.log"[^}]*\}\n#endif\n', '', s)

# 2. Добавить cstdio в самое начало (первый #include)
if "#include <cstdio>" not in s:
    idx = s.find('#include')
    if idx >= 0:
        s = s[:idx] + "#include <cstdio>\n#include <cstdlib>\n" + s[idx:]
        print("cstdio included")

# 3. Заменить на fopen-based trace
old_patterns = [
    'void tLoadRequest::InternalCallback::Done()\n{\n#ifdef RAD_PSP\n    {\n        SceUID _fd = sceIoOpen("ms0:/hitr_cb.log", 0x602, 0777);\n        if (_fd >= 0) { sceIoWrite(_fd, "[CB] InternalCallback::Done enter\\n", 36); sceIoClose(_fd); }\n    }\n#endif\n',
    'void tLoadRequest::InternalCallback::Done()\n{\n#ifdef RAD_PSP\n    {\n        SceUID _fd = sceIoOpen("ms0:/hitr_cb.log", PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0777);\n        if (_fd >= 0) { sceIoWrite(_fd, "[CB] InternalCallback::Done enter\\n", 36); sceIoClose(_fd); }\n    }\n#endif\n',
    'void tLoadRequest::InternalCallback::Done()\n{\n'
]

new_fn = '''void tLoadRequest::InternalCallback::Done()
{
#ifdef RAD_PSP
    {
        FILE* _f = fopen("ms0:/hitr_cb.log", "a");
        if (_f) { fputs("[CB] InternalCallback::Done enter\\n", _f); fclose(_f); }
    }
#endif
'''

done = False
for pat in old_patterns:
    if pat in s:
        s = s.replace(pat, new_fn, 1)
        print(f"Done traced (pattern matched)")
        done = True
        break
if not done:
    print("WARN: no Done pattern matched")

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
