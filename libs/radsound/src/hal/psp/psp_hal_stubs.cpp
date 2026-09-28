//=============================================================================
// psp_hal_stubs.cpp
//
// PSP-заглушки для radsound HAL. Все возвращают nullptr — звук не работает,
// но код линкуется. Реальная реализация через sceAudio — позже.
//=============================================================================

#include <radsound_hal.hpp>

IRadSoundHalSystem*         radSoundHalSystemGet(void) { return nullptr; }
IRadSoundHalListener*       radSoundHalListenerGet(void) { return nullptr; }
IRadSoundHalVoice*          radSoundHalVoiceCreate(radMemoryAllocator) { return nullptr; }
IRadSoundHalBuffer*         radSoundHalBufferCreate(radMemoryAllocator) { return nullptr; }
IRadSoundHalPositionalGroup* radSoundHalPositionalGroupCreate(radMemoryAllocator) { return nullptr; }
