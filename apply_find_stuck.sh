#!/bin/bash
set -e
cd ~/psp/hitr-psp

echo "=== 1. Смотрим fileftt.cpp WaitForCompletion ==="
grep -n "SwitchTask\|WaitForCompletion\|IsComplete" libs/pure3d/p3d/fileftt.cpp
sed -n '150,180p' libs/pure3d/p3d/fileftt.cpp
sed -n '220,250p' libs/pure3d/p3d/fileftt.cpp
sed -n '300,330p' libs/pure3d/p3d/fileftt.cpp

echo
echo "=== 2. Убираем SwitchTask трейсы (мешают) ==="
python3 - <<'PY'
import re
p="libs/pure3d/p3d/loadmanager.cpp"
s=open(p).read()
# Удаляем все наши fopen-блоки с hitr_st.log
s = re.sub(r'#ifdef RAD_PSP\n    \{ FILE\* _f = fopen\("ms0:/hitr_st\.log"[^\n]*\n#endif\n', '', s)
s = re.sub(r'^\s*#ifdef RAD_PSP\n\s*\{ FILE\* _f = fopen\("ms0:/hitr_st\.log"[\s\S]*?#endif\n', '', s, flags=re.MULTILINE)
s = s.replace('    STr("[ST] enter");\n', '')
open(p,"w").write(s)
print("switch traces cleaned")
PY

echo "=== 3. Ускоряем PNG: chunk = 8192, MAX_READ = 512KB ==="
python3 - <<'PY'
p="libs/pure3d/p3d/psppng.cpp"
s=open(p).read()

s = s.replace("static const unsigned MAX_READ = 65536;",
              "static const unsigned MAX_READ = 524288;", 1)
s = s.replace("        if (chunk > 2048) chunk = 2048;",
              "        if (chunk > 8192) chunk = 8192;", 1)
s = s.replace("if (++s_no_iend <= 20) PngDbg(\"[PNG] no IEND\", total, 0);",
              "if (++s_no_iend <= 500) PngDbg(\"[PNG] no IEND\", total, 0);", 1)
s = s.replace("if (++s_count % 10 == 0) PngDbg(\"[PNG] count\", (unsigned)s_count, total);",
              "if (++s_count % 5 == 0) PngDbg(\"[PNG] count\", (unsigned)s_count, total);", 1)
open(p,"w").write(s)
print("psppng speedup")
PY

echo "=== 4. Trace: какой файл ждёт fileftt ==="
python3 - <<'PY'
p="libs/pure3d/p3d/fileftt.cpp"
s=open(p).read()

# helper
if "FTTr" not in s:
    idx=s.find('#include'); end=s.find('\n',idx)
    helper='''

#ifdef RAD_PSP
#include <cstdio>
static void FTTr(const char* tag, const char* name) {
    static int cnt = 0; if (cnt > 300) return; cnt++;
    FILE* f = fopen("ms0:/hitr_ftt.log", "a");
    if (!f) return;
    fputs(tag, f);
    if (name) { fputs(" ", f); fputs(name, f); }
    fputs("\\n", f); fclose(f);
}
#else
#define FTTr(a,b) ((void)0)
#endif
'''
    s = s[:end+1] + helper + s[end+1:]
    print("FTTr helper added")

# Найти while-SwitchTask циклы и логировать имя файла
# Строки 164, 232, 316
import re
# Паттерн: while(...) { ... p3d::loadManager->SwitchTask(); ... }
# Проще — заменить каждый "p3d::loadManager->SwitchTask();" в fileftt на trace+call
s = s.replace("p3d::loadManager->SwitchTask();",
              "FTTr(\"[FTT] loop\", GetFilename()); p3d::loadManager->SwitchTask();", 1)

open(p,"w").write(s)
print("fileftt traced (first occurrence)")
PY

echo "=== 5. Проверка что GetFilename существует ==="
grep -n "GetFilename\|m_Filename\|filename" libs/pure3d/p3d/fileftt.cpp | head -5
grep -n "GetFilename\|m_Filename" libs/pure3d/p3d/file.hpp libs/pure3d/p3d/fileftt.hpp 2>/dev/null | head -5

echo "=== rebuild ==="
docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build pspdev/pspdev:latest \
  bash -c "rm -f libs/pure3d/CMakeFiles/p3d.dir/p3d/loadmanager.cpp.obj \
                    libs/pure3d/CMakeFiles/p3d.dir/p3d/psppng.cpp.obj \
                    libs/pure3d/CMakeFiles/p3d.dir/p3d/fileftt.cpp.obj && \
           make SRR2 -j2 2>&1 | tail -15"

docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build/code pspdev/pspdev:latest \
  bash -c "mksfoex -d MEMSIZE=2 'HitR' PARAM.SFO && \
           psp-strip SRR2 -o SRR2_strip.elf && \
           pack-pbp EBOOT.PBP PARAM.SFO NULL NULL NULL NULL NULL SRR2_strip.elf NULL"

ls -la build/code/EBOOT.PBP
rm -f ~/.config/ppsspp/hitr_*.log
echo "DONE — run EBOOT, wait 60s"
