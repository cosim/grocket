/**
 * @file libgrocket/gr_module.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   user module
 * Revision History
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-05    Created.
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

#include "gr_module.h"
#include "gr_log.h"
#include "gr_global.h"
#include "gr_errno.h"
#include "gr_tools.h"
#include "gr_mem.h"
#include "gr_dll.h"
#include "gr_config.h"

typedef struct
{
    gr_dll_t                dll;

    gr_init_t               init;
    gr_term_t               term;
    gr_tcp_accept_t         tcp_accept;
    gr_tcp_close_t          tcp_close;
    gr_check_t              chk_binary;
    gr_proc_t               proc_binary;
    gr_proc_http_t          proc_http;

} gr_module_t;

static inline
void module_unload(
    gr_module_t * module
)
{
    module->init         = NULL;
    module->term         = NULL;
    module->tcp_accept   = NULL;
    module->tcp_close    = NULL;
    module->chk_binary   = NULL;
    module->proc_binary  = NULL;
    module->proc_http    = NULL;

    if ( NULL != module->dll ) {
        gr_dll_close( module->dll );
        module->dll = NULL;
    }
}

static inline
int module_load(
    gr_module_t * module,
    const char * path,
    bool is_absolute
)
{
    if ( NULL != module->dll ) {
        return GR_ERR_WRONG_CALL_ORDER;
    }

    if ( is_absolute ) {
        module->dll = gr_dll_open_absolute( path );
    } else {
        module->dll = gr_dll_open( path );
    }
    if ( NULL== module->dll ) {
        return GR_ERR_INIT_MODULE_FAILED;
    }

    module->init = (gr_init_t)gr_dll_symbol( module->dll, GR_INIT_NAME );
    module->term = (gr_term_t)gr_dll_symbol( module->dll, GR_TERM_NAME );
    module->tcp_accept = (gr_tcp_accept_t)gr_dll_symbol( module->dll, GR_TCP_ACCEPT_NAME );
    module->tcp_close = (gr_tcp_close_t)gr_dll_symbol( module->dll, GR_TCP_CLOSE_NAME );
    module->chk_binary = (gr_check_t)gr_dll_symbol( module->dll, GR_CHECK_NAME );
    module->proc_binary = (gr_proc_t)gr_dll_symbol( module->dll, GR_PROC_NAME );
    module->proc_http = (gr_proc_http_t)gr_dll_symbol( module->dll, GR_PROC_HTTP_NAME );

    if ( NULL == module->proc_binary && NULL == module->proc_http ) {
        gr_fatal( "(%s) gr_proc_http & gr_proc both not found", path );
        module_unload( module );
        return GR_ERR_INIT_MODULE_FAILED;
    }

    return 0;
}

int gr_module_init(
    gr_init_t       init,
    gr_term_t       term,
    gr_tcp_accept_t tcp_accept,
    gr_tcp_close_t  tcp_close,
    gr_check_t      chk_binary,
    gr_proc_t       proc_binary,
    gr_proc_http_t  proc_http)
{
    gr_module_t *  module;

    if ( NULL != g_ghost_rocket_global.module ) {
        gr_fatal( "[init]gr_module_init already called" );
        return GR_ERR_WRONG_CALL_ORDER;
    }

    module = (gr_module_t *)gr_calloc( 1, sizeof( gr_module_t ) );
    if ( NULL == module ) {
        gr_fatal( "[init]malloc %d bytes failed, errno=%d,%s",
            (int)sizeof(gr_module_t), errno, strerror( errno ) );
        return GR_ERR_BAD_ALLOC;
    }

    module->init         = init;
    module->term         = term;
    module->tcp_accept   = tcp_accept;
    module->tcp_close    = tcp_close;
    module->chk_binary   = chk_binary;
    module->proc_binary  = proc_binary;
    module->proc_http    = proc_http;

    if (   NULL == module->init
        && NULL == module->term
        && NULL == module->tcp_accept
        && NULL == module->tcp_close
        && NULL == module->chk_binary
        && NULL == module->proc_binary
        && NULL == module->proc_http
    )
    {
        // 没指定用户函数，要装载模块
        char path[ MAX_PATH ] = "";
        bool is_absolute;
        gr_config_get_module_path( path, sizeof( path ), & is_absolute );

        if ( '\0' != path[ 0 ] ) {
            int r;
            r = module_load( module, path, is_absolute );
            if ( 0 != r ) {
                gr_fatal( "module_load( %s ) failed, return %d", path, r );
                return r;
            }
        }
    }

    g_ghost_rocket_global.module = module;
    return GR_OK;
}

void gr_module_term()
{
    if ( NULL != g_ghost_rocket_global.module ) {
        gr_module_t * module = g_ghost_rocket_global.module;
        module_unload( module );

        gr_free( g_ghost_rocket_global.module );
        g_ghost_rocket_global.module = NULL;
    }
}

int gr_module_master_process_init()
{
    gr_module_t * module = g_ghost_rocket_global.module;
    if ( NULL != module && NULL != module->init ) {
        int r = module->init( GR_PROCESS_MASTER, & g_ghost_rocket_global.server_interface );
        if ( 0 != r ) {
            gr_fatal( "init with GR_PROCESS_MASTER return %d", r );
            return r;
        }
    }
    return 0;
}

void gr_module_master_process_term()
{
    gr_module_t * module = g_ghost_rocket_global.module;
    if ( NULL != module && NULL != module->term ) {
        module->term( GR_PROCESS_MASTER, NULL );
    }
}

int gr_module_child_process_init()
{
    gr_module_t * module = g_ghost_rocket_global.module;
    if ( NULL != module && NULL != module->init ) {
        int r = module->init( GR_PROCESS_CHILD, & g_ghost_rocket_global.server_interface );
        if ( 0 != r ) {
            gr_fatal( "init with GR_PROCESS_CHILD return %d", r );
            return r;
        }
    }
    return 0;
}

void gr_module_child_process_term()
{
    gr_module_t * module = g_ghost_rocket_global.module;
    if ( NULL != module && NULL != module->term ) {
        module->term( GR_PROCESS_CHILD, NULL );
    }
}

int gr_module_worker_init( int worker_id )
{
    gr_module_t * module = g_ghost_rocket_global.module;
    if ( NULL != module && NULL != module->init ) {
        int r = module->init(
            GR_PROCESS_THREAD_1 + worker_id,
            & g_ghost_rocket_global.server_interface );
        if ( 0 != r ) {
            gr_fatal( "init with GR_PROCESS_THREAD_1 + %d return %d", worker_id, r );
            return r;
        }
    }
    return 0;
}

void gr_module_worker_term( int worker_id )
{
    gr_module_t * module = g_ghost_rocket_global.module;
    if ( NULL != module && NULL != module->term ) {
        module->term( GR_PROCESS_THREAD_1 + worker_id, NULL );
    }
}

bool gr_module_on_tcp_accept(
    gr_port_item_t *    port_item,
    int                 fd
)
{
    gr_module_t * module = g_ghost_rocket_global.module;
    if ( NULL != module && NULL != module->tcp_accept ) {
        bool need_disconnect = false;
        module->tcp_accept( port_item->port, fd, & need_disconnect );
        return ! need_disconnect;
    }

    return true;
}

void gr_module_check_tcp(
    gr_tcp_req_t *      req,
    bool *              is_error,
    bool *              is_full
)
{
    gr_module_t * module = g_ghost_rocket_global.module;

    * is_error  = false;
    * is_full   = false;

    if ( NULL != module && NULL != module->chk_binary ) {
        module->chk_binary(
            req->buf, req->buf_len,
            req->parent->port_item,
            req->parent->fd,
            & req->check_ctxt,
            is_error,
            is_full
        );
    }
}

void gr_module_proc_tcp(
    gr_tcp_req_t *      req,
    gr_proc_ctxt_t *    ctxt,
    int *               processed_len
)
{
    gr_module_t * module = g_ghost_rocket_global.module;

    if ( NULL != module && NULL != module->proc_binary ) {

        module->proc_binary(
            req->buf,
            req->buf_len,
            ctxt,
            processed_len
        );
    } else {
        // 没实现二进制数据包处理，我只能断连接了
        * processed_len = -1;
    }
}
