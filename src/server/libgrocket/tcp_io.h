/**
 * @file libgrocket/tcp_io.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/07
 * @version $Revision$ 
 * @brief   TCP recv and send implement
 * Revision History
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-07    Created.
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

#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_TCP_IO_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_TCP_IO_H_

// 万恶的Windows！让本应该无需公开的数据结构和代码被公开！FUCK！
// windows不允许同一个socket同时加到两个iocp里，所以我必须
// 把 tcp_in 和 tcp_out 在Windows下合并，其它平台不受影响。
// 收发处理代码还必须是一份。所以有了这个文件

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
#include "gr_worker.h"
#include "gr_tcp_close.h"

///////////////////////////////////////////////////////////////////////
//
// 为了容易处理，我把gr_tcp_in_t 和gr_tcp_out_t 定义统一在了一起
//

typedef struct
{
    // thread group
    gr_threads_t    threads;
    // a poll for each thread
    gr_poll_t **    polls;

    int             concurrent;
    bool            is_tcp_in;
    bool            worker_disabled;
    bool            tcp_out_disabled;

} gr_tcp_io_t;

typedef gr_tcp_io_t gr_tcp_in_t;
typedef gr_tcp_io_t gr_tcp_out_t;

///////////////////////////////////////////////////////////////////////
//
// below should be tcp_out's private code
//

static_inline
void on_tcp_send_error(
    gr_tcp_out_t *          self,
    gr_poll_t *             poll,
    gr_thread_t *           thread,
    gr_tcp_conn_item_t *    conn
)
{
    if ( ! conn->is_network_error ) {
        conn->is_network_error = true;
    }

    if ( conn->close_type > GR_NEED_CLOSE ) {
        conn->close_type = GR_NEED_CLOSE;
    }

    // 网络异常时已经不可能有返回数据包被压过来，完全可以把返回数据包列表删了
    // 为什么要加这句话呢？因为代码(1)处只在 NULL == conn->rsp_list_head 时才删连接
    // 希望走到本分支时，在(1)处可以走到删除连接的逻辑。
    gr_tcp_conn_clear_rsp_list( conn );

    if ( self->tcp_out_disabled ) {
        // 如果 tcp_out 已经禁用，则 tcp_out 退出，tcp_in 就已经退出了
        conn->tcp_in_open = false;
    }
    // 关连接，然后退出
    gr_tcp_close_from_out( conn, self->tcp_out_disabled );
}

static_inline
void on_tcp_send(
    gr_tcp_out_t *          self,
    gr_poll_t *             poll,
    gr_thread_t *           thread,
    gr_tcp_conn_item_t *    conn
)
{
    if ( ! conn->is_network_error ) {

        // 如果网络状态没出错, 则继续发数据
        // 这儿不能判断 close_type，因为即使 close_type 想关连接，
        // 也要将已经接收了的请求处理后的返回包发走。

        int             r;

        // 发数据，里面已经做循环发了
        r = gr_poll_send( poll, thread, conn );
        if ( r < 0 ) {
            if ( EAGAIN != errno ) {
                // 要么网络出错要么对方断连接了
                gr_error( "[%s]gr_poll_send return error %d", thread->name, r );
                assert( conn->close_type <= GR_NEED_CLOSE );
                // 网络异常时已经不可能有返回数据包被压过来，完全可以把返回数据包列表删了
                gr_tcp_conn_clear_rsp_list( conn );
                if ( self->tcp_out_disabled ) {
                    // 如果 tcp_out 已经禁用，则 tcp_out 退出，tcp_in 就已经退出了
                    conn->tcp_in_open = false;
                }
                // 关连接，然后退出
                gr_tcp_close_from_out( conn, self->tcp_out_disabled );
            }
            return;
        }

        //TODO: r 的值如果是0表示什么?
        gr_debug( "[%s][sent=%d]gr_poll_send OK", thread->name, r );
    } else {
        // 如果网络异常时，close_type 状态至少是正在关闭过程中
        assert( conn->close_type <= GR_NEED_CLOSE );
        // 网络异常时已经不可能有返回数据包被压过来，完全可以把返回数据包列表删了
        // 为什么要加这句话呢？因为代码(1)处只在 NULL == conn->rsp_list_head 时才删连接
        // 希望走到本分支时，在(1)处可以走到删除连接的逻辑。
        gr_tcp_conn_clear_rsp_list( conn );
    }

    if ( conn->close_type <= GR_NEED_CLOSE ) {
        //(1) 如果连接需要关闭
        if ( NULL == conn->rsp_list_head ) {
            // 如果所有待发数据包都已经发完，则关连接
            gr_error( "[%s]conn->close_type is %d and conn->rsp_list_head is NULL",
                thread->name, (int)conn->close_type );
            if ( self->tcp_out_disabled ) {
                // 如果 tcp_out 已经禁用，则 tcp_out 退出，tcp_in 就已经退出了
                conn->tcp_in_open = false;
            }
            gr_tcp_close_from_out( conn, self->tcp_out_disabled );
        }
    }
}

///////////////////////////////////////////////////////////////////////
//
// 这本应该是tcp_in的私有代码
//
static_inline
void on_tcp_recv_error(
    gr_tcp_in_t *           self,
    gr_poll_t *             poll,
    gr_thread_t *           thread,
    gr_tcp_conn_item_t *    conn
)
{
    int r;
    
    // 先把当前连接在当前接收poll中停掉
    r = gr_poll_recv_done( poll, thread, conn, false );
    if ( 0 != r ) {
        gr_error( "[%s]gr_poll_recv_done return error %d", thread->name, r );
    }

    if ( self->tcp_out_disabled ) {
        // 如果 tcp_out 已经禁用，则 tcp_in 退出，tcp_out 就已经退出了
        conn->tcp_out_open = false;
    }
    // 关连接
    gr_tcp_close_from_in( conn, self->tcp_out_disabled );
}

static_inline
void on_tcp_not_full(
    gr_tcp_in_t *           self,
    gr_poll_t *             poll,
    gr_thread_t *           thread,
    gr_tcp_conn_item_t *    conn
)
{
    int r;
    
    r = gr_poll_recv_done( poll, thread, conn, true );
    if ( 0 != r ) {
        gr_error( "[%s]gr_poll_recv_done return error %d", thread->name, r );
        on_tcp_recv_error( self, poll, thread, conn );
    }
}

static_inline
void on_tcp_full(
    gr_tcp_in_t *           self,
    gr_poll_t *             poll,
    gr_thread_t *           thread,
    gr_tcp_conn_item_t *    conn,
    gr_tcp_req_t *          req
)
{
    // 将当前req扔到工作线程中去
    // 当前连接的包继续收
    int     r;
 
    if ( ! self->worker_disabled ) {
        // 启动 worker
        r = gr_worker_add_tcp( req );
        if ( 0 != r ) {
            gr_fatal( "[%s]gr_worker_add_tcp return error %d", thread->name, r );
            on_tcp_recv_error( self, poll, thread, conn );
            return;
        }
    } else {
        // 禁用 worker。直接在接收线程处理请求
        r = gr_worker_process_tcp( req );
        if ( 0 != r ) {
            gr_fatal( "[%s]gr_worker_process_tcp return error %d", thread->name, r );
            on_tcp_recv_error( self, poll, thread, conn );
            return;
        }
    }

    // 收完了，事后通知
    r = gr_poll_recv_done( poll, thread, conn, true );
    if ( 0 != r ) {
        gr_error( "[%s]gr_poll_recv_done return error %d", thread->name, r );
        on_tcp_recv_error( self, poll, thread, conn );
        return;
    }
}

static_inline
void on_tcp_recv(
    gr_tcp_in_t *           self,
    gr_poll_t *             poll,
    gr_thread_t *           thread,
    gr_tcp_conn_item_t *    conn
)
{
    int             r;
    gr_tcp_req_t *  req         = NULL;
    bool            is_error    = false;
    bool            is_full     = false;

    if ( conn->is_network_error || conn->close_type <= GR_NEED_CLOSE ) {
        // 如果网络已经不可用，或者正在关闭过程中，则不能再收数据了。
        gr_warning( "[%s]is_network_error=%d or conn->close_type=%d, GR_NEED_CLOSE=%d, error",
            thread->name, (int)conn->is_network_error, (int)conn->close_type, GR_NEED_CLOSE);
        on_tcp_recv_error( self, poll, thread, conn );
        return;
    }
    
    // 收数据，里面已经在循环里一直收到没数据了
    r = gr_poll_recv( poll, thread, conn, & req );
    if ( r <= 0 ) {
        // 要么网络出错要么对方断连接了
        if ( 0 == r ) {
            gr_info( "[%s]recv %d bytes", thread->name, r );
        } else {
            gr_error( "[%s]gr_poll_recv return error %d", thread->name, r );
        }
        on_tcp_recv_error( self, poll, thread, conn );
        return;
    }

    // 本次收到了r字节数据，判断数据包是否完整
    gr_module_check_tcp( req, & is_error, & is_full );
    if ( is_error ) {
        // 协议错
        gr_error( "[%s]gr_module_check_tcp is_error is true", thread->name );
        on_tcp_recv_error( self, poll, thread, conn );
        return;
    }
    if ( ! is_full ) {
        on_tcp_not_full( self, poll, thread, conn );
        return;
    }

    // 收到了一个完整数据包
    on_tcp_full( self, poll, thread, conn, req );
}

//

static void tcp_io_worker( gr_thread_t * thread )
{
    // 现在收、发线程都调这个线程函数了  
#define     TCP_IO_WAIT_TIMEOUT    100
    int                     count;
    int                     i;
    gr_tcp_io_t *           self;
    gr_poll_event_t *       events;
    gr_poll_event_t *       e;
    gr_tcp_conn_item_t *    conn;
    gr_poll_t *             poll;

    self    = (gr_tcp_io_t *)thread->param;
    poll    = self->polls[ thread->id ];

    events  = (gr_poll_event_t *)gr_malloc( sizeof( gr_poll_event_t ) * self->concurrent );
    if ( NULL == events ) {
        gr_fatal( "[%s]bad_alloc %d",
            thread->name, (int)sizeof( gr_poll_event_t ) * self->concurrent );
        return;
    }

    while ( ! thread->is_need_exit ) {

        count = gr_poll_wait( poll, events, self->concurrent, TCP_IO_WAIT_TIMEOUT, thread );
        if ( count < 0 ) {
            gr_fatal( "[%s]gr_poll_wait return %d", thread->name, count );
            continue;
        } else if ( 0 == count ) {
            continue;
        }

        for ( i = 0; i < count; ++ i ) {
            e = & events[ i ];

            conn = (gr_tcp_conn_item_t *)e->data.ptr;

            if ( e->events & GR_POLLIN ) {
                on_tcp_recv( self, poll, thread, conn );
            } else if ( e->events & GR_POLLOUT ) {
                on_tcp_send( self, poll, thread, conn );
            } else if ( e->events & GR_POLLERR ) {
                if ( self->is_tcp_in ) {
                    on_tcp_recv_error( self, poll, thread, conn );
                } else {
                    on_tcp_send_error( self, poll, thread, conn );
                }
            }
        }
    };

    gr_free( events );
}

#endif // #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_TCP_IO_H_
