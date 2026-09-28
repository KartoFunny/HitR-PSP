//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        dispatcher.cpp
//
// Subsystem:	Foundation Technologies - Dispatcher
//
// Description:	This file contains the implementation of the Foundation 
//              Technologies Dispatcher. The dispatcher is responsible
//              for controlling the flow of low level events within the system.
//
// Date:    	Mar 12, 2001
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"

#include "dispatcher.hpp"

#include <raddispatch.hpp>
#include <radobject.hpp>
#include <radmemory.hpp>
#include <raddebug.hpp>
#include <radmemorymonitor.hpp>

#ifdef RAD_PSP

#include <pspthreadman.h>
#include <cstdint>
#include <cstdlib>

#ifndef SDL_MAJOR_VERSION
#define SDL_MAJOR_VERSION 3
#endif

// На PSP нет sceKernelCreateMutex — используем семафоры с count=1.
static inline SDL_Mutex* PspCreateMutex(void)
{
    SDL_Mutex* m = (SDL_Mutex*)malloc(sizeof(SDL_Mutex));
    if (!m) return nullptr;
    // sceKernelCreateSema(name, attr, initCount, maxCount, opt)
    m->uid = sceKernelCreateSema("radDispatch", 0, 1, 1, nullptr);
    return m;
}
static inline void PspDestroyMutex(SDL_Mutex* m)
{
    if (!m) return;
    if (m->uid > 0) sceKernelDeleteSema(m->uid);
    free(m);
}
static inline void PspLockMutex(SDL_Mutex* m)
{
    if (m && m->uid > 0) sceKernelWaitSema(m->uid, 1, nullptr);
}
static inline void PspUnlockMutex(SDL_Mutex* m)
{
    if (m && m->uid > 0) sceKernelSignalSema(m->uid, 1);
}

#define SDL_CreateMutex()        PspCreateMutex()
#define SDL_DestroyMutex(m)      PspDestroyMutex((SDL_Mutex*)(m))
#define SDL_LockMutex(m)         PspLockMutex((SDL_Mutex*)(m))
#define SDL_UnlockMutex(m)       PspUnlockMutex((SDL_Mutex*)(m))

#else

#include <SDL.h>

#endif

//=============================================================================
// Local Defintions
//=============================================================================

//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    radDispatchCreate
//=============================================================================
// Description: This is the object factory for the dispatcher.
//
// Parameters:  pIRadDispatcher - returns the object.
//              maxCallback,    - max queued callbacks
//              alloc           - where to get memory
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void radDispatchCreate
( 
    IRadDispatcher**   pIRadDispatcher, 
    unsigned int       maxCallbacks,
    radMemoryAllocator alloc
)
{
    //
    // Simply new up a a dispatcher object.
    //
    *pIRadDispatcher = new( alloc ) radDispatcher( maxCallbacks, alloc );
}


//=============================================================================
// Function:    radDispatcher::radDispatcher
//=============================================================================
// Description: Constructor.Nothing to interesting. Just initialize members.
//
// Parameters:  maxcallbacks
//              allocator
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

radDispatcher::radDispatcher
( 
    unsigned int maxCallbacks,
    radMemoryAllocator alloc    
)
    :
    m_ReferenceCount( 1 ),
    m_MaxEvents( maxCallbacks ),
    m_EventQueueHeadIndex( 0 ),
    m_EventQueueTailIndex( 0 ),
    m_EventsQueued( 0 )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radDispatcher" );
    //
    // Allocate memory to use for queing events.
    //
    m_EventQueue = (Event*) radMemoryAlloc( alloc, sizeof(Event) * m_MaxEvents );

    m_Mutex = SDL_CreateMutex();
}

//=============================================================================
// Function:    fDispatcher::~fDispatcher
//=============================================================================
// Description: Destructor. Free any resources.
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

radDispatcher::~radDispatcher( void )
{
    //
    // If this asserts the caller did not call purge.
    //
    rAssert( m_EventsQueued == 0 );

    SDL_DestroyMutex( m_Mutex );

    //
    // Free up the memory
    //
    radMemoryFree( m_EventQueue );
}

//=============================================================================
// Function:    radDispatcher::AddRef
//=============================================================================

void radDispatcher::AddRef( void )
{
    m_ReferenceCount++;
}

//=============================================================================
// Function:    radDispatcher::Release
//=============================================================================

void radDispatcher::Release( void )
{
    m_ReferenceCount--;
    
    if( m_ReferenceCount == 0 )
    {
       delete this;
    }
}

//=============================================================================
// Function:    radDispatcher::Dump
//=============================================================================

#ifdef RAD_DEBUG

void radDispatcher::Dump( char * pStringBuffer, unsigned int bufferSize )
{
    sprintf( pStringBuffer, "Object: [radDispatcher] At Memory Location:[%p]\n", this );
}

#endif

//=============================================================================
// Function:    fDispatcher::QueueCallback
//=============================================================================

void radDispatcher::QueueCallback
( 
    IRadDispatchCallback* pDispatchCallback,
    void*                 userData 
)
{
    //
    // Update reference count on the dispatch event object since we are holding
    // a pointer to it,
    //      
    pDispatchCallback->AddRef( );

    //
    // Protect the addition of this record to the event list.
    //
    SDL_LockMutex( m_Mutex );

    //
    // Assert that we have not exceeded the maximum number of events in the queue.
    //
    rAssert( m_EventsQueued != m_MaxEvents );                         

    //
    // Add it to the queue at the head.
    //
    m_EventQueue[ m_EventQueueHeadIndex ].m_Callback = pDispatchCallback;
    m_EventQueue[ m_EventQueueHeadIndex ].m_UserData = userData;
    m_EventQueueHeadIndex++;
    if( m_EventQueueHeadIndex == m_MaxEvents )
    {
        m_EventQueueHeadIndex = 0;
    }        
    m_EventsQueued++;

    //
    // Remove protection
    //
    SDL_UnlockMutex( m_Mutex );
}


//=============================================================================
// Function:    radDispatcher::QueueCallbackFromInterrupt
//=============================================================================

void radDispatcher::QueueCallbackFromInterrupt
( 
    IRadDispatchCallback* pDispatchCallback,
    void*                 userData 
)
{
    #if defined ( RAD_WIN32 ) || defined( RAD_XBOX )
    (void) pDispatchCallback;
    (void) userData;
    rAssert( false );
    #endif

    #if defined( RAD_PS2 ) || defined( RAD_GAMECUBE )
    pDispatchCallback->AddRef( );
    rAssert( m_EventsQueued != m_MaxEvents );                         
    m_EventQueue[ m_EventQueueHeadIndex ].m_Callback = pDispatchCallback;
    m_EventQueue[ m_EventQueueHeadIndex ].m_UserData = userData;
    m_EventQueueHeadIndex++;
    if( m_EventQueueHeadIndex == m_MaxEvents )
    {
        m_EventQueueHeadIndex = 0;
    }        
    m_EventsQueued++;
    #endif
    // PSP: interrupt-driven callbacks пока не используются, no-op.
}

//=============================================================================
// Function:    radDispatcher::Service
//=============================================================================

unsigned int radDispatcher::Service( void )
{
    unsigned int eventsToDispatch = m_EventsQueued;

    #ifdef RAD_PS2
    ThreadParam threadInfo;
    ReferThreadStatus( GetThreadId( ), &threadInfo );
    #endif

    SDL_LockMutex( m_Mutex );

    while( (m_EventsQueued != 0) && (eventsToDispatch != 0) )
    {
        Event event = m_EventQueue[ m_EventQueueTailIndex ];
        m_EventQueueTailIndex++;
        if( m_EventQueueTailIndex == m_MaxEvents )
        {
            m_EventQueueTailIndex = 0;
        }        
        m_EventsQueued--;
        eventsToDispatch--;

        SDL_UnlockMutex( m_Mutex );

        event.m_Callback->OnDispatchCallack( event.m_UserData );

        event.m_Callback->Release( );

        #ifdef RAD_PS2
        RotateThreadReadyQueue( threadInfo.currentPriority );
        #endif

        SDL_LockMutex( m_Mutex );
    }

    SDL_UnlockMutex( m_Mutex );

    return( m_EventsQueued );
}
