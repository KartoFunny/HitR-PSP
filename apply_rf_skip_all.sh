#!/bin/bash
set -e
cd ~/psp/hitr-psp

python3 - <<'PY'
p="code/presentation/gui/guisystem.cpp"
s=open(p).read()

# Найти case GUI_MSG_RUN_FRONTEND ... до следующего case
import re
start_m = re.search(r'case\s+GUI_MSG_RUN_FRONTEND\s*:', s)
assert start_m, "RUN_FRONTEND not found"
start = start_m.start()

# Найти конец блока — от start, искать "break;" внутри switch-case
# Проще — найти следующий "case " (за пределами)
after = s[start+10:]
# Найти "break;" после "case GUI_MSG_RUN_FRONTEND"
brk_idx = after.find('break;')
assert brk_idx >= 0, "break not found"
end_pos = start + 10 + brk_idx + len('break;')

# Что вырезаем
block = s[start:end_pos]
print("=== original block ===")
print(block[:200])

# Заменяем на скип
new_block = '''case GUI_MSG_RUN_FRONTEND:
        {
#ifdef RAD_PSP
            // PSP: everything inside this case is broken without proper
            // Scrooby project loading (PNG stub, sprite bypass). Just set
            // state and return. M_pManagerFrontEnd stays null.
            {
                FILE* _f = fopen("ms0:/hitr_force.log", "a");
                if (_f) { fputs("[CGS] PSP: RUN_FRONTEND SKIPPED entirely\\n", _f); fclose(_f); }
            }
            m_state = FRONTEND_ACTIVE;
            break;
#else
            // thaw frontend render layer
            GetRenderManager()->mpLayer(RenderEnums::GUI)->Thaw();

            m_pApp->SetProject( m_pProject );

            m_state = FRONTEND_ACTIVE;

            // start the frontend manager
            rAssert( m_pManagerFrontEnd );
            if( param1 != 0 )
            {
                m_pManagerFrontEnd->Start( static_cast<CGuiWindow::eGuiWindowID>( param1 ) );
            }
            else
            {
                m_pManagerFrontEnd->Start();
            }
            break;
#endif
        }'''

s = s[:start] + new_block + s[end_pos:]
print("RUN_FRONTEND fully skipped")
open(p,"w").write(s)
PY

echo "=== check ==="
sed -n '/case GUI_MSG_RUN_FRONTEND/,/^#endif/p' code/presentation/gui/guisystem.cpp | head -30

echo "=== rebuild ==="
docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build pspdev/pspdev:latest \
  bash -c "rm -f code/CMakeFiles/SRR2.dir/presentation/gui/guisystem.cpp.obj && \
           make SRR2 -j2 2>&1 | tail -12"

docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build/code pspdev/pspdev:latest \
  bash -c "mksfoex -d MEMSIZE=2 'HitR' PARAM.SFO && \
           psp-strip SRR2 -o SRR2_strip.elf && \
           pack-pbp EBOOT.PBP PARAM.SFO NULL NULL NULL NULL NULL SRR2_strip.elf NULL"

ls -la build/code/EBOOT.PBP
rm -f ~/.config/ppsspp/hitr_*.log
echo "DONE — run EBOOT, wait 20s"
