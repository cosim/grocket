/**
 * @file libgrocket/gr_library_impl.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/08
 * @version $Revision$ 
 * @brief   server frame function library interface implement
 * Revision History
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-08    Created.
 **/
/* 
 *
 * Copyright (C) 2013-now da_ming at hotmail.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "gr_library_impl.h"
#include "gr_log.h"
#include "gr_global.h"
#include "gr_errno.h"
#include "gr_tools.h"
#include "gr_mem.h"
#include "gr_dll.h"
#include "gr_config.h"
#include "gr_library_invoke.h"
#include "server_object.h"

typedef struct
{
    gr_dll_t            core_dll;
    gr_library_init_t   core_init;

    // 将singleton的服务器对象保存在全局结构里，相当于一个全局变量
    GR_CLASS_DECLARE_SINGLETON( server )

} gr_library_impl_t;

#define gr_server      g_ghost_rocket_global.server_interface

static
int buildin_library_init(
    gr_library_t *  library
);

static inline
void library_impl_destroy( gr_library_impl_t * self )
{
    uint32_t i;

    if ( self ) {
        if ( NULL != gr_server.library ) {
            for ( i = 0; i < gr_server.library->class_max; ++ i ) {
                gr_class_t * klass = gr_server.library->classes[ i ];
                if ( NULL != klass ) {
                    if ( NULL != klass->destroy_class ) {
                        klass->destroy_class( gr_server.library->classes[ i ] );
                    }
                    gr_server.library->classes[ i ] = NULL;
                }
            }

            gr_free( gr_server.library );
            gr_server.library = NULL;
        }

        if ( NULL != self->core_dll ) {
            self->core_init = NULL;
            gr_dll_close( self->core_dll );
            self->core_dll = NULL;
        }

        gr_free( self );
    }
}

int gr_library_impl_init()
{
    gr_library_impl_t * p;
    int                 r;
    int                 class_max   = gr_config_library_class_max();
    const char *        library_core= gr_config_library_core_path();
    int                 ret;

    if ( g_ghost_rocket_global.server_interface.is_debug ) {
        ret = gr_invoke_test();
        if ( 0 != ret ) {
            gr_fatal( "[init] gr_invoke_test return error %d", ret );
            return GR_ERR_UNKNOWN;
        }
    }

    if ( NULL != g_ghost_rocket_global.library ) {
        gr_fatal( "[init]gr_library_impl_init already called" );
        return GR_ERR_WRONG_CALL_ORDER;
    }

    p = (gr_library_impl_t *)gr_calloc( 1, sizeof( gr_library_impl_t ) );
    if ( NULL == p ) {
        gr_fatal( "[init]malloc %d bytes failed, errno=%d,%s",
            (int)sizeof(gr_library_impl_t), errno, strerror( errno ) );
        return GR_ERR_BAD_ALLOC;
    }

    r = GR_ERR_UNKNOWN;

    do {

        gr_server.library = (gr_library_t *)gr_calloc( 1,
            sizeof( gr_library_t ) + sizeof( gr_class_t * ) * ( class_max - 1 ) );
        if ( NULL == gr_server.library ) {
            r = GR_ERR_BAD_ALLOC;
            break;
        }

        memcpy( gr_server.library->magic, GR_LIBRARY_MAGIC, sizeof( GR_LIBRARY_MAGIC ) - 1 );
        gr_server.library->version      = GR_LIBRARY_VERSION;
        gr_server.library->low_version  = GR_LIBRARY_LOW_VERSION;
        gr_server.library->class_max    = class_max;

        // 装载所有内置对象
        g_ghost_rocket_global.library = p;
        ret = buildin_library_init( gr_server.library );
        if ( 0 != ret ) {
            gr_fatal( "buildin_library_init return error %d", ret );
            break;
        }

        // 试图装载所有外置对象
        if ( NULL != library_core && '\0' != * library_core && is_exists( library_core ) ) {
            p->core_dll = gr_dll_open( library_core );
            if ( NULL != p->core_dll ) {
                p->core_init = (gr_library_init_t)gr_dll_symbol( p->core_dll, GR_LIBRARY_INIT_NAME );
                if ( NULL == p->core_init ) {
                    gr_fatal( "can not find %s in %s", GR_LIBRARY_INIT_NAME, library_core );
                    break;
                }

                ret = p->core_init( gr_server.library );
                if ( 0 != ret ) {
                    gr_fatal( "call %s in %s return error %d", GR_LIBRARY_INIT_NAME, library_core, ret );
                    break;
                }

            } else {
                gr_info( "load %s failed, ignore", library_core );
            }
        }

        r = GR_OK;
    } while ( false );

    if ( GR_OK != r ) {
        library_impl_destroy( p );
        g_ghost_rocket_global.library = NULL;
        return r;
    }

    g_ghost_rocket_global.library = p;
    return GR_OK;
}

void gr_library_impl_term()
{
    gr_library_impl_t * p = (gr_library_impl_t *)g_ghost_rocket_global.library;
    if ( NULL != p ) {
        library_impl_destroy( p );
        g_ghost_rocket_global.library = NULL;
    }
}

///////////////////////////////////////////////////////////////////////

static inline
int buildin_library_init(
    gr_library_t *  library
)
{
    gr_library_impl_t * parent  = (gr_library_impl_t *)g_ghost_rocket_global.library;

    if ( NULL != gr_server.library->classes[ CLASS_SERVER ] ) {
        // 你丫把我地方占了！
        gr_fatal( "CLASS_SERVER slot must be NULL!" );
        return -1;
    }

    // 安装 server singleton 对象到 library 中
    GR_CLASS_INSTALL_SINGLETON( library, parent, server, CLASS_SERVER );

    if ( NULL == gr_server.library->classes[ CLASS_SERVER ] ) {
        gr_fatal( "install CLASS_SERVER slot failed" );
        return -1;
    }

    gr_server.library->buildin = (gr_i_server_t *)gr_server.library->classes[ CLASS_SERVER ];
    return 0;
}
