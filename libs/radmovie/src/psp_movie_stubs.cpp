//=============================================================================
// psp_movie_stubs.cpp — заглушки radMovie для PSP.
//=============================================================================

#include <radmovie2.hpp>
#include <radmemory.hpp>

void radMovieInitialize2( radMemoryAllocator alloc )
{
    (void)alloc;
}

void radMovieTerminate2( void )
{
}

void radMovieService2( void )
{
}

IRadMoviePlayer2* radMoviePlayerCreate2( radMemoryAllocator )
{
    return nullptr;
}

IRadMovieRenderStrategy* radMovieSimpleFullScreenRenderStrategyCreate( radMemoryAllocator )
{
    return nullptr;
}
