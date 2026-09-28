//=============================================================================
// psp_controller_stubs.cpp — заглушки radController для PSP.
//=============================================================================

#include <radcontroller.hpp>

void radControllerInitialize(IRadControllerConnectionChangeCallback*, radMemoryAllocator) {}
void radControllerTerminate(void) {}
IRadControllerSystem* radControllerSystemGet(void) { return nullptr; }
void radControllerSystemService(void) {}
