/**
 * @file libgrocket/gr_tcp_accept.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/03
 * @version $Revision$ 
 * @brief   TCP Accept module
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

#include "gr_tcp_accept.h"
#include "gr_thread.h"
#include "gr_log.h"
#include "gr_global.h"
#include "gr_errno.h"
#include "gr_tools.h"
#include "gr_mem.h"
#include "gr_config.h"
#include "gr_poll.h"
#include "gr_module.h"
#include "gr_socket.h"
#include "gr_conn.h"
#include "gr_tcp_in.h"

// gr_tcp_accept_t 已经是外部接口回调函数声明了, 我只能换一个名字 gr_accept_t
typedef struct
{
    gr_threads_t    threads;

    gr_poll_t *     poll;

    int             concurrent;

} gr_accept_t;

///////////////////////////////////////////////////////////////////////
//
// TCP accept
//

static_inline
void on_tcp_accept(
    gr_accept_t *           self,
    gr_thread_t *           thread,
    gr_port_item_t *        port_item
)
{
    int                     fd;
    gr_tcp_conn_item_t *    conn;
    int                     r;
    union {
        struct sockaddr_in6 v6;
        struct sockaddr_in  v4;
        struct sockaddr     u;
    } addr;
    int                     addr_len;
    
    while ( true ) {

        // accept
        addr_len = (int)sizeof( addr.v6 );
        fd = gr_poll_accept(
            self->poll,
            thread,
            port_item->fd,
            & addr.u, & addr_len );       // 这个阶段我根本不想要对方地址，爱谁谁
        if ( fd < 0 ) {
            if ( EAGAIN == errno ) {
                return;
            }

            gr_error( "gr_poll_accept failed: %d,%s", errno, strerror( errno ) );
            return;
        }

        gr_debug( "tcp_in accept( %d ) return fd %d, addr=%s:%d",
            port_item->fd, fd, inet_ntoa(addr.v4.sin_addr), ntohs(addr.v4.sin_port) );

        // 设成异步socket
        if ( ! gr_socket_set_block( fd, false ) ) {
            gr_error( "gr_socket_set_block failed: %d", get_errno() );
            gr_socket_close( fd );
            continue;
        }
 
        // 分配连接对象
        conn = gr_tcp_conn_alloc( port_item, fd );
        if ( NULL == conn ) {
            gr_error( "gr_conn_alloc_tcp failed" );
            gr_socket_close( fd );
            continue;
        }

        // 看模块喜不喜欢这个连接
        if ( ! gr_module_on_tcp_accept( conn ) ) {
            // 不喜欢，关掉就是了
            gr_error( "gr_module_on_tcp_accept reject" );
            gr_tcp_conn_free( conn );
            continue;
        }

        // 将该socket加到tcp_in里
        r = gr_tcp_in_add_conn( conn );
        if ( 0 != r ) {
            gr_fatal( "gr_tcp_accept_add_conn return %d", r );
            gr_tcp_conn_free( conn );
            continue;
        }
    }
}

void tcp_accept_worker( gr_thread_t * thread )
{
#define     TCP_ACCEPT_WAIT_TIMEOUT    100
    int                     count;
    int                     i;
    gr_accept_t *           self;
    gr_poll_event_t *       events;
    gr_poll_event_t *       e;
    gr_server_t *           server;
    gr_port_item_t *        port_item;
    //gr_tcp_conn_item_t *    conn;

    server = & g_ghost_rocket_global.server_interface;
    self    = (gr_accept_t *)thread->param;

    events  = (gr_poll_event_t *)gr_malloc( sizeof( gr_poll_event_t ) * self->concurrent );
    if ( NULL == events ) {
        gr_fatal( "bad_alloc %d", (int)sizeof( gr_poll_event_t ) * self->concurrent );
        return;
    }

    while ( ! thread->is_need_exit ) {

        count = gr_poll_wait( self->poll, events, self->concurrent, TCP_ACCEPT_WAIT_TIMEOUT, thread );
        if ( count < 0 ) {
            gr_fatal( "gr_poll_wait return %d", count );
            continue;
        }

        for ( i = 0; i < count; ++ i ) {
            e = & events[ i ];
            
            if (   (gr_port_item_t*)e->data.ptr >= & server->ports[ 0 ]
                && (gr_port_item_t*)e->data.ptr <= & server->ports[ server->ports_count - 1 ] )
            {
                // TCP新连接
                port_item = (gr_port_item_t *)e->data.ptr;
                on_tcp_accept( self, thread, port_item );
            } else {
                // 其它命令
            }
        }
    };

    gr_free( events );
}

int gr_tcp_accept_init()
{
    gr_accept_t *  p;
    int thread_count = gr_config_tcp_accept_thread_count();
    int r;

    if ( thread_count < 1 ) {
        gr_fatal( "[init]tcp_accept thread_count invalid" );
        return GR_ERR_INVALID_PARAMS;
    }

    if ( NULL != g_ghost_rocket_global.tcp_accept ) {
        gr_fatal( "[init]gr_tcp_accept_init already called" );
        return GR_ERR_WRONG_CALL_ORDER;
    }

    p = (gr_accept_t *)gr_calloc( 1, sizeof( gr_accept_t ) );
    if ( NULL == p ) {
        gr_fatal( "[init]malloc %d bytes failed, errno=%d,%s",
            (int)sizeof(gr_accept_t), errno, strerror( errno ) );
        return GR_ERR_BAD_ALLOC;
    }

    p->concurrent = gr_config_tcp_accept_concurrent();

    r = GR_OK;

    do {

        const char * name = "tcp.listen";

        p->poll = gr_poll_create( p->concurrent, thread_count, GR_POLLIN, name );
        if ( NULL == p->poll ) {
            r = GR_ERR_INIT_POLL_FALED;
            break;
        }

        r = gr_threads_start(
            & p->threads,
            thread_count,
            NULL,
            tcp_accept_worker,
            p,
            gr_poll_raw_buff_for_accept_len(),
            true,
            ENABLE_THREAD,
            name );
        if ( GR_OK != r ) {
            break;
        }

        gr_debug( "tcp_accept_init OK" );

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

    g_ghost_rocket_global.tcp_accept = p;
    return GR_OK;
}

void gr_tcp_accept_term()
{
    gr_accept_t *  p = (gr_accept_t *)g_ghost_rocket_global.tcp_accept;
    if ( NULL != p ) {

        gr_threads_close( & p->threads );

        // 线程停了就可以把全局变量清掉了
        g_ghost_rocket_global.tcp_accept = NULL;

        if ( NULL != p->poll ) {
            gr_poll_destroy( p->poll );
            p->poll = NULL;
        }

        gr_free( p );
    }
}

int gr_tcp_accept_add_listen_ports()
{
    int             i;
    int             r               = 0;
    gr_server_t *   server_interface= & g_ghost_rocket_global.server_interface;
    gr_accept_t *   self            = (gr_accept_t *)g_ghost_rocket_global.tcp_accept;

    if ( NULL == self ) {
        gr_fatal( "[init]tcp_accept is NULL" );
        return -1;
    }

    for ( i = 0; i < server_interface->ports_count; ++ i ) {
        gr_port_item_t * item = & server_interface->ports[ i ];

        if ( item->is_tcp ) {
            r = gr_poll_add_listen_fd( self->poll, item->is_tcp, item->fd, item, & self->threads );
            if ( 0 != r ) {
                gr_fatal( "[init]gr_poll_add_listen_fd return %d", r );
                return -2;
            }

            gr_info( "[init]start listen TCP port %d", item->port );
            printf( "[init]start listen TCP port %d\n", item->port );
        }
    }

    return 0;
}
