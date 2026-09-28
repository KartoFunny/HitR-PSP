//=============================================================================
// Copyright (C) 2025 The Simpsons Hit & Run PSP port
//
// Component:   PspPlatform
//
// Description: PSP implementation of the Platform interface.
//              Based on ps2platform (same MIPS architecture) and
//              win32platform (SDL-based, used for Android/Vita).
//
//=============================================================================

#ifndef PSPPLATFORM_H
#define PSPPLATFORM_H

#include "platform.h"
#include <data/config/gameconfig.h>
#include <cstdarg>

struct IRadMemoryHeap;
class tPlatform;
class tContext;

class PspPlatform : public Platform
#ifdef RAD_PC
    , public GameConfigHandler
#endif
{
public:
    // Singleton access
    static PspPlatform* CreateInstance();
    static PspPlatform* GetInstance();
    static void DestroyInstance();

    // Called BEFORE Foundation Tech is initialized
    static void InitializeFoundation();
    static void InitializeMemory();
    static void ShutdownMemory();

    // Platform interface
    virtual void InitializePlatform();
    virtual void ShutdownPlatform();

    virtual void LaunchDashboard();
    virtual void ResetMachine();

    virtual void DisplaySplashScreen( SplashScreen screenID,
        const char* overlayText = NULL,
        float fontScale = 1.0f,
        float textPosX = 0.0f,
        float textPosY = 0.0f,
        tColour textColour = tColour( 255, 255, 255 ),
        int fadeFrames = 3 );

    virtual void DisplaySplashScreen( const char* textureName,
        const char* overlayText = NULL,
        float fontScale = 1.0f,
        float textPosX = 0.0f,
        float textPosY = 0.0f,
        tColour textColour = tColour( 255, 255, 255 ),
        int fadeFrames = 3 );

    virtual void OnControllerError(const char *msg);

protected:
    virtual void InitializeFoundationDrive();
    virtual void ShutdownFoundation();
    virtual void InitializePure3D();
    virtual void ShutdownPure3D();

private:
    PspPlatform();
    virtual ~PspPlatform();

    PspPlatform( const PspPlatform& );
    PspPlatform& operator=( const PspPlatform& );

    static PspPlatform* spInstance;

    tPlatform* mpPlatform;
    tContext*  mpContext;
};

#endif // PSPPLATFORM_H
