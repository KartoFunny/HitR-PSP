//=============================================================================
// psp_early_init.cpp
//
// ВАЖНО: этот файл должен линковаться ПЕРВЫМ.
// Его constructor с приоритетом 101 выполняется РАНЬШЕ всех остальных
// статических конструкторов в проекте. Это критично: глобальные объекты
// в code/ и libs/ могут вызывать radMemoryAlloc() из своих конструкторов,
// а таблица аллокаторов radcore пуста до radMemoryInitialize().
//=============================================================================

#include <radmemory.hpp>
#include <radtime.hpp>

extern "C" void __attribute__((constructor(101))) psp_early_radcore_init(void)
{
    radMemoryInitialize();
    radTimeInitialize();
}
