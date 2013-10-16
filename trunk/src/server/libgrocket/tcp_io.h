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

#ifndef _tcp_io_h_
#define _tcp_io_h_

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
    gr_threads_t    threads;

    gr_poll_t *     poll;

    int             concurrent;

} gr_tcp_io_t;

typedef gr_tcp_io_t gr_tcp_in_t;
typedef gr_tcp_io_t gr_tcp_out_t;

#if defined( WIN32 ) || defined( WIN64 )

// windows 下，tcp_in 线程和 tcp_out 线程都使用的相同的线程函数。
void tcp_io_windows( gr_thread_t * thread );

#endif // #if defined( WIN32 ) || defined( WIN64 )

///////////////////////////////////////////////////////////////////////
//
// 这本应该是tcp_out的私有代码
//

static inline
void on_tcp_send_error(
    gr_tcp_in_t *           self,
    gr_thread_t *           thread,
    gr_tcp_conn_item_t *    conn
)
{
    int r;
    
    // 先把当前连接在当前接收poll中停掉
    r = gr_poll_send_failed( self->poll, thread, conn );
    if ( 0 != r ) {
        gr_error( "gr_poll_send_failed return error %d", r );
    }

    // 关连接
    gr_tcp_close_from_out( conn );
}

static inline
void on_tcp_send(
    gr_tcp_out_t *          self,
    gr_thread_t *           thread,
    gr_tcp_conn_item_t *    conn
)
{
    int             r;

    // 发数据，里面已经做循环发了
    r = gr_poll_send( self->poll, thread, conn );
    if ( r < 0 ) {
        // 要么网络出错要么对方断连接了
        gr_error( "gr_poll_send return error %d", r );
        on_tcp_send_error( self, thread, conn );
        return;
    }

    gr_info( "on_tcp_send called" );
}

///////////////////////////////////////////////////////////////////////
//
// 这本应该是tcp_in的私有代码
//
static inline
void on_tcp_recv_error(
    gr_tcp_in_t *           self,
    gr_thread_t *           thread,
    gr_tcp_conn_item_t *    conn
)
{
    int r;
    
    // 先把当前连接在当前接收poll中停掉
    r = gr_poll_recv_done( self->poll, thread, conn, false );
    if ( 0 != r ) {
        gr_error( "gr_poll_recv_done return error %d", r );
    }

    // 关连接
    gr_tcp_close_from_in( conn );
}

static inline
void on_tcp_not_full(
    gr_tcp_in_t *           self,
    gr_thread_t *           thread,
    gr_tcp_conn_item_t *    conn
)
{
    int r;
    
    r = gr_poll_recv_done( self->poll, thread, conn, true );
    if ( 0 != r ) {
        gr_error( "gr_poll_recv_done return error %d", r );
        on_tcp_recv_error( self, thread, conn );
    }
}

static inline
void on_tcp_full(
    gr_tcp_in_t *           self,
    gr_thread_t *           thread,
    gr_tcp_conn_item_t *    conn,
    gr_tcp_req_t *          req
)
{
    // 将当前req扔到工作线程中去
    // 当前连接的包继续收
    int     r;
 
    r = gr_worker_add_tcp( req, false );
    if ( 0 != r ) {
        gr_fatal( "gr_worker_add_tcp return error %d", r );
        on_tcp_recv_error( self, thread, conn );
        return;
    }

    // 收完了，事后通知
    r = gr_poll_recv_done( self->poll, thread, conn, true );
    if ( 0 != r ) {
        gr_error( "gr_poll_recv_done return error %d", r );
        on_tcp_recv_error( self, thread, conn );
        return;
    }
}

static inline
void on_tcp_recv(
    gr_tcp_in_t *           self,
    gr_thread_t *           thread,
    gr_tcp_conn_item_t *    conn
)
{
    int             r;
    gr_tcp_req_t *  req = NULL;
    bool            is_error = false;
    bool            is_full = false;

    // 收数据，里面已经在循环里一直收到没数据了
    r = gr_poll_recv( self->poll, thread, conn, & req );
    if ( r <= 0 ) {
        // 要么网络出错要么对方断连接了
        if ( 0 == r ) {
            gr_info( "user disconnect connection" );
        } else {
            gr_error( "gr_poll_recv return error %d", r );
        }
        on_tcp_recv_error( self, thread, conn );
        return;
    }

    // 本次收到了r字节数据，判断数据包是否完整
    gr_module_check_tcp( req, & is_error, & is_full );
    if ( is_error ) {
        // 协议错
        gr_error( "gr_module_check_tcp is_error is true" );
        on_tcp_recv_error( self, thread, conn );
        return;
    }
    if ( ! is_full ) {
        on_tcp_not_full( self, thread, conn );
        return;
    }

    // 收到了一个完整数据包
    on_tcp_full( self, thread, conn, req );
}

#endif // #ifndef _tcp_io_h_