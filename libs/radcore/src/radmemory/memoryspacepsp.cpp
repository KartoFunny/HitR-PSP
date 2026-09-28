//=============================================================================
// memoryspacepsp.cpp
//
// PSP-заглушка для radMemorySpace* — на PSP нет отдельных memory spaces,
// вся память линейная. Все операции = no-op или memcpy.
//=============================================================================

#include <radmemory.hpp>
#include <cstring>

// Глобальные переменные, объявленные extern в radmemory.hpp
unsigned int radMemorySpace_OptimalMultiple = 32;
unsigned int radMemorySpace_OptimalAlignment = 32;

//=============================================================================
// ::radMemorySpaceInitialize
//=============================================================================
void radMemorySpaceInitialize( void )
{
    // Нет отдельных memory spaces — no-op.
}

//=============================================================================
// ::radMemorySpaceTerminate
//=============================================================================
void radMemorySpaceTerminate( void )
{
    // Симметрично Initialize.
}

//=============================================================================
// ::radMemorySpaceCopyAsync
//=============================================================================
// На PSP вся память линейная, поэтому "асинхронная" копия — просто memcpy.
// Возвращаем nullptr — на PS2/Vita тут был бы объект запроса.
//=============================================================================
IRadMemorySpaceCopyRequest* radMemorySpaceCopyAsync(
    void* pDest, radMemorySpace /*spaceDest*/,
    const void* pSrc, radMemorySpace /*spaceSrc*/,
    unsigned int bytes )
{
    if ( pDest && pSrc && bytes )
    {
        memcpy( pDest, pSrc, bytes );
    }
    return nullptr;
}

//=============================================================================
// ::radMemorySpaceAlloc
//=============================================================================
void* radMemorySpaceAlloc( radMemorySpace /*space*/,
                            radMemoryAllocator allocator,
                            unsigned int numBytes )
{
    return radMemoryAlloc( allocator, numBytes );
}

//=============================================================================
// ::radMemorySpaceFree
//=============================================================================
void radMemorySpaceFree( radMemorySpace /*space*/,
                         radMemoryAllocator allocator,
                         void* pMemory )
{
    radMemoryFree( allocator, pMemory );
}

//=============================================================================
// ::radMemorySpaceGetAllocator
//=============================================================================
IRadMemoryAllocator* radMemorySpaceGetAllocator( radMemorySpace /*space*/,
                                                   radMemoryAllocator /*allocator*/ )
{
    // На PSP нет отдельных memory spaces. Возвращаем nullptr,
    // вызывающий код должен использовать radMemoryAlloc напрямую.
    return nullptr;
}
