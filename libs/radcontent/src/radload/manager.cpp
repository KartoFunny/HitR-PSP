//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================

#include <radload/manager.hpp>


// hitr_trace_helper_ld
#ifdef RAD_PSP
#include <pspiofilemgr.h>
#include <pspkernel.h>
static void PspTrLD(const char* tag) {
    static int cnt = 0; if (cnt > 500) return; cnt++;
    SceUID fd = sceIoOpen("ms0:/hitr_trace.log", PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0777);
    if (fd < 0) return;
    int n=0; while(tag[n]) n++;
    sceIoWrite(fd, tag, n); sceIoWrite(fd, "\n", 1); sceIoClose(fd);
}
#else
#define PspTrLD(x) ((void)0)
#endif
#include <string.h>
#include <radload/utility/hashtable.hpp>
#include <radload/utility/queue.hpp>
#include <radfile.hpp>

#ifdef RADLOAD_USE_WATCHER
#include <raddebugwatch.hpp>
#endif

radLoadManagerWrapper radLoad;

ILoadManager* ILoadManager::s_instance = NULL;

void radLoadInitialize( radLoadInit* init )
{
    if( !init )
    {
        radMemoryAllocator old = ::radMemorySetCurrentAllocator( RADMEMORY_ALLOC_TEMP );
        init = new radLoadInit();
        ::radMemorySetCurrentAllocator( old );
    }
    ILoadManager::s_instance = new radLoadManager( *init );
    delete init;
}

void radLoadTerminate()
{
    radLoad->Terminate();
    ILoadManager::s_instance = NULL;
}

void radLoadService()
{
    radLoad->Service();
}

ILoadManager* radLoadInstance()
{
    return ILoadManager::s_instance;
}

radLoadManager::radLoadManager( radLoadInit& init )
:
m_bSyncLoading( false ),
m_bDone( false ),
#ifdef RADLOAD_GATHER_STATS
m_totalLoads( 0 ),
m_completedLoads( 0 ),
m_pendingLoads( 0 ),
m_maxPendingLoads( 0 ),
m_minLoadTime( 0xFFFFFFFF ),
m_maxLoadTime( 0 ),
m_avgLoadTime( 0 ),
m_minQueuedTime( 0xFFFFFFFF ),
m_maxQueuedTime( 0 ),
m_avgQueuedTime( 0 ),
#endif
m_pCurrent( NULL ),
m_pFileLoaders( NULL ),
m_pDataLoaders( NULL ),
m_pLoadQueue( NULL ),
m_pThread( NULL ),
m_pMutex( NULL )
{
    // Make our hash tables so that they will never resize or repack
    m_pFileLoaders = new RefHashTable<radLoadFileLoader>( init.fileLoaderListSize, 200, init.fileLoaderListSize );
    m_pFileLoaders->AddRef();
    m_pDataLoaders = new RefHashTable<radLoadDataLoader>( init.dataLoaderListSize, 200, init.dataLoaderListSize );
    m_pDataLoaders->AddRef();
    m_pLoadQueue = new RefQueue<radLoadObject>( init.loadQueueSize );
    m_pLoadQueue->AddRef();
    m_pCallbacks = new RefQueue<radLoadCallback>( 32 );
    m_pCallbacks ->AddRef();

    ::radThreadCreateMutex( &m_pMutex );
#ifndef RAD_PSP
    m_pMutex->Lock();
#endif
    ::radThreadCreateThread( &m_pThread, radLoadManager::LoadThreadEntry, static_cast<void*>(this), IRadThread::PriorityNormal, init.loadThreadStackSize );

#ifdef RADLOAD_GATHER_STATS
#ifdef RADLOAD_USE_WATCHER
    radDbgWatchAddUnsignedInt( &m_totalLoads, "Total Loads", "radload\\stats", 0, 0 );
    radDbgWatchAddUnsignedInt( &m_completedLoads, "Completed Loads", "radload\\stats", 0, 0 );
    radDbgWatchAddUnsignedInt( &m_pendingLoads, "Pending Loads", "radload\\stats", 0, 0 );
    radDbgWatchAddUnsignedInt( &m_maxPendingLoads, "Max Pending Loads", "radload\\stats", 0, 0 );
    radDbgWatchAddUnsignedInt( &m_minLoadTime, "Min Load Time", "radload\\stats", 0, 0 );
    radDbgWatchAddUnsignedInt( &m_maxLoadTime, "Max Load Time", "radload\\stats", 0, 0 );
    radDbgWatchAddUnsignedInt( &m_avgLoadTime, "Average Load Time", "radload\\stats", 0, 0 );
    radDbgWatchAddUnsignedInt( &m_minQueuedTime, "Min Queued Time", "radload\\stats", 0, 0 );
    radDbgWatchAddUnsignedInt( &m_maxQueuedTime, "Max Queued Time", "radload\\stats", 0, 0 );
    radDbgWatchAddUnsignedInt( &m_avgQueuedTime, "Average Queued Time", "radload\\stats", 0, 0 );
#endif
#endif
}

radLoadManager::~radLoadManager()
{
    // Destruction of objects happens in the Terminate function
}

void radLoadManager::AddCallback( radLoadCallback* callback )
{
    if( callback )
    {
        callback->AddRef();
        if( !IsLoadPending() )
        {
            callback->Done();
            callback->Release();
        }
        else
        {
            m_pLoadQueue->Push( callback );
        }
    }
}

void radLoadManager::AddDataLoader( radLoadDataLoader* dataLoader, radLoadClassID id )
{
    m_pDataLoaders->Store( id, dataLoader );
}

void radLoadManager::AddFileLoader( radLoadFileLoader* fileLoader, const char* extension )
{
    m_pFileLoaders->Store( radMakeCaseInsensitiveKey( extension ), fileLoader );
}

void radLoadManager::Cancel()
{
    unsigned int i = 0;

    while( !m_pLoadQueue->Empty() )
    {
       radLoadObject* obj = m_pLoadQueue->Pop();
       radLoadUpdatableRequest* req = dynamic_cast<radLoadUpdatableRequest*>( obj );
       if( req )
       {
           req->SetState( CANCELED );
       }
       obj->Release();
    }
    if( m_pCurrent )
    {
        m_pCurrent->Cancel();
    }
}

radLoadDataLoader* radLoadManager::GetDataLoader( radLoadClassID id )
{
    return m_pDataLoaders->Find( id );
}

radLoadFileLoader* radLoadManager::GetFileLoader( const char* extension )
{
    const char* ext = extension;
    if( *ext == '.' )
    {
        ext++;
    }
    return m_pFileLoaders->Find( radMakeCaseInsensitiveKey( ext ) );
}

void radLoadManager::InternalService()
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
                b[i++]='\n';
                sceIoWrite(fd, b, i); sceIoClose(fd);
            }
        }
    }
    // PSP: one-shot — pop at most one item and return.
    // Called every frame from Service(), so queue drains over time.
    if( m_pLoadQueue->Empty() )
    {
        return;
    }

    radLoadObject* obj = m_pLoadQueue->Pop();
    if( !obj ) return;

    radLoadCallback* callback = dynamic_cast<radLoadCallback*>( obj );
    if( callback )
    {
        m_pCallbacks->Push( callback );
        return;
    }

    QueueItem* item = dynamic_cast<QueueItem*>(obj);
    if( !item ) return;

    m_pCurrent = item;
    m_pCurrent->AddRef();
    m_pCurrent->SetState( LOADING );

    char* filename = item->GetOptions()->filename;

    int i = strlen( filename ) - 1;
    while( i && (filename[i] != '.') )
    {
        i--;
    }
    i++;

    radLoadFileLoader* loader = m_pFileLoaders->Find( radMakeCaseInsensitiveKey( filename + i ) );
    if( !loader )
    {
        PspTrLD("[LD] ERROR: no loader for extension");
        PspTrLD( filename + i );
        if( m_pCurrent->GetState() == LOADING )
        {
            m_pCurrent->SetState( COMPLETE );
        }
        radLoadObject::Release( m_pCurrent );
        m_pCurrent = NULL;
        return;
    }

    radMemoryAllocator oldAlloc = ::radMemorySetCurrentAllocator( item->GetOptions()->allocator );
    PspTrLD("[LD] before loader->LoadFile");
    loader->LoadFile( item->GetOptions(), static_cast<radLoadUpdatableRequest*>( item ) );
    PspTrLD("[LD] after loader->LoadFile");
    ::radMemorySetCurrentAllocator( oldAlloc );

    if( m_pCurrent->GetState() == LOADING )
    {
        m_pCurrent->SetState( COMPLETE );
    }
    radLoadObject::Release( m_pCurrent );
    m_pCurrent = NULL;
#else
    m_pMutex->Lock();
    while( !m_bDone )
    {
        if( !m_pLoadQueue->Empty() )
        {
            radLoadObject* obj = m_pLoadQueue->Pop();
            radLoadCallback* callback = dynamic_cast<radLoadCallback*>( obj );
            if( callback )
            {
                m_pCallbacks->Push(callback);
            }
            else
            {
                QueueItem* item = dynamic_cast<QueueItem*>(obj);
                if( item )
                {
                    m_pCurrent = item;
                    m_pCurrent->AddRef();
                    m_pCurrent->SetState( LOADING );

                    char* filename = item->GetOptions()->filename;
                    int i = strlen( filename ) - 1;
                    while( i && (filename[i] != '.') )
                    {
                        i--;
                    }
                    i++;

                    radLoadFileLoader* loader = m_pFileLoaders->Find( radMakeCaseInsensitiveKey( filename + i ) );
                    rAssert( loader );
                    radMemoryAllocator old = ::radMemorySetCurrentAllocator (item->GetOptions()->allocator);
                    loader->LoadFile( item->GetOptions(), static_cast<radLoadUpdatableRequest*>( item ) );
                    ::radMemorySetCurrentAllocator (old);
                    if( m_pCurrent->GetState() == LOADING )
                    {
                        m_pCurrent->SetState( COMPLETE );
                    }
                    radLoadObject::Release( m_pCurrent );
                }
            }
        }
        else
        {
            SwitchTasks();
        }
    }
    m_pMutex->Unlock();
#endif
}


bool radLoadManager::IsLoadPending()
{
    return (!m_pLoadQueue->Empty() || m_pCurrent);
}

bool radLoadManager::IsSyncLoading()
{
    return m_bSyncLoading;
}

void radLoadManager::Load( radLoadOptions* options, radLoadRequest** request )
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
                b[i++]='\n';
                sceIoWrite(fd, b, i); sceIoClose(fd);
            }
        }
    }
#endif
#ifdef RADLOAD_GATHER_STATS
    m_totalLoads++;
    m_pendingLoads++;
    m_maxPendingLoads = (m_pendingLoads > m_maxPendingLoads) ? m_pendingLoads : m_maxPendingLoads;
#endif
    rAssert( options );
    rAssert( options->filename );
#ifdef RAD_PSP
    // PSP: syncLoad would deadlock: loader->LoadFile eventually calls back into
    // Load() recursively, and the inner sync-loop's InternalService has an
    // already-empty queue, so item never transitions to COMPLETE.
    // Force async — Service() is pumped every frame and drains the queue.
    options->syncLoad = false;
#else
    options->syncLoad |= m_bSyncLoading;
#endif
    radMemoryAllocator old = ::radMemorySetCurrentAllocator( RADMEMORY_ALLOC_TEMP );
    QueueItem* item = new QueueItem( *options );
    ::radMemorySetCurrentAllocator( old );
    item->AddRef();
    m_pLoadQueue->Push( item );
    item->SetState( QUEUED );
    *request = static_cast<radLoadRequest*>(item);
    if( options->stream )
    {
        item->SetStream( options->stream );
    }
#ifndef RAD_PSP
    if( options->syncLoad )
    {
        while( item->GetState() != COMPLETE )
        {
            SwitchTasks();
            radFileService();
        }
    }
#endif
}

void radLoadManager::Load( const char* filename, radLoadRequest** request )
{
    radLoadOptions options;
    options.filename = new char[strlen( filename )];
    strcpy( options.filename, filename );
    Load( &options, request );
}

unsigned int radLoadManager::LoadThreadEntry( void* data )
{
    PspTrLD("[LD] LoadThreadEntry enter");
    radLoadManager* manager = static_cast<radLoadManager*>( data );
    manager->InternalService();
    return 0;
}

float radLoadManager::PercentDone()
{
    if( m_pLoadQueue->Empty() )
    {
        return 1.0f;
    }
    // While hardly the most accurate number out there, it'll work for now.
    return 1.0f / static_cast<float>(m_pLoadQueue->Size());
}

void radLoadManager::PrintStats()
{
#ifdef RADLOAD_GATHER_STATS
    rDebugPrintf( "RadLoad Stats\n" );
    rDebugPrintf( "=====================================\n" );
    rDebugPrintf( "\tTotal Loads : %d\n", m_totalLoads );
    rDebugPrintf( "\tCompleted Loads : %d\n", m_completedLoads );
    rDebugPrintf( "\tPending Loads : %d\n", m_pendingLoads );
    rDebugPrintf( "\tMax Pending Loads : %d\n", m_maxPendingLoads );
    rDebugPrintf( "\tMin Load Time : %d\n", m_minLoadTime );
    rDebugPrintf( "\tMax Load Time : %d\n", m_maxLoadTime );
    rDebugPrintf( "\tAverage Load Time : %d\n", m_avgLoadTime );
    rDebugPrintf( "\tMin Queued Time : %d\n", m_minQueuedTime );
    rDebugPrintf( "\tMax Queued Time : %d\n", m_maxQueuedTime );
    rDebugPrintf( "\tAverage Queued Time : %d\n", m_avgQueuedTime );
    rDebugPrintf( "=====================================\n" );
#endif
}

void radLoadManager::RemoveDataLoader( radLoadClassID id )
{
    radLoadDataLoader* loader = GetDataLoader( id );
    while( loader )
    {
        RemoveDataLoader( loader );
        loader = GetDataLoader( id );
    }
}

void radLoadManager::RemoveDataLoader( radLoadDataLoader* loader )
{
    m_pDataLoaders->Remove( loader );
}

void radLoadManager::RemoveFileLoader( const char* extension )
{
    radLoadFileLoader* loader = GetFileLoader( extension );
    while( loader )
    {
        RemoveFileLoader( loader );
        loader = GetFileLoader( extension );
    }
}

void radLoadManager::RemoveFileLoader( radLoadFileLoader* loader )
{
    m_pFileLoaders->Remove( loader );
}

void radLoadManager::Service()
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
                b[i++]='\n';
                sceIoWrite(fd, b, i); sceIoClose(fd);
            }
        }
    }
#endif
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
#else
    if( IsLoadPending() )
    {
        SwitchTasks();
    }
#endif

    if(!m_pCallbacks->Empty())
    {
        radLoadCallback* callback = m_pCallbacks->Pop();
        callback->Done();
        callback->Release();
    }

#ifdef RAD_PSP
    s_depth--;
#endif
}

void radLoadManager::SetSyncLoading( bool sync )
{
    m_bSyncLoading = sync;
}

void radLoadManager::SwitchTasks()
{
    PspTrLD("[LD] SwitchTasks");
#ifdef RAD_PSP
    // PSP: single-threaded — just yield, no mutex ops.
    radThreadSleep(0);
#else
    m_pMutex->Unlock();
    radThreadSleep(0);
    m_pMutex->Lock();
#endif
}

void radLoadManager::Terminate()
{
    m_pFileLoaders->Release();
    m_pDataLoaders->Release();
    rAssert( m_pLoadQueue->Empty() );
    m_pLoadQueue->Release();
    m_pCallbacks->Release();
    m_bDone = true;
    #ifndef RAD_PSP
    m_pMutex->Unlock();
#endif
    m_pThread->WaitForTermination();
    m_pThread->Release();

    m_pMutex->Release();

    delete this;
}
    
