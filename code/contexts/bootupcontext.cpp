//=============================================================================
// Copyright (C) 2002 Radical Entertainment Ltd.  All rights reserved.
//
// File:        bootupcontext.cpp
//
// Description: Implementation of BootupContext.
//
// History:     + Created -- Darwin Chau
//
//=============================================================================

// COMPORTAMIENTO ORIGINAL CINEMATICAS
/*
#ifdef RAD_RELEASE
    #ifndef RAD_E3
        #define SHOW_MOVIES
    #endif
#endif

*/


// COMO EL ARRANQUE DE CINEMATICAS INICIALES DA CRASH EN ANDROID POR AHORA LO SALTAMOS
#if defined(RAD_RELEASE) && !defined(RAD_E3) && !defined(RAD_ANDROID)
    #define SHOW_MOVIES
#endif


//========================================
// System Includes
//========================================
#include <raddebug.hpp>

#include <radtime.hpp>
#include <raddebugwatch.hpp>
#include <radmovie2.hpp>
#include <p3d/utility.hpp>
#include <p3d/context.hpp>
#include <pddi/pddi.hpp>


#ifdef RAD_PSP
#include <pspiofilemgr.h>
#include <cstdarg>
#include <radfile.hpp>
static void BLOG(const char* fmt, ...) {
    SceUID fd = sceIoOpen("ms0:/hitr_bootctx.log",
                          PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, 0777);
    if (fd < 0) return;
    char buf[256];
    va_list a; va_start(a, fmt);
    vsnprintf(buf, sizeof(buf), fmt, a);
    va_end(a);
    int len = 0; while (buf[len]) len++;
    sceIoWrite(fd, buf, len);
    sceIoClose(fd);
}
#define BLOG_M(x) BLOG(x "\n")
#else
#define BLOG_M(x) ((void)0)
#endif


//========================================
// Project Includes
//========================================
#include <contexts/bootupcontext.h>

#include <atc/atcmanager.h>
#include <cards/cardgallery.h>
#include <cheats/cheatinputsystem.h>
#include <constants/movienames.h>
#include <contexts/contextenum.h>
#include <data/gamedatamanager.h>
#include <data/memcard/memorycardmanager.h>
#include <gameflow/gameflow.h>
#include <input/inputmanager.h>
#include <interiors/interiormanager.h>
#include <loading/loadingmanager.h>
#include <main/commandlineoptions.h>
#include <memory/leakdetection.h>
#include <memory/srrmemory.h>
#include <mission/gameplaymanager.h>
#include <mission/missionmanager.h>
#include <mission/charactersheet/charactersheetmanager.h>
#include <mission/rewards/rewardsmanager.h>
#include <presentation/presentation.h>
#include <presentation/presevents/presentationevent.h>
#include <presentation/gui/guisystem.h>
#include <presentation/tutorialmanager.h>
#include <render/RenderManager/RenderManager.h>
#include <render/RenderManager/RenderLayer.h>
#include <sound/soundmanager.h>
#include <supersprint/supersprintmanager.h>
#include <worldsim/worldphysicsmanager.h>
#include <worldsim/character/charactermanager.h>

#ifdef RAD_GAMECUBE
    #include <main/gamecube_extras/gcmanager.h>
#endif

#ifdef RAD_PC
    #include <main/win32platform.h>
    #include <data/config/gameconfigmanager.h>

#endif

//******************************************************************************
//
// Global Data, Local Data, Local Classes
//
//******************************************************************************

// Static pointer to instance of singleton.
BootupContext* BootupContext::spInstance = NULL;

#ifdef RAD_RELEASE
    #ifdef RAD_PS2
        // TC: Edwin (Singh) says that the PS2 TRC requires that the license screen
        //     be displayed for at least 5 seconds.
        //
        const int MINIMUM_LICENSE_SCREEN_DISPLAY_TIME = 5000; // in msec
    #else
        const int MINIMUM_LICENSE_SCREEN_DISPLAY_TIME = 1000; // in msec
    #endif
#else
    const int MINIMUM_LICENSE_SCREEN_DISPLAY_TIME = 1000; // in msec
#endif

//******************************************************************************
//
// Public Member Functions
//
//******************************************************************************

//==============================================================================
// BootupContext::GetInstance
//==============================================================================
//
// Description: - Access point for the BootupContext singleton.  
//              - Creates the BootupContext if needed.
//
// Parameters:	None.
//
// Return:      Pointer to the BootupContext.
//
// Constraints: This is a singleton so only one instance is allowed.
//
//==============================================================================
BootupContext* BootupContext::GetInstance()
{
    if( spInstance == NULL )
    {
        spInstance = new(GMA_PERSISTENT) BootupContext;
        rAssert( spInstance );
    }
    
    return spInstance;
}

//=============================================================================
// BootupContext::StartMovies
//=============================================================================
// Description: Comment
//
// Parameters:  Game Mode (Frontend/In-Game)
//
// Return:      void 
//
//=============================================================================
void BootupContext::StartMovies()
{
#ifndef FINAL
    if( CommandLineOptions::Get( CLO_SKIP_FE ) )
    {
        // enable 'unlock missions' cheat for 'skipfe' commandline option
        //
        GetCheatInputSystem()->SetCheatEnabled( CHEAT_ID_UNLOCK_MISSIONS, true );

        short levelIndex = CommandLineOptions::GetDefaultLevel();

        if( levelIndex == -1 ) // L0 = minigame
        {
            SetGameplayManager( SuperSprintManager::GetInstance() );

            // skip FE and go to mini-game
            //
            GetGameFlow()->SetContext( CONTEXT_SUPERSPRINT_FE );
        }
        else
        {
            SetGameplayManager( MissionManager::GetInstance() );

            // register controller ID [0] for player [0], by default
            //
            GetInputManager()->RegisterControllerID( 0, 0 );

            short missionIndex = CommandLineOptions::GetDefaultMission();
            if( levelIndex == RenderEnums::L1 )
            {
                // special case for level 1 due to tutorial mission being mission 0
                //
                missionIndex++;
            }

            // set level and mission to load for normal gameplay
            //
            GetGameplayManager()->SetLevelIndex( static_cast< RenderEnums::LevelEnum >( levelIndex ) );
            GetGameplayManager()->SetMissionIndex( static_cast< RenderEnums::MissionEnum >( missionIndex ) );

            // skip FE and go to normal gameplay
            //
            GetGameFlow()->SetContext( CONTEXT_LOADING_GAMEPLAY );
        }
    }
    else
#endif // !FINAL
    {
#ifdef SHOW_MOVIES
        if( CommandLineOptions::Get( CLO_SKIP_MOVIE ) )
        {
            // Switch to frontend context.
            GetGameFlow()->SetContext( CONTEXT_FRONTEND );
        }
        else
        {
            FMVEvent* pEvent = 0;

            GetPresentationManager()->QueueFMV( &pEvent, this );
            strcpy( pEvent->fileName, MovieNames::VUGLOGO );
            pEvent->SetRenderLayer( RenderEnums::PresentationSlot );
            pEvent->SetAutoPlay( true );
            pEvent->SetAudioIndex( FMVEvent::AUDIO_INDEX_ENGLISH );
            pEvent->SetClearWhenDone( true );

            GetPresentationManager()->QueueFMV( &pEvent, this );
            strcpy( pEvent->fileName, MovieNames::FOXLOGO );
            pEvent->SetRenderLayer( RenderEnums::PresentationSlot );
            pEvent->SetAutoPlay( true );
            pEvent->SetAudioIndex( FMVEvent::AUDIO_INDEX_ENGLISH );
            pEvent->SetClearWhenDone( true );

			GetPresentationManager()->QueueFMV( &pEvent, this );
            strcpy( pEvent->fileName, MovieNames::GRACIELOGO );
            pEvent->SetRenderLayer( RenderEnums::PresentationSlot );
            pEvent->SetAutoPlay( true );
            pEvent->SetAudioIndex( FMVEvent::AUDIO_INDEX_ENGLISH );
            pEvent->SetClearWhenDone( true );

            GetPresentationManager()->QueueFMV( &pEvent, this );
            strcpy( pEvent->fileName, MovieNames::RADICALLOGO );
            pEvent->SetRenderLayer( RenderEnums::PresentationSlot );
            pEvent->SetAutoPlay( true );
            pEvent->SetAudioIndex( FMVEvent::AUDIO_INDEX_ENGLISH );
            pEvent->SetClearWhenDone( true );

            GetRenderManager()->mpLayer( RenderEnums::GUI )->Chill();
			
        }
#else
        // Switch to frontend context.
        GetGameFlow()->SetContext( CONTEXT_FRONTEND );
#endif
    }
}

void
BootupContext::StartLoadingSound()
{
    GetSoundManager()->OnBootupStart();

    GetLoadingManager()->AddCallback( this, (void*)GetSoundManager() );
}

#ifdef RAD_PC
void BootupContext::LoadConfig()
{
    // Load the config file for the game.
    GameConfigManager* gc = GetGameConfigManager();
    bool success = gc->LoadConfigFile();

    // If we couldn't load the config file, create a new one.
    if( !success )
    {
#ifdef RAD_PC
        Win32Platform::GetInstance()->LoadDefaults();
#endif
        gc->SaveConfigFile();
    }
}
#endif

//******************************************************************************
//
// Protected Member Functions
//
//******************************************************************************

//==============================================================================
// BootupContext::OnStart
//==============================================================================
//
// Description: 
//
// Parameters:  
//
// Return:      
//
//==============================================================================
void BootupContext::OnStart( ContextEnum previousContext )
{
    BLOG_M("[BC] OnStart A: enter");

    SetMemoryIdentification( "BootupContext" );
    HeapMgr()->PrepareHeapsFeCleanup();
    HeapMgr()->PrepareHeapsFeSetup();
    HeapMgr()->PushHeap (GMA_PERSISTENT);
#ifdef DEBUGINFO_ENABLED
    DebugInfo::InitializeStaticVariables();
#endif

    MEMTRACK_PUSH_FLAG( "Bootup" );

    BLOG_M("[BC] OnStart B: before GameDataManager");
    GetGameDataManager()->Init();
    BLOG_M("[BC] OnStart C: after GameDataManager");

#ifdef RAD_PS2
    if( !CommandLineOptions::Get( CLO_SKIP_MEMCHECK ) )
    {
        GetMemoryCardManager()->LoadMemcardInfo();
    }
#endif

    BLOG_M("[BC] OnStart D: before GuiSystem");
#ifndef RAD_PSP
    GetGuiSystem()->Init();
    GetGuiSystem()->RegisterUserInputHandlers();
#endif
    BLOG_M("[BC] OnStart E: after GuiSystem");

    BLOG_M("[BC] OnStart F: before CardGallery");
#ifndef RAD_PSP
    GetCardGallery()->Init();
#endif
    BLOG_M("[BC] OnStart G: after CardGallery");

    BLOG_M("[BC] OnStart H: before CheatInput");
#ifndef RAD_PSP
    GetCheatInputSystem()->Init();
#endif
    BLOG_M("[BC] OnStart I: after CheatInput");

    BLOG_M("[BC] OnStart J: before Tutorial");
#ifndef RAD_PSP
    GetTutorialManager()->Initialize();
#endif
    BLOG_M("[BC] OnStart K: after Tutorial");

    BLOG_M("[BC] OnStart L: before ATC");
#ifndef RAD_PSP
    GetATCManager()->Init();
#endif
    BLOG_M("[BC] OnStart M: after ATC");

    BLOG_M("[BC] OnStart N: before CharacterSheet");
#ifndef RAD_PSP
    GetCharacterSheetManager()->InitCharacterSheet();
#endif
    BLOG_M("[BC] OnStart O: after CharacterSheet");

    BLOG_M("[BC] OnStart P: before Presentation");
#ifndef RAD_PSP
    GetPresentationManager()->InitializePlayerDrawable();
#endif
    BLOG_M("[BC] OnStart Q: after Presentation");

    BLOG_M("[BC] OnStart R: before WorldPhysics");
#ifndef RAD_PSP
    GetWorldPhysicsManager()->Init();
#endif
    BLOG_M("[BC] OnStart S: after WorldPhysics");

    BLOG_M("[BC] OnStart T: before Interior");
#ifndef RAD_PSP
    GetInteriorManager()->OnBootupStart();
#endif
    BLOG_M("[BC] OnStart U: after Interior");

    BLOG_M("[BC] OnStart V: before CharacterManager::PreLoad");
#ifndef RAD_PSP
    GetCharacterManager()->PreLoad();
#endif
    BLOG_M("[BC] OnStart W: after CharacterManager");

    BLOG_M("[BC] OnStart X: before RewardsManager::LoadScript");
#ifndef RAD_PSP
    GetRewardsManager()->LoadScript();
#endif
    BLOG_M("[BC] OnStart Y: after RewardsManager");

    BLOG_M("[BC] OnStart Z: before LoadingManager requests");
    GetLoadingManager()->AddRequest( FILEHANDLER_PURE3D, "art\\cars\\common.p3d", GMA_DEFAULT, "Global" );
    GetLoadingManager()->AddRequest( FILEHANDLER_PURE3D, "art\\cars\\huskA.p3d", GMA_DEFAULT, "Global");
    GetLoadingManager()->AddRequest( FILEHANDLER_PURE3D, "art\\phonecamera.p3d", GMA_DEFAULT, "Global");
    GetLoadingManager()->AddRequest( FILEHANDLER_PURE3D, "art\\cards.p3d", GMA_DEFAULT, "Global");
    GetLoadingManager()->AddRequest( FILEHANDLER_PURE3D, "art\\wrench.p3d", GMA_DEFAULT, "Global");
    GetLoadingManager()->AddRequest( FILEHANDLER_PURE3D, "art\\missions\\generic\\missgen.p3d", GMA_DEFAULT, "Global");

    GetLoadingManager()->AddCallback( this );

    BLOG_M("[BC] OnStart END");



}


//==============================================================================
// BootupContext::OnStop
//==============================================================================
//
// Description: 
//
// Parameters:  
//
// Return:      
//
//==============================================================================
void BootupContext::OnStop( ContextEnum nextContext )
{
    rTunePrintf("BootupContext::OnStop... ");

#ifdef RAD_PSP
    { SceUID fd = sceIoOpen("ms0:/hitr_onstop.log",
                            PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0777);
      if(fd>=0){ sceIoWrite(fd,"[STOP] OnStop enter\n",20); sceIoClose(fd);} }
#endif

    GetGuiSystem()->UnregisterUserInputHandlers();

#ifdef RAD_PSP
    { SceUID fd = sceIoOpen("ms0:/hitr_onstop.log",
                            PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0777);
      if(fd>=0){ sceIoWrite(fd,"[STOP] after Unregister\n",25); sceIoClose(fd);} }
    // PSP: GUI scripts not loaded, skip GUI_MSG_RELEASE_BOOTUP
#else
    // release GUI bootup
    GetGuiSystem()->HandleMessage( GUI_MSG_RELEASE_BOOTUP );
#endif

#if defined( RAD_PC ) && defined( SHOW_MOVIES )
    GetInputManager()->GetFEMouse()->SetInGameMode( false );
#endif


#ifndef RAD_PSP
    MEMTRACK_POP_FLAG( "" );
    HeapMgr()->PopHeap ( GMA_PERSISTENT );
#endif
    rTunePrintf("Finished\n");
#ifdef RAD_PSP
    {
        SceUID fd = sceIoOpen("ms0:/hitr_onstop.log",
                              PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0777);
        if(fd>=0){ sceIoWrite(fd,"[STOP] before SetMemoryIdentification\n",38); sceIoClose(fd);}
    }
#endif
    SetMemoryIdentification( "BootupContext Finished" );
#ifdef RAD_PSP
    {
        SceUID fd = sceIoOpen("ms0:/hitr_onstop.log",
                              PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0777);
        if(fd>=0){ sceIoWrite(fd,"[STOP] OnStop END\n",18); sceIoClose(fd);}
    }
#endif
}


//==============================================================================
// BootupContext::OnUpdate
//==============================================================================
//
// Description: 
//
// Parameters:  
//
// Return:      
//
//==============================================================================

// hitr_upd_helper

void BootupContext::OnUpdate( unsigned int elapsedTime )
{
#ifdef RAD_PSP
    {
        static int s_count = 0;
        if (++s_count <= 5) {
            SceUID fd = sceIoOpen("ms0:/hitr_update.log",
                                  PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, 0777);
            if (fd >= 0) {
                char b[80]; int i = 0;
                const char* p = "[UPD] BootupContext::OnUpdate ";
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

#ifdef RAD_PSP
    // PSP: hard fallback — after 5 seconds in Bootup, force transition.
    {
        static int s_fe_cnt = 0;
        if ( ++s_fe_cnt == 300 && m_bootupLoadCompleted )
        {
            extern bool g_pspRequestFrontEnd;
            g_pspRequestFrontEnd = true;
            SceUID fd = sceIoOpen("ms0:/hitr_fe.log", PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0777);
            if (fd >= 0) { sceIoWrite(fd, "[FE] forced transition request\n", 30); sceIoClose(fd); }
        }
    }
#endif

#ifdef RAD_PSP
    {
        static int s_frm = 0;
        if ((++s_frm % 60) == 0) {
            SceUID fd = sceIoOpen("ms0:/hitr_update.log",
                                  PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0777);
            if (fd>=0){ char b[40]; int i=0; const char*p="[UPD] frame ";
              while(p[i]){b[i]=p[i];i++;}
              int v=s_frm; char n[10]; int k=0;
              while(v>0){n[k++]='0'+(v%10);v/=10;}
              for(int j=k-1;j>=0;j--) b[i++]=n[j];
              b[i++]='\n'; sceIoWrite(fd,b,i); sceIoClose(fd);} }
    }
#endif

#ifdef RAD_PSP
    // PSP: ResetLicenseScreenDisplayTime() is called by the license-screen GUI
    // which we don't have. Kick off the timer ourselves once assets are loaded.
    if ( m_elapsedTime == -1 && m_bootupLoadCompleted && m_soundLoadCompleted )
    {
        rReleasePrintf("[BC] PSP: starting license-screen timer\n");
        m_elapsedTime = 0;
    }
#endif

    if( m_elapsedTime != -1 )
    {
        if( m_elapsedTime > MINIMUM_LICENSE_SCREEN_DISPLAY_TIME &&
            m_bootupLoadCompleted && m_soundLoadCompleted )
        {
#ifdef RAD_PSP
            // PSP: don't call SetContext from inside OnUpdate (re-entrancy is
            // risky — GameFlow::OnTimerDone is mid-call). Instead set a flag
            // and let Game::Run do the transition after OnTimerDone returns.
            extern bool g_pspRequestFrontEnd;
            g_pspRequestFrontEnd = true;
            rReleasePrintf("[BC] PSP: requesting FrontEnd transition\n");
#else
            GetGuiSystem()->HandleMessage( GUI_MSG_QUIT_BOOTUP );
#endif
            m_elapsedTime = -1;
        }
        else
        {
            m_elapsedTime += elapsedTime;
        }
    }

#ifdef RAD_PSP
    // PSP: таймер перехода во FRONTEND отключён — сидим в Bootup, пока не разберёмся с загрузкой.
#endif

    // update game data manager
    GetGameDataManager()->Update( elapsedTime );
    GetPresentationManager()->Update( elapsedTime );
    // update GUI system
    GetGuiSystem()->Update( elapsedTime );
}


//==============================================================================
// BootupContext::OnSuspend
//==============================================================================
//
// Description: 
//
// Parameters:  
//
// Return:      
//
//==============================================================================
void BootupContext::OnSuspend()
{
}


//==============================================================================
// BootupContext::OnResume
//==============================================================================
//
// Description: 
//
// Parameters:  
//
// Return:      
//
//==============================================================================
void BootupContext::OnResume()
{
}


//==============================================================================
// BootupContext::OnHandleEvent
//==============================================================================
//
// Description: 
//
// Parameters:  
//
// Return:      
//
//==============================================================================
void BootupContext::OnHandleEvent( EventEnum id, void* pEventData )
{
}

//=============================================================================
// BootupContext::OnProcessRequestsComplete
//=============================================================================
// Description: Called when startup loading is done
//
// Parameters:  pUserData - unused
//
// Return:      void 
//
//=============================================================================
void BootupContext::OnProcessRequestsComplete( void* pUserData )
{
#ifdef RAD_PSP
    // PSP: SoundManager is stubbed (GetSoundManager() returns nullptr), and
    // AddCallback(this) passes pUserData=0, so the original comparison
    // `pUserData == GetSoundManager()` matched falsely. Disambiguate by
    // checking that GetSoundManager() actually exists.
    if( pUserData != NULL && pUserData == GetSoundManager() )
#else
    if( pUserData == GetSoundManager() )
#endif
    {
        // set flag indicating all sound loads have completed
        //
        m_soundLoadCompleted = true;
    }
    else
    {
        // set flag indicating all bootup loads (except for sound) have completed
        //
        m_bootupLoadCompleted = true;
    }
#ifdef RAD_PSP
    // PSP: SoundManager is stubbed; StartLoadingSound() from license GUI never
    // runs, so m_soundLoadCompleted would never be set on its own. Force it
    // once the bootup assets are done loading.
    if ( m_bootupLoadCompleted )
    {
        m_soundLoadCompleted = true;
    }
    {
        SceUID fd = sceIoOpen("ms0:/hitr_oprc.log", PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0777);
        if (fd >= 0) {
            char b[64]; int i=0;
            const char* m = "[OPRC] bootup="; while(m[i]){b[i]=m[i];i++;}
            b[i++] = '0' + (m_bootupLoadCompleted ? 1 : 0);
            const char* m2 = " sound="; int j=0; while(m2[j]){b[i++]=m2[j++];}
            b[i++] = '0' + (m_soundLoadCompleted ? 1 : 0);
            b[i++] = '\n';
            sceIoWrite(fd, b, i); sceIoClose(fd);
        }
    }
#endif

    //
    // Tell the sound manager to do some processing, now that the scripts
    // are sure to have been loaded.
    //
    // NOTE: I've moved this here since this call triggers a CPU-hogging
    // bit of dialog script postprocessing.  That processing should be pulled out
    // and done offline, but until then, do this somewhere where
    // it won't starve the completion of FMVs. -- Esan
    //
    if( m_bootupLoadCompleted && m_soundLoadCompleted )
    {
#ifdef RAD_PSP
        // PSP: SoundManager is stubbed (returns nullptr). Skip OnBootupComplete.
        // The subsequent ToggleRumble is safe (InputManager exists).
#else
        GetSoundManager()->OnBootupComplete();
#endif

        GetInputManager()->ToggleRumble( false );
    }
}

//=============================================================================
// BootupContext::OnPresentationEventBegin
//=============================================================================
// Description: Comment
//
// Parameters:  ( PresentationEvent* pEvent )
//
// Return:      void 
//
//=============================================================================
void BootupContext::OnPresentationEventBegin( PresentationEvent* pEvent )
{
}

//=============================================================================
// BootupContext::OnPresentationEventLoadComplete
//=============================================================================
// Description: Comment
//
// Parameters:  ( PresentationEvent* pEvent )
//
// Return:      void 
//
//=============================================================================
void BootupContext::OnPresentationEventLoadComplete( PresentationEvent* pEvent )
{
}


//=============================================================================
// BootupContext::OnPresentationEventEnd
//=============================================================================
// Description: Comment
//
// Parameters:  ( PresentationEvent* pEvent )
//
// Return:      void 
//
//=============================================================================
void BootupContext::OnPresentationEventEnd( PresentationEvent* pEvent )
{
    if( GetPresentationManager()->IsQueueEmpty() )
    {
		
        GetRenderManager()->mpLayer( RenderEnums::GUI )->Warm();
        // Switch to frontend context.
        GetGameFlow()->SetContext( CONTEXT_FRONTEND );
    }
}

//******************************************************************************
//
// Private Member Functions
//
//******************************************************************************


//==============================================================================
// BootupContext::BootupContext
//==============================================================================
//
// Description: 
//
// Parameters:  
//
// Return:      
//
//==============================================================================// 
BootupContext::BootupContext()
:   m_elapsedTime( -1 ),
    m_bootupLoadCompleted( false ),
    m_soundLoadCompleted( false ),
    m_pSharedShader( 0 )
{
    BLOG_M("[BC] ctor ENTER");

    BLOG_M("[BC] line 1");
    m_pSharedShader = 0;  // PSP: pddiPspDevice::NewShader returns nullptr
    BLOG_M("[BC] line 2");
    rAssert( m_pSharedShader );
    BLOG_M("[BC] line 3");
    /* PSP: skip AddRef, m_pSharedShader = 0 */
    BLOG_M("[BC] ctor EXIT");
}


//==============================================================================
// BootupContext::~BootupContext
//==============================================================================
//
// Description: 
//
// Parameters:  
//
// Return:      
//
//==============================================================================// 
BootupContext::~BootupContext()
{
    // Too bad we can't use tEntity::Release() since is
    //a pddi object.
    if( m_pSharedShader != 0 )
    {
        m_pSharedShader->Release();
        m_pSharedShader = 0;
    }
    spInstance = NULL;
}

