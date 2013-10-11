#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_MEM_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_MEM_H_

// 想在Windows下链接成功,把 third 目录下的 gperftools-2.1.tar.gz 解压到当前目录先

#include "gr_stdinc.h"
#include "gperftools/tcmalloc.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline
void * gr_malloc( size_t n )
{
    return tc_malloc( n );
}

static inline
void gr_free( void * p )
{
    tc_free( p );
}

static inline
void * gr_calloc( size_t item_size, size_t item_count )
{
    return tc_calloc( item_size, item_count );
}

static inline
void* gr_realloc( void * ptr, size_t size )
{
    return tc_realloc( ptr, size );
}

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_MEM_H_
