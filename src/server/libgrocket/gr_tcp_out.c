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

#include "gr_tcp_out.h"
#include "tcp_io.h"
#include "gr_tcp_in.h"

#if ! defined( WIN32 ) && ! defined( WIN64 )

static
void tcp_out_worker( gr_thread_t * thread )
{
#define     TCP_OUT_WAIT_TIMEOUT    100
    int                     count;
    int                     i;
    gr_tcp_out_t *          self;
    gr_poll_event_t *       events;
    gr_poll_event_t *       e;
    gr_tcp_conn_item_t *    conn;
    gr_poll_t *             poll;

    self    = (gr_tcp_out_t *)thread->param;
    poll    = self->polls[ thread->id ];

    events  = (gr_poll_event_t *)gr_malloc( sizeof( gr_poll_event_t ) * self->concurrent );
    if ( NULL == events ) {
        gr_fatal( "bad_alloc %d", (int)sizeof( gr_poll_event_t ) * self->concurrent );
        return;
    }

    while ( ! thread->is_need_exit ) {

        count = gr_poll_wait( poll, events, self->concurrent, TCP_OUT_WAIT_TIMEOUT, thread );
        if ( count < 0 ) {
            gr_fatal( "gr_poll_wait return %d", count );
            continue;
        } else if ( 0 == count ) {
            continue;
        }

        for ( i = 0; i < count; ++ i ) {
            e = & events[ i ];

            conn = (gr_tcp_conn_item_t *)e->data.ptr;
            on_tcp_send( self, poll, thread, conn );
        }
    };

    gr_free( events );
}

#endif // #if ! defined( WIN32 ) && ! defined( WIN64 )

int gr_tcp_out_init()
{
    gr_tcp_out_t *  p;
    bool    tcp_out_disabled= gr_config_tcp_out_disabled();
    int     thread_count    = gr_config_tcp_out_thread_count();
    int     tcp_in_count    = gr_config_tcp_in_thread_count();
    int     udp_in_count    = gr_config_udp_in_thread_count();
    int     r;
    int     i;

    if ( tcp_in_count < 0 || udp_in_count < 0 ) {
        gr_fatal( "[init]tcp_in_count %d or udp_in_count %d invalid",
            tcp_in_count, udp_in_count );
        return GR_ERR_INVALID_PARAMS;
    }

#if ! defined( WIN32 ) && ! defined( WIN64 )
    if ( tcp_out_disabled ) {
        thread_count = tcp_in_count;
    }
#endif
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

    p->concurrent       = gr_config_tcp_out_concurrent();
    p->worker_disabled  = gr_config_worker_disabled();
    p->tcp_out_disabled = gr_config_tcp_out_disabled();

    r = GR_OK;

    do {

        const char * name   = "tcp.output";

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
                p->tcp_out_disabled ? (GR_POLLIN | GR_POLLOUT) : GR_POLLOUT,
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

        {
            gr_poll_t **    tcp_in_polls;
            int             tcp_in_poll_count;

#if defined( WIN32 ) || defined( WIN64 )
            // windows不允许同一个socket同时加到两个iocp里
            // 这里让tcp_out和tcp_in共用IOCP
            tcp_in_polls = (gr_poll_t **)gr_tcp_in_get_polls( & tcp_in_poll_count );
            for ( i = 0; i < tcp_in_poll_count; ++ i ) {
                r = gr_pool_replace_from(
                    p->polls[ i ],
                    tcp_in_polls[ i ]
                );
                if ( 0 != r ) {
                    gr_fatal( "[init]gr_pool_replace_from return error %d", r );
                    r = GR_ERR_INIT_POLL_FALED;
                    break;
                }
            }
            if ( 0 != r ) {
                break;
            }
            gr_info( "[init]tcp.in and tcp.out use same IOCP" );
#else
            if ( p->tcp_out_disabled ) {
                // 如果要禁用tcp_out，则也要让 tcp_out和tcp_in共用 poll
                tcp_in_polls = (gr_poll_t **)gr_tcp_in_get_polls( & tcp_in_poll_count );
                for ( i = 0; i < tcp_in_poll_count; ++ i ) {
                    r = gr_pool_replace_from(
                        p->polls[ i ],
                        tcp_in_polls[ i ]
                    );
                    if ( 0 != r ) {
                        gr_fatal( "[init]gr_pool_replace_from return error %d", r );
                        r = GR_ERR_INIT_POLL_FALED;
                        break;
                    }
                }
                if ( 0 != r ) {
                    break;
                }
                gr_info( "[init]tcp.in and tcp.out use same gr_poll" );
            }
#endif
        }

        r = gr_threads_start(
            & p->threads,
            thread_count,
            NULL,
#if defined( WIN32 ) || defined( WIN64 )
            tcp_io_worker,
#else
            p->tcp_out_disabled ? tcp_io_worker : tcp_out_worker,
#endif
            p,
            gr_poll_raw_buff_for_tcp_out_len(),
            true,
            p->tcp_out_disabled ? DISABLE_THREAD : ENABLE_THREAD,
            name );
        if ( GR_OK != r ) {
            gr_fatal( "[init]gr_threads_start return error %d", r );
            break;
        }

        if ( p->tcp_out_disabled ) {
            gr_info( "[init]tcp.out.disabled = true" );
        } else {
            gr_info( "[init]tcp.out.disabled = false, tcp.out.thread_count = %d", thread_count );
        }

        gr_debug( "[init]tcp_out_init OK" );

        r = GR_OK;
    } while ( false );

    if ( GR_OK != r ) {

        int thread_count = p->threads.thread_count;
        int i;

        gr_threads_close( & p->threads );

        if ( p->polls ) {
            for ( i = 0; i < thread_count; ++ i ) {
                if ( NULL != p->polls[ i ] ) {
#if ! defined( WIN32 ) && ! defined( WIN64 )
                    if ( ! p->tcp_out_disabled ) {
                        gr_poll_destroy( p->polls[ i ] );
                    }
#endif
                    p->polls[ i ] = NULL;
                }
            }

            gr_free( p->polls );
            p->polls = NULL;
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

        int thread_count = p->threads.thread_count;
        int i;

        gr_threads_close( & p->threads );

        if ( p->polls ) {
            for ( i = 0; i < thread_count; ++ i ) {
                if ( NULL != p->polls[ i ] ) {
#if ! defined( WIN32 ) && ! defined( WIN64 )
                    if ( ! p->tcp_out_disabled ) {
                        gr_poll_destroy( p->polls[ i ] );
                    }
#endif
                    p->polls[ i ] = NULL;
                }
            }

            gr_free( p->polls );
            p->polls = NULL;
        }

        gr_free( p );
        g_ghost_rocket_global.tcp_out = NULL;
    }
}

static_inline
size_t
tcp_out_hash(
    gr_tcp_out_t *          self,
    gr_tcp_conn_item_t *    conn
)
{
    return conn->fd % self->threads.thread_count;
}

int gr_tcp_out_add( gr_tcp_rsp_t * rsp )
{
    int                     r;
    gr_tcp_out_t *          self;
    gr_tcp_conn_item_t *    conn;
    gr_poll_t *             poll;

    self = (gr_tcp_out_t *)g_ghost_rocket_global.tcp_out;
    if ( NULL == self ) {
        gr_fatal( "[tcp.output]gr_tcp_out_init never call" );
        return -1;
    }

    conn = rsp->parent;
    if ( NULL == conn ) {
        gr_fatal( "[tcp.output]rsp->parent is NULL" );
        return -1;
    }

    if ( ! conn->tcp_out_open ) {
        //gr_atomic_add( 1, & conn->thread_refs );
        // 该标记表示连接在数据发送线程里
        conn->tcp_out_open = true;
    }

    poll = self->polls[ tcp_out_hash( self, conn ) ];

    // 将该socket加到poll里
    r = gr_poll_add_tcp_send_fd(
        poll,
        conn,
        & self->threads
    );
    if ( 0 != r ) {
        gr_fatal( "[tcp.output]gr_poll_add_tcp_send_fd return %d", r );
        //gr_atomic_add( -1, & conn->thread_refs );
        return -3;
    }

    return 0;
}

int gr_tcp_out_del_tcp_conn( gr_tcp_conn_item_t * conn )
{
    int             r;
    gr_tcp_out_t *  self;
    gr_poll_t *     poll;

    self = (gr_tcp_out_t *)g_ghost_rocket_global.tcp_out;
    if ( NULL == self ) {
        gr_fatal( "[tcp.output]gr_tcp_out_init never call" );
        return -1;
    }

    poll = self->polls[ tcp_out_hash( self, conn ) ];

    r = gr_poll_del_tcp_send_fd(
        poll,
        conn,
        & self->threads
    );
    if ( 0 != r ) {
        gr_fatal( "[tcp.output]gr_poll_del_tcp_send_fd return %d", r );
        return -3;
    }

    return 0;
}

int gr_tcp_out_notify_close( gr_tcp_conn_item_t * conn )
{
    int             r;
    gr_tcp_out_t *  self;
    gr_poll_t *     poll;

    self = (gr_tcp_out_t *)g_ghost_rocket_global.tcp_out;
    if ( NULL == self ) {
        gr_fatal( "[tcp.output]gr_tcp_out_init never call" );
        return -1;
    }

    poll = self->polls[ tcp_out_hash( self, conn ) ];

    // 将该socket加到poll里
    r = gr_poll_add_tcp_send_fd(
        poll,
        conn,
        & self->threads
    );
    if ( 0 != r ) {
        gr_fatal( "[tcp.output]gr_poll_add_tcp_send_fd return %d", r );
        return -3;
    }

    return 0;
}
