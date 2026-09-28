#!/bin/bash
set -e
cd ~/psp/hitr-psp

python3 - <<'PY'
p="libs/radcontent/src/radload/manager.cpp"
s=open(p).read()

old="""void radLoadManager::Service()
{
    PspTrLD("[LD] Service enter");
#ifdef RAD_PSP
    // PSP: background LoadThread does not run — do the work inline.
    if( IsLoadPending() )
    {
        InternalService();
    }
#else"""

new="""void radLoadManager::Service()
{
    PspTrLD("[LD] Service enter");
#ifdef RAD_PSP
    // PSP: reentrancy guard — the callback->Done() path can re-enter
    // Service() through tFileFTT::WaitForCompletion -> SwitchTask.
    // We only allow one level of Service(); nested calls return immediately.
    static int s_depth = 0;
    if (s_depth > 0) return;
    s_depth++;
    if( IsLoadPending() )
    {
        InternalService();
    }
#else"""

assert old in s, "Service anchor not found"
s=s.replace(old,new,1)
print("Service reentrancy guard added")

# Закрыть s_depth-- в конце функции
old2="""    if(!m_pCallbacks->Empty())
    {
        radLoadCallback* callback = m_pCallbacks->Pop();
        callback->Done();
        callback->Release();
    }


}"""
new2="""    if(!m_pCallbacks->Empty())
    {
        radLoadCallback* callback = m_pCallbacks->Pop();
        callback->Done();
        callback->Release();
    }

#ifdef RAD_PSP
    s_depth--;
#endif
}"""
if old2 in s:
    s=s.replace(old2,new2,1)
    print("s_depth-- added")
else:
    # Попробуем более общий anchor
    old3="""    if(!m_pCallbacks->Empty())
    {
        radLoadCallback* callback = m_pCallbacks->Pop();
        callback->Done();
        callback->Release();
    }
}"""
    if old3 in s:
        s=s.replace(old3, new2, 1)
        print("s_depth-- added (variant)")

open(p,"w").write(s)
PY

echo "=== Проверка, что патч применился ==="
sed -n '/void radLoadManager::Service/,/^}/p' libs/radcontent/src/radload/manager.cpp | head -40

echo
echo "=== Rebuild ==="
docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build pspdev/pspdev:latest \
  bash -c "rm -f libs/radcontent/CMakeFiles/radcontent.dir/src/radload/manager.cpp.obj && \
           make SRR2 -j2 2>&1 | tail -10"

echo
echo "=== Pack ==="
docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build/code pspdev/pspdev:latest \
  bash -c "mksfoex -d MEMSIZE=2 'HitR' PARAM.SFO && \
           psp-strip SRR2 -o SRR2_strip.elf && \
           pack-pbp EBOOT.PBP PARAM.SFO NULL NULL NULL NULL NULL SRR2_strip.elf NULL"

ls -la build/code/EBOOT.PBP build/code/SRR2
rm -f ~/.config/ppsspp/hitr_*.log
echo "DONE — run EBOOT, wait 20s"
