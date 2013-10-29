/**
 * @file libgrocket/gr_tcp_in.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/03
 * @version $Revision$ 
 * @brief   TCP recv module
 * Revision History
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-03    Created.
 *       2     zouyueming   2013-10-27    support tcp out disable
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

#include "gr_tcp_in.h"
#include "tcp_io.h"

#if ! defined( WIN32 ) && ! defined( WIN64 )

static
void tcp_in_worker( gr_thread_t * thread )
{
#define     TCP_IN_WAIT_TIMEOUT    100
    int                     count;
    int                     i;
    gr_tcp_in_t *           self;
    gr_poll_event_t *       events;
    gr_poll_event_t *       e;
    gr_server_t *           server;
    gr_tcp_conn_item_t *    conn;
    gr_poll_t *             poll;

    server  = & g_ghost_rocket_global.server_interface;
    self    = (gr_tcp_in_t *)thread->param;
    poll    = self->polls[ thread->id ];

    events  = (gr_poll_event_t *)gr_malloc( sizeof( gr_poll_event_t ) * self->concurrent );
    if ( NULL == events ) {
        gr_fatal( "bad_alloc %d", (int)sizeof( gr_poll_event_t ) * self->concurrent );
        return;
    }

    while ( ! thread->is_need_exit ) {

        count = gr_poll_wait( poll, events, self->concurrent, TCP_IN_WAIT_TIMEOUT, thread );
        if ( count < 0 ) {
            gr_fatal( "gr_poll_wait return %d", count );
            continue;
        } else if ( 0 == count ) {
            continue;
        }

        for ( i = 0; i < count; ++ i ) {
            e = & events[ i ];
            // TCP recv
            conn = (gr_tcp_conn_item_t *)e->data.ptr;
            on_tcp_recv( self, poll, thread, conn );
        }
    };

    gr_free( events );
}

#endif // #if ! defined( WIN32 ) && ! defined( WIN64 )

int gr_tcp_in_init()
{
    gr_tcp_in_t *   p;
    int             thread_count;
    int             r;
    int             i;

    thread_count = gr_config_tcp_in_thread_count();
    if ( thread_count < 1 ) {
        gr_fatal( "[init]tcp_in thread_count invalid" );
        return GR_ERR_INVALID_PARAMS;
    }

    if ( NULL != g_ghost_rocket_global.tcp_in ) {
        gr_fatal( "[init]gr_tcp_in_init already called" );
        return GR_ERR_WRONG_CALL_ORDER;
    }

    p = (gr_tcp_in_t *)gr_calloc( 1, sizeof( gr_tcp_in_t ) );
    if ( NULL == p ) {
        gr_fatal( "[init]malloc %d bytes failed, errno=%d,%s",
            (int)sizeof(gr_tcp_in_t), errno, strerror( errno ) );
        return GR_ERR_BAD_ALLOC;
    }

    p->concurrent       = gr_config_tcp_in_concurrent();
    p->worker_disabled  = gr_config_worker_disabled();
    p->tcp_out_disabled = gr_config_tcp_out_disabled();

    r = GR_OK;

    do {

        const char * name = "tcp.input ";

        p->polls = (gr_poll_t**)gr_calloc( 1, sizeof( gr_poll_t * ) * thread_count );
        if ( NULL == p->polls ) {
            gr_fatal( "[init]gr_calloc %d bytes failed",
                (int)sizeof( gr_poll_t * ) * thread_count );
            r = GR_ERR_INIT_POLL_FALED;
            break;
        }

        for ( i = 0; i < thread_count; ++ i ) {
            p->polls[ i ] = gr_poll_create(
                p->concurrent,
                thread_count,
                p->tcp_out_disabled ? (GR_POLLIN | GR_POLLOUT) : GR_POLLIN,
                name );
            if ( NULL == p->polls[ i ] ) {
                gr_fatal( "[init]gr_poll_create return NULL" );
                r = GR_ERR_INIT_POLL_FALED;
                break;
            }
        }
        if ( GR_OK != r ) {
            break;
        }

        r = gr_threads_start(
            & p->threads,
            thread_count,
            NULL,
#if defined( WIN32 ) || defined( WIN64 )
            tcp_io_worker,
#else
            p->tcp_out_disabled ? tcp_io_worker : tcp_in_worker,
#endif
            p,
            gr_poll_raw_buff_for_tcp_in_len(),
            true,
            ENABLE_THREAD,
            name );
        if ( GR_OK != r ) {
            gr_fatal( "[init]gr_threads_start return error %d", r );
            break;
        }

        gr_info( "[init]tcp.in init OK, thread_count=%d, worker.disabled=%d, tcp.out.disabled=%d",
            thread_count, (int)p->worker_disabled, (int)p->tcp_out_disabled );

        r = GR_OK;
    } while ( false );

    if ( GR_OK != r ) {

        gr_threads_close( & p->threads );

        if ( p->polls ) {
            for ( i = 0; i < thread_count; ++ i ) {
                if ( NULL != p->polls[ i ] ) {
                    if ( p->polls[ i ] ) {
                        gr_poll_destroy( p->polls[ i ] );
                        p->polls[ i ] = NULL;
                    }
                }
            }

            gr_free( p->polls );
            p->polls = NULL;
        }

        gr_free( p );
        return r;
    }

    g_ghost_rocket_global.tcp_in = p;
    return GR_OK;
}

void gr_tcp_in_term()
{
    gr_tcp_in_t *  p = (gr_tcp_in_t *)g_ghost_rocket_global.tcp_in;
    if ( NULL != p ) {

        int i;
        int thread_count = p->threads.thread_count;

        gr_threads_close( & p->threads );

        if ( p->polls ) {
            for ( i = 0; i < thread_count; ++ i ) {
                if ( NULL != p->polls[ i ] ) {
                    gr_poll_destroy( p->polls[ i ] );
                    p->polls[ i ] = NULL;
                }
            }

            gr_free( p->polls );
            p->polls = NULL;
        }

        gr_free( p );
        g_ghost_rocket_global.tcp_in = NULL;
    }
}

static_inline
size_t
tcp_in_hash(
    gr_tcp_in_t *           self,
    gr_tcp_conn_item_t *    conn
)
{
    return conn->fd % self->threads.thread_count;
}

int gr_tcp_in_add_conn( gr_tcp_conn_item_t * conn )
{
    int             r;
    gr_tcp_in_t *   self;
    gr_poll_t *     poll;

    self = (gr_tcp_in_t *)g_ghost_rocket_global.tcp_in;
    if ( NULL == self ) {
        gr_fatal( "gr_tcp_in_init never call" );
        return -1;
    }

    poll = self->polls[ tcp_in_hash( self, conn ) ];

    // 该标记表示连接已经在数据收线程里
    conn->tcp_in_open = true;

    // 请求、回复模式
    //assert( 0 == conn->thread_refs );
    //conn->thread_refs = 1;

    // 将该socket加到poll里
    r = gr_poll_add_tcp_recv_fd(
        poll,
        conn,
        & self->threads
    );
    if ( 0 != r ) {
        gr_fatal( "[tcp.input ]gr_poll_add_tcp_recv_fd return %d", r );
        //gr_atomic_add( -1, & conn->thread_refs );
        conn->tcp_in_open = false;
        return -2;
    }

    return 0;
}

int gr_tcp_in_del_tcp_conn( gr_tcp_conn_item_t * conn )
{
    int             r;
    gr_tcp_in_t *   self;
    gr_poll_t *     poll;

    self = (gr_tcp_in_t *)g_ghost_rocket_global.tcp_in;
    if ( NULL == self ) {
        gr_fatal( "[tcp.input ]gr_tcp_in_init never call" );
        return -1;
    }

    poll = self->polls[ tcp_in_hash( self, conn ) ];

    r = gr_poll_del_tcp_recv_fd(
        poll,
        conn,
        & self->threads
    );
    if ( 0 != r ) {
        gr_fatal( "[tcp.input ]gr_poll_del_tcp_rec_fd return %d", r );
        return -3;
    }

    return 0;
}

void ** gr_tcp_in_get_polls( int * count )
{
    gr_tcp_in_t *   self;
    
    self = (gr_tcp_in_t *)g_ghost_rocket_global.tcp_in;
    assert( self );

    assert( count );
    * count = self->threads.thread_count;

    return (void**)self->polls;
}
