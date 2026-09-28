#!/bin/bash
set -e
cd ~/psp/hitr-psp

python3 - <<'PY'
p="libs/radcontent/src/radload/manager.cpp"
s=open(p).read()

# 1. Логировать состояние callback-очереди в Service
old="""void radLoadManager::Service()
{
    PspTrLD("[LD] Service enter");"""
new="""void radLoadManager::Service()
{
    PspTrLD("[LD] Service enter");
#ifdef RAD_PSP
    {
        static int s_svc_count = 0;
        s_svc_count++;
        if ((s_svc_count % 5) == 0) {
            SceUID fd = sceIoOpen("ms0:/hitr_service.log", PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0777);
            if (fd >= 0) {
                char b[120]; int i=0;
                const char* m="[SVC] count="; while(*m) b[i++]=*m++;
                int v=s_svc_count; char t[10]; int k=0;
                while(v>0){t[k++]='0'+(v%10);v/=10;}
                while(k>0)b[i++]=t[--k];
                m=" pending="; while(*m) b[i++]=*m++;
                v=IsLoadPending()?1:0; b[i++]='0'+v;
                m=" cbEmpty="; while(*m) b[i++]=*m++;
                v=m_pCallbacks->Empty()?1:0; b[i++]='0'+v;
                m=" qSize="; while(*m) b[i++]=*m++;
                v=m_pLoadQueue->Size(); char t2[10]; int k2=0;
                if(v==0){t2[k2++]='0';} else {while(v>0){t2[k2++]='0'+(v%10);v/=10;}}
                while(k2>0)b[i++]=t2[--k2];
                b[i++]='\\n';
                sceIoWrite(fd, b, i); sceIoClose(fd);
            }
        }
    }
#endif"""
assert old in s, "Service anchor not found"
s=s.replace(old,new,1)
print("Service counters added")

# 2. Счётчик в InternalService
old2="""void radLoadManager::InternalService()
{
#ifdef RAD_PSP
    // PSP: one-shot — pop at most one item and return.
    // Called every frame from Service(), so queue drains over time.
    if( m_pLoadQueue->Empty() )
    {
        return;
    }"""
new2="""void radLoadManager::InternalService()
{
#ifdef RAD_PSP
    {
        static int s_is_count = 0;
        s_is_count++;
        if ((s_is_count % 20) == 0) {
            SceUID fd = sceIoOpen("ms0:/hitr_service.log", PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0777);
            if (fd >= 0) {
                char b[64]; int i=0;
                const char* m="[IS] n="; while(*m) b[i++]=*m++;
                int v=s_is_count; char t[10]; int k=0;
                while(v>0){t[k++]='0'+(v%10);v/=10;}
                while(k>0)b[i++]=t[--k];
                b[i++]='\\n';
                sceIoWrite(fd, b, i); sceIoClose(fd);
            }
        }
    }
    // PSP: one-shot — pop at most one item and return.
    // Called every frame from Service(), so queue drains over time.
    if( m_pLoadQueue->Empty() )
    {
        return;
    }"""
assert old2 in s, "InternalService anchor not found"
s=s.replace(old2,new2,1)
print("InternalService counter added")

# 3. Счётчик в Load
old3="""void radLoadManager::Load( radLoadOptions* options, radLoadRequest** request )
{
    PspTrLD("[LD] Load enter");"""
new3="""void radLoadManager::Load( radLoadOptions* options, radLoadRequest** request )
{
    PspTrLD("[LD] Load enter");
#ifdef RAD_PSP
    {
        static int s_ld_count = 0;
        s_ld_count++;
        if ((s_ld_count % 5) == 0) {
            SceUID fd = sceIoOpen("ms0:/hitr_service.log", PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0777);
            if (fd >= 0) {
                char b[80]; int i=0;
                const char* m="[LOAD] n="; while(*m) b[i++]=*m++;
                int v=s_ld_count; char t[10]; int k=0;
                while(v>0){t[k++]='0'+(v%10);v/=10;}
                while(k>0)b[i++]=t[--k];
                m=" q="; while(*m) b[i++]=*m++;
                v=m_pLoadQueue->Size(); char t2[10]; int k2=0;
                if(v==0){t2[k2++]='0';} else {while(v>0){t2[k2++]='0'+(v%10);v/=10;}}
                while(k2>0)b[i++]=t2[--k2];
                b[i++]='\\n';
                sceIoWrite(fd, b, i); sceIoClose(fd);
            }
        }
    }
#endif"""
assert old3 in s, "Load anchor not found"
s=s.replace(old3,new3,1)
print("Load counter added")

open(p,"w").write(s)
PY

docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build pspdev/pspdev:latest \
  bash -c "rm -f libs/radcontent/CMakeFiles/radcontent.dir/src/radload/manager.cpp.obj && \
           make SRR2 -j2 2>&1 | tail -12"

docker run --rm -v $(pwd)/..:/work -w /work/hitr-psp/build/code pspdev/pspdev:latest \
  bash -c "mksfoex -d MEMSIZE=2 'HitR' PARAM.SFO && \
           psp-strip SRR2 -o SRR2_strip.elf && \
           pack-pbp EBOOT.PBP PARAM.SFO NULL NULL NULL NULL NULL SRR2_strip.elf NULL"

ls -la build/code/EBOOT.PBP
rm -f ~/.config/ppsspp/hitr_*.log
echo "DONE — run EBOOT, wait 15s"
