//=============================================================================
// psp_sound_stubs.cpp
//
// PSP-заглушки для soundrenderer. Все методы — no-op / nullptr,
// чтобы игра слинковалась и запустилась. Звука не будет.
//=============================================================================

#include <sound/soundrenderer/soundrenderingmanager.h>
#include <sound/soundrenderer/soundplayer.h>
#include <sound/soundrenderer/playermanager.h>
#include <sound/soundrenderer/soundresourcemanager.h>
#include <sound/soundrenderer/sounddynaload.h>
#include <sound/soundrenderer/soundnucleus.hpp>

#include <radmemory.hpp>

namespace Sound {

// --- Singleton ---
static daSoundRenderingManager* s_psp_RenderingManager = nullptr;

//=============================================================================
// daSoundRenderingManager
//=============================================================================

daSoundRenderingManager::daSoundRenderingManager() {}
daSoundRenderingManager::~daSoundRenderingManager() {}

daSoundRenderingManager* daSoundRenderingManager::GetInstance(void) { return s_psp_RenderingManager; }
void daSoundRenderingManager::Terminate() {}
void daSoundRenderingManager::QueueCementFileRegistration() {}
void daSoundRenderingManager::QueueRadscriptFileLoads() {}
void daSoundRenderingManager::SetLanguage(Scrooby::XLLanguage) {}
void daSoundRenderingManager::OnProcessRequestsComplete(void*) {}
void daSoundRenderingManager::Initialize() {}
bool daSoundRenderingManager::IsInitialized() { return false; }
void daSoundRenderingManager::Service() {}
void daSoundRenderingManager::ServiceOncePerFrame(unsigned int) {}
void daSoundRenderingManager::Render() {}
IRadNameSpace* daSoundRenderingManager::GetSoundNamespace() { return nullptr; }
IRadNameSpace* daSoundRenderingManager::GetTuningNamespace() { return nullptr; }
IRadNameSpace* daSoundRenderingManager::GetCharacterNamespace(unsigned int) { return nullptr; }
IRadSoundHalListener* daSoundRenderingManager::GetTheListener() { return nullptr; }
daSoundDynaLoadManager* daSoundRenderingManager::GetDynaLoadManager() { return nullptr; }
IDaSoundTuner* daSoundRenderingManager::GetTuner() { return nullptr; }
daSoundResourceManager* daSoundRenderingManager::GetResourceManager() { return nullptr; }
daSoundPlayerManager* daSoundRenderingManager::GetPlayerManager() { return nullptr; }

// Глобальные функции-фабрики из soundsystem.h
daSoundRenderingManager* daSoundRenderingManagerGet() { return s_psp_RenderingManager; }
void daSoundRenderingManagerCreate(radMemoryAllocator) { s_psp_RenderingManager = new daSoundRenderingManager(); }
void daSoundRenderingManagerTerminate() { delete s_psp_RenderingManager; s_psp_RenderingManager = nullptr; }

//=============================================================================
// daSoundClipStreamPlayer
//=============================================================================

void daSoundClipStreamPlayer::Play() {}
void daSoundClipStreamPlayer::UnCapture() {}
void daSoundClipStreamPlayer::RegisterSoundPlayerStateCallback(IDaSoundPlayerState*, void*) {}
void daSoundClipStreamPlayer::UnregisterSoundPlayerStateCallback(IDaSoundPlayerState*, void*) {}

//=============================================================================
// daSoundPlayerManager
//=============================================================================

void daSoundPlayerManager::CaptureFreePlayer(daSoundClipStreamPlayer** pp, IDaSoundResource*, bool)
{
    if (pp) *pp = nullptr;
}
void daSoundPlayerManager::PausePlayers() {}
void daSoundPlayerManager::ContinuePlayers() {}
bool daSoundPlayerManager::AreAllPlayersStopped() { return true; }

//=============================================================================
// daSoundResourceManager
//=============================================================================

IDaSoundResource* daSoundResourceManager::FindResource(const char*) { return nullptr; }
IDaSoundResource* daSoundResourceManager::FindResource(unsigned int) { return nullptr; }
void daSoundResourceManager::SetActiveResource(IRadNameSpace*) {}
void daSoundResourceManager::ReleaseActiveResource(IRadNameSpace*) {}

//=============================================================================
// daSoundDynaLoadManager
//=============================================================================

void daSoundDynaLoadManager::AddCompletionCallback(IDaSoundDynaLoadCompletionCallback*, void*) {}

//=============================================================================
// SoundNucleus
//=============================================================================

IRadSoundHalAudioFormat* SoundNucleusGetClipFileAudioFormat() { return nullptr; }

} // namespace Sound
