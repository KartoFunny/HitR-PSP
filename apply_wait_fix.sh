#!/bin/bash
set -e
cd ~/psp/hitr-psp

# 1. radFile::WaitForCompletion — под PSP не зацикливаться
python3 - <<'PY'
p="libs/radcore/src/radfile/common/file.cpp"
s=open(p).read()

old="""void radFile::WaitForCompletion( void )
{
    while ( !CheckForCompletion( ) )
    {
        m_pDrive->Service( );
//        radThreadSleep(0);
    }
}"""
new="""void radFile::WaitForCompletion( void )
{
#ifdef RAD_PSP
    // PSP: all I/O is done inline via radDriveThread::QueueRequest's
    // ProcessRequestsInline. m_pDrive->Service() is a no-op on this platform,
    // so an infinite loop here would deadlock on large files that need
    // multiple read chunks. Just check once and bail.
    if ( !CheckForCompletion() )
    {
        // give the drive one synchronous service attempt then stop
        m_pDrive->Service();
    }
#else
    while ( !CheckForCompletion( ) )
    {
        m_pDrive->Service( );
//        radThreadSleep(0);
    }
#endif
}"""
assert old in s, "WaitForCompletion anchor not found"
s=s.replace(old,new,1)
print("WaitForCompletion fixed")
open(p,"w").write(s)
PY

# 2. Проверим, есть ли у radPspDrive метод Service, и что он делает
echo "=== radPspDrive::Service ==="
grep -n "radPspDrive::Service\|::Service" libs/radcore/src/radfile/psp/pspdrive.cpp libs/radcore/src/radfile/psp/pspdrive.hpp

# 3. Смотрим fileftt.cpp — что там с SwitchTask
echo "=== fileftt.cpp 140-330 ==="
sed -n '140,180p' libs/pure3d/p3d/fileftt.cpp
sed -n '220,330p' libs/pure3d/p3d/fileftt.cpp

docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build pspdev/pspdev:latest \
  bash -c "rm -f libs/radcore/CMakeFiles/radcore.dir/src/radfile/common/file.cpp.obj && \
           make SRR2 -j2 2>&1 | tail -12"

docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build/code pspdev/pspdev:latest \
  bash -c "mksfoex -d MEMSIZE=2 'HitR' PARAM.SFO && \
           psp-strip SRR2 -o SRR2_strip.elf && \
           pack-pbp EBOOT.PBP PARAM.SFO NULL NULL NULL NULL NULL SRR2_strip.elf NULL"

ls -la build/code/EBOOT.PBP
rm -f ~/.config/ppsspp/hitr_*.log
echo "DONE — run EBOOT, wait 20s"
