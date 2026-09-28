//=============================================================================
// Copyright (C) 2002 Radical Entertainment Ltd.  All rights reserved.
//
// File:        
//
// Description: Implement FrontEndContext
//
// History:     21/05/2002 + Created -- NAME
//
//=============================================================================

//========================================
// System Includes
//========================================
// Foundation Tech
#include <raddebug.hpp>

// hitr stripped — empty trace stubs
#define FEStTr(x) ((void)0)

#include <cstdio>

#ifdef RAD_PSP
#include <pspkernel.h>
#include <pspiofilemgr.h>
#endif

//========================================
// Project Includes
//========================================
#include <cheats/cheatinputsystem.h>
#include <contexts/frontendcontext.h>
#include <contexts/contextenum.h>
#include <memory/leakdetection.h>
#include <memory/srrmemory.h>

#include <presentation/gui/guisystem.h>
#include <presentation/gui/frontend/guimanagerfrontend.h>
#include <mission/rewards/rewardsmanager.h>

#include <sound/soundmanager.h>

#include <input/inputmanager.h>

#include <data/gamedatamanager.h>

#include <worldsim/coins/coinmanager.h>

//******************************************************************************
//
// Global Data, Local Data, Local Classes
//
//******************************************************************************

// Static pointer to instance of singleton.
FrontEndContext* FrontEndContext::spInstance = NULL;

//******************************************************************************
//
// Public Member Functions
//
//******************************************************************************

//==============================================================================
// FrontEndContext::GetInstance
//==============================================================================
//
// Description: - Access point for the FrontEndContext singleton.  
//              - Creates the FrontEndContext if needed.
//
// Parameters:	None.
//
// Return:      Pointer to the FrontEndContext.
//
// Constraints: This is a singleton so only one instance is allowed.
//
//==============================================================================
FrontEndContext* FrontEndContext::GetInstance()
{
    if( spInstance == NULL )
    {
        spInstance = new(GMA_PERSISTENT) FrontEndContext;
        rAssert( spInstance );
    }
    
    return spInstance;
}

//==============================================================================
// FrontEndContext::FrontEndContext
//==============================================================================
// Description: Constructor.
//
// Parameters: None.
//
// Return:      N/A.
//
//==============================================================================
FrontEndContext::FrontEndContext()
{
}

//==============================================================================
// FrontEndContext::~FrontEndContext
//==============================================================================
// Description: Destructor.
//
// Parameters: None.
//
// Return:      N/A.
//
//==============================================================================
FrontEndContext::~FrontEndContext()
{
}


//******************************************************************************
//
// Protected Member Functions
//
//******************************************************************************

//=============================================================================
// FrontEndContext::OnStart
//=============================================================================
// Description: Comment
//
// Parameters:  ( ContextEnum previousContext )
//
// Return:      void 
//
//=============================================================================
void FrontEndContext::OnStart( ContextEnum previousContext )
{
#ifdef RAD_PSP
    {
        SceUID fd = sceIoOpen("ms0:/hitr_fe.log",
                              PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0777);
        if(fd>=0){ sceIoWrite(fd,"[FE] OnStart enter\n",19); sceIoClose(fd);}
    }
#endif
    SetMemoryIdentification( "FEContext" );
    MEMTRACK_PUSH_FLAG( "Front End" );

    HeapMgr()->PushHeap( GMA_LEVEL_FE );

    if( previousContext != CONTEXT_BOOTUP )
    {
        HeapMgr()->PrepareHeapsFeCleanup();
        LEAK_DETECTION_CHECKPOINT();
        HeapMgr()->PrepareHeapsFeSetup();

        // reset all cheats
        //
        GetCheatInputSystem()->Reset();

        // unregister controller ID from all players
        //
        GetInputManager()->UnregisterAllControllerID();

        // tell GUI system to run backend during loading
        //
        GetGuiSystem()->HandleMessage( GUI_MSG_RUN_BACKEND );

        // initialize GUI frontend mode (and load resources)
        GetGuiSystem()->HandleMessage( GUI_MSG_INIT_FRONTEND );

        GetLoadingManager()->AddCallback( this );
    }
    else
    {
        // Start the front end.
        LEAK_DETECTION_CHECKPOINT();
#ifdef RAD_PSP
        // PSP: We came from CONTEXT_BOOTUP, but our Bootup never ran GUI init
        // (we skipped movies and license GUI), so m_pManagerFrontEnd is not
        // yet created. Kick off GUI_MSG_INIT_FRONTEND to load the scrooby
        // project and create the FrontEnd manager. StartFrontEnd will happen
        // later from OnProcessRequestsComplete once the project is loaded.
        {
            SceUID fd = sceIoOpen("ms0:/hitr_fe.log",
                                  PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0777);
            if(fd>=0){ sceIoWrite(fd,"[FE] PSP: forcing INIT_FRONTEND\n",30); sceIoClose(fd);}
        }
        GetGuiSystem()->HandleMessage( GUI_MSG_INIT_FRONTEND );
        GetLoadingManager()->AddCallback( this );
#else
        this->StartFrontEnd( CGuiWindow::GUI_SCREEN_ID_SPLASH );
#endif
    }

#ifdef RAD_PSP
    {
        SceUID fd = sceIoOpen("ms0:/hitr_fe.log",
                              PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0777);
        if(fd>=0){ sceIoWrite(fd,"[FE] before ToggleRumble\n",25); sceIoClose(fd);}
    }
#endif
    GetInputManager()->ToggleRumble( false );

#ifdef RAD_PSP
    {
        SceUID fd = sceIoOpen("ms0:/hitr_fe.log",
                              PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0777);
        if(fd>=0){ sceIoWrite(fd,"[FE] before RegisterUserInputHandlers\n",38); sceIoClose(fd);}
    }
#endif
    GetGuiSystem()->RegisterUserInputHandlers();

#ifdef RAD_PSP
    {
        SceUID fd = sceIoOpen("ms0:/hitr_fe.log",
                              PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0777);
        if(fd>=0){ sceIoWrite(fd,"[FE] OnStart END\n",17); sceIoClose(fd);}
    }
#endif
    GetInputManager()->SetGameState( Input::ACTIVE_FRONTEND );
}

//=============================================================================
// FrontEndContext::OnStop
//=============================================================================
// Description: Comment
//
// Parameters:  ( ContextEnum nextContext )
//
// Return:      void 
//
//=============================================================================
void FrontEndContext::OnStop( ContextEnum nextContext )
{
    GetGuiSystem()->UnregisterUserInputHandlers();

    // release GUI frontend
    GetGuiSystem()->HandleMessage( GUI_MSG_RELEASE_FRONTEND );

    //
    // Notify the sound system that the front end is stopping
    //
    GetSoundManager()->OnFrontEndEnd();

    GetInputManager()->SetGameState( Input::ACTIVE_ALL );

    HeapMgr()->PopHeap(GMA_LEVEL_FE);

    MEMTRACK_POP_FLAG( "" );

    SetMemoryIdentification( "FEContext Finished" );
}

//=============================================================================
// FrontEndContext::OnUpdate
//=============================================================================
// Description: Comment
//
// Parameters:  ( unsigned int elapsedTime )
//
// Return:      void 
//
//=============================================================================
void FrontEndContext::OnUpdate( unsigned int elapsedTime )
{
#ifdef RAD_PSP
    // PSP FORCE: after 30 frames, try to start the front end even if the
    // Scrooby async-load never reported completion (which is what we see).
    {
        static int s_force = 0;
        s_force++;
        if (s_force == 30) {
            FILE* f = fopen("ms0:/hitr_force.log", "a");
            if (f) { fputs("[FE] forcing StartFrontEnd at frame 30\n", f); fclose(f); }
            this->StartFrontEnd( CGuiWindow::GUI_SCREEN_ID_SPLASH );
        }
    }
    {
        static int s_count = 0;
        if (++s_count <= 200) {
            SceUID fd = sceIoOpen("ms0:/hitr_fe.log",
                                  PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, 0777);
            if (fd >= 0) {
                char b[80]; int i = 0;
                const char* p = "[FE] OnUpdate ";
                while (p[i]) { b[i] = p[i]; i++; }
                int v = s_count;
                char num[6]; int n = 0;
                while (v > 0) { num[n++] = '0' + (v % 10); v /= 10; }
                for (int j = n-1; j >= 0; j--) b[i++] = num[j];
                b[i++] = '\n';
                sceIoWrite(fd, b, i);
                sceIoClose(fd);
            }
        }
    }
#endif

    // update game data manager
    //
    GetGameDataManager()->Update( elapsedTime );

    // update GUI system
    //
    GetGuiSystem()->Update( elapsedTime );

    //Chuck: adding this so that the rewards manager reflects changes found in the charactersheet.
    GetRewardsManager()->SynchWithCharacterSheet();
}

//=============================================================================
// FrontEndContext::OnSuspend
//=============================================================================
// Description: Comment
//
// Parameters:  ()
//
// Return:      void 
//
//=============================================================================
void FrontEndContext::OnSuspend()
{
}

//=============================================================================
// FrontEndContext::OnResume
//=============================================================================
// Description: Comment
//
// Parameters:  ()
//
// Return:      void 
//
//=============================================================================
void FrontEndContext::OnResume()
{
}

//=============================================================================
// FrontEndContext::OnHandleEvent
//=============================================================================
// Description: Comment
//
// Parameters:  ( EventEnum id, void* pEventData )
//
// Return:      void 
//
//=============================================================================
void FrontEndContext::OnHandleEvent( EventEnum id, void* pEventData )
{
}

//=============================================================================
// FrontEndContext::OnProcessRequestsComplete
//=============================================================================
// Description: Called when startup loading is done
//
// Parameters:  pUserData - unused
//
// Return:      void 
//
//=============================================================================
void FrontEndContext::OnProcessRequestsComplete( void* pUserData )
{
    // tell GUI system to quit backend
    //
    GetGuiSystem()->HandleMessage( GUI_MSG_QUIT_BACKEND );

#ifndef RAD_DEMO
    if( GetGuiSystem()->IsSplashScreenFinished() )
    {
        if( GetGuiSystem()->IsShowCreditsUponReturnToFE() )
        {
            this->StartFrontEnd( CGuiWindow::GUI_SCREEN_ID_VIEW_CREDITS );
        }
        else
        {
            this->StartFrontEnd( CGuiWindow::GUI_SCREEN_ID_MAIN_MENU );
        }
    }
    else
#endif
    {
        this->StartFrontEnd( CGuiWindow::GUI_SCREEN_ID_SPLASH );
    }
}

//******************************************************************************
//
// Private Member Functions
//
//******************************************************************************

//=============================================================================
// FrontEndContext::StartFrontEnd
//=============================================================================
// Description: Comment
//
// Parameters:  ()
//
// Return:      void 
//
//=============================================================================
void FrontEndContext::StartFrontEnd( unsigned int initialScreen )
{
    // Start up GUI frontend manager
    GetGuiSystem()->HandleMessage( GUI_MSG_RUN_FRONTEND, initialScreen );

#ifdef RAD_PSP
    // PSP: SoundManager is a stub returning nullptr; skip.
#else
    //
    // Notify the sound system that the front end is starting
    //
    GetSoundManager()->OnFrontEndStart();
#endif
}

