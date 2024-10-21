#ifdef PROFILING
#include "external/tracy/public/tracy/Tracy.hpp"

// for now doesn't work, TODO

/*void* operator new( std::size_t count )
{
    auto ptr = malloc( count );
    TracyAllocS( ptr, count, 10 );
    return ptr;
}

void operator delete( void* ptr ) noexcept
{
    TracyFreeS( ptr, 10 );
    free( ptr );
}*/

#endif