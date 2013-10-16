/**
 * @file libgrocket/gr_tcp_out.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/03
 * @version $Revision$ 
 * @brief   TCP send module
 * Revision History
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-03    Created.
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

#include "gr_tcp_out.h"
#include "tcp_io.h"
#if defined( WIN32 ) || defined( WIN64 )
#include "gr_tcp_in.h"
#endif // #if defined( WIN32 ) || defined( WIN64 )

void tcp_out_worker( gr_thread_t * thread )
{
#define     TCP_OUT_WAIT_TIMEOUT    100
    int                     count;
    int                     i;
    gr_tcp_out_t *          self;
    gr_poll_event_t *       events;
    gr_poll_event_t *       e;
    gr_tcp_conn_item_t *    conn;

    self    = (gr_tcp_out_t *)thread->param;

    events  = (gr_poll_event_t *)gr_malloc( sizeof( gr_poll_event_t ) * self->concurrent );
    if ( NULL == events ) {
        gr_fatal( "bad_alloc %d", (int)sizeof( gr_poll_event_t ) * self->concurrent );
        return;
    }

    while ( ! thread->is_need_exit ) {

        count = gr_poll_wait( self->poll, events, self->concurrent, TCP_OUT_WAIT_TIMEOUT, thread );
        if ( count < 0 ) {
            gr_fatal( "gr_poll_wait return %d", count );
            continue;
        } else if ( 0 == count ) {
            continue;
        }

        for ( i = 0; i < count; ++ i ) {
            e = & events[ i ];

            conn = (gr_tcp_conn_item_t *)e->data.ptr;
            on_tcp_send( self, thread, conn );
        }
    };

    gr_free( events );
}

int gr_tcp_out_init()
{
    gr_tcp_out_t *  p;
    int thread_count = gr_config_tcp_out_thread_count();
    int r;

    if ( thread_count < 1 ) {
        gr_fatal( "[init]tcp_out thread_count invalid" );
        return GR_ERR_INVALID_PARAMS;
    }

    if ( NULL != g_ghost_rocket_global.tcp_out ) {
        gr_fatal( "[init]gr_tcp_out_init already called" );
        return GR_ERR_WRONG_CALL_ORDER;
    }

    p = (gr_tcp_out_t *)gr_calloc( 1, sizeof( gr_tcp_out_t ) );
    if ( NULL == p ) {
        gr_fatal( "[init]malloc %d bytes failed, errno=%d,%s",
            (int)sizeof(gr_tcp_out_t), errno, strerror( errno ) );
        return GR_ERR_BAD_ALLOC;
    }

    p->concurrent = gr_config_tcp_out_concurrent();

    r = GR_OK;

    do {

        const char * name = "tcp.output";

        p->poll = gr_poll_create( p->concurrent, thread_count, GR_POLLOUT, name );
        if ( NULL == p->poll ) {
            r = GR_ERR_INIT_POLL_FALED;
            break;
        }

#if defined( WIN32 ) || defined( WIN64 )
        // windows不允许同一个socket同时加到两个iocp里
        // 这里让tcp_out和tcp_in共用IOCP
        r = gr_pool_replace_from(
            p->poll,
            (gr_poll_t *)gr_tcp_in_get_poll()
        );
        if ( 0 != r ) {
            r = GR_ERR_INIT_POLL_FALED;
            break;
        }
        gr_info( "tcp.in and tcp.out use sampe IOCP" );
#endif

        r = gr_threads_start(
            & p->threads,
            thread_count,
            NULL,
#if defined( WIN32 ) || defined( WIN64 )
            tcp_io_windows,
#else
            tcp_out_worker,
#endif
            p,
            gr_poll_raw_buff_for_tcp_out_len(),
            true,
            name );
        if ( GR_OK != r ) {
            break;
        }

        gr_debug( "tcp_out_init OK" );

        r = GR_OK;
    } while ( false );

    if ( GR_OK != r ) {

        gr_threads_close( & p->threads );

        if ( NULL != p->poll ) {
            gr_poll_destroy( p->poll );
            p->poll = NULL;
        }

        gr_free( p );
        return r;
    }

    g_ghost_rocket_global.tcp_out = p;
    return GR_OK;
}

void gr_tcp_out_term()
{
    gr_tcp_out_t *  p = (gr_tcp_out_t *)g_ghost_rocket_global.tcp_out;
    if ( NULL != p ) {

        gr_threads_close( & p->threads );

        if ( NULL != p->poll ) {
            gr_poll_destroy( p->poll );
            p->poll = NULL;
        }

        gr_free( p );
        g_ghost_rocket_global.tcp_out = NULL;
    }
}

int gr_tcp_out_add(
    gr_tcp_rsp_t * rsp
)
{
    int                     r;
    gr_tcp_out_t *           self;
    gr_tcp_conn_item_t *    conn;
    
    self = (gr_tcp_out_t *)g_ghost_rocket_global.tcp_out;
    if ( NULL == self ) {
        gr_fatal( "gr_tcp_out_init never call" );
        return -1;
    }

    conn = rsp->parent;
    if ( NULL == conn ) {
        gr_fatal( "rsp->parent is NULL" );
        return -1;
    }

    // 该标记表示连接在数据发送线程里
    conn->tcp_out_open = true;

    // 将该socket加到poll里
    r = gr_poll_add_tcp_send_fd(
        self->poll,
        conn,
        & self->threads
    );
    if ( 0 != r ) {
        gr_fatal( "gr_poll_add_tcp_send_fd return %d", r );
        return -3;
    }

    return 0;
}
