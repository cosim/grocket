/**
 * @file libgrocket/gr_poll_windows.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   high performance network event. Windows
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

#include "gr_tcp_in.h"
#include "gr_tcp_out.h"
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
#include "tcp_io.h"

#if defined(WIN32) || defined(WIN64)

// 2013-10-06 13:05 windows不允许同一个socket同时加到两个iocp里

#include <MSWSock.h>
#pragma comment( lib, "mswsock.lib" )

struct gr_poll_t
{
    GR_POLL_EVENT   type;
    HANDLE          iocp;

    int             thread_count;
    // windows下Accept和recv还不一样，真恶心！
    bool            is_accept_thread;
};

typedef struct
{
    DWORD                   transfer_bytes;
    bool                    is_result_ok;
} per_worker_io_t;

typedef struct
{
    OVERLAPPED              overlapped;
    DWORD                   receive_bytes;
    int                     accept_fd;
    bool                    in_accept_ex;
    bool                    is_result_ok;
} per_worker_accept_t;

int gr_poll_raw_buff_for_accept_len()
{
    return sizeof( per_worker_accept_t ) + sizeof( struct sockaddr_in6 ) * 2 + 16 * 2;
}

int gr_poll_raw_buff_for_udp_in_len()
{
    return sizeof( per_worker_io_t );
}

int gr_poll_raw_buff_for_tcp_in_len()
{
    return sizeof( per_worker_io_t );
}

int gr_poll_raw_buff_for_tcp_out_len()
{
    return sizeof( per_worker_io_t );
}

gr_poll_t * gr_poll_create(
    int             concurrent,
    int             thread_count,
    GR_POLL_EVENT   poll_type,
    const char *    name
)
{
    gr_poll_t *     p;
    int             r                   = 0;

    p = (gr_poll_t *)gr_calloc( 1, sizeof( gr_poll_t ) );
    if ( NULL == p ) {
        gr_fatal( "alloc %d failed", (int)sizeof( gr_poll_t ) );
        return NULL;
    }

    do {
        p->thread_count = thread_count;
        p->type         = poll_type;
        p->iocp         = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0 );
        if ( NULL == p->iocp ) {
            gr_fatal( "CreateIoCompletionPort failed: %d", (int)GetLastError() );
            r = -1;
            break;
        }

    } while ( false );

    if ( 0 != r ) {
        gr_poll_destroy( p );
        return NULL;
    }

    return p;
}

void gr_poll_destroy( gr_poll_t * poll )
{
    if ( NULL == poll ) {
        return;
    }

    if ( NULL != poll->iocp ) {
        CloseHandle( poll->iocp );
        poll->iocp = NULL;
    }

    gr_free( poll );
}

static_inline
bool accept_ex_with_thread( gr_poll_t * poll, int fd, gr_thread_t * thread )
{
    per_worker_accept_t *   p   = (per_worker_accept_t *)thread->cookie;
    bool                    b;

    if ( 0 == thread->cookie_len ) {
        thread->cookie_len = (unsigned long)gr_poll_raw_buff_for_accept_len();
        memset( p, 0, thread->cookie_len );
        p->accept_fd = -1;
    }

    if ( p->in_accept_ex ) {
        return true;
    }

    if ( -1 == p->accept_fd ) {
        p->accept_fd = (int)socket( PF_INET, SOCK_STREAM, IPPROTO_TCP );
        if ( -1 == p->accept_fd ) {
            gr_fatal( "socket failed" );
            thread->cookie_len = 0;
            return false;
        }
    }

    p->in_accept_ex     = true;
    p->receive_bytes    = 0;
    p->is_result_ok     = false;

    b = AcceptEx(
        fd, p->accept_fd,
        p + 1,  // 注意 per_worker_accept_t 后面是给 AcceptEx 的缓冲区
        0,
        sizeof( struct sockaddr_in6 ) + 16,
        sizeof( struct sockaddr_in6 ) + 16,
        & p->receive_bytes,
        & p->overlapped );
    if ( ! b ) {
        DWORD e = GetLastError();
        if ( ERROR_IO_PENDING != e ) {
            gr_fatal( "AcceptEx failed, err = %d", (int)e );
            closesocket( p->accept_fd );
            p->accept_fd = -1;
            p->in_accept_ex = false;
            return false;
        }
    }

    return true;
}

static_inline
int hash_accept( gr_poll_t * poll, int fd )
{
    return fd % poll->thread_count;
}

static_inline
bool accept_ex_with_threads( gr_poll_t * poll, int fd, gr_threads_t * threads )
{
    int             hash_id = hash_accept( poll, fd );
    gr_thread_t *   thread  = & threads->threads[ hash_id ];
    return accept_ex_with_thread( poll, fd, thread );
}

int gr_poll_add_listen_fd(
    gr_poll_t * poll,
    bool is_tcp,
    int fd,
    void * data,
    gr_threads_t * threads )
{
    // 加到IOCP里
    if ( ! CreateIoCompletionPort( (HANDLE)(SOCKET)fd, poll->iocp, (ULONG_PTR)data, 0 ) ) {
        gr_fatal( "add sock to iocp failed: %d", get_errno() );
        return -1;
    }

    if ( is_tcp ) {
        // 异步做AcceptEx

        if ( ! poll->is_accept_thread ) {
            poll->is_accept_thread = true;
        }

        if ( ! accept_ex_with_threads( poll, fd, threads ) ) {
            gr_fatal( "accept_ex_with_threads failed" );
            return -2;
        }
    }
    
    return 0;
}

static_inline
bool recv_with_thread(
    gr_poll_t *             poll,
    gr_tcp_conn_item_t *    conn,
    gr_thread_t *           thread
)
{
    gr_tcp_req_t *      req;
    int                 r;
    per_worker_io_t *   p   = (per_worker_io_t *)thread->cookie;

    // 准备req收数据
    req = gr_tcp_conn_prepare_recv( conn, gr_worker_get_thread( true, thread->id ) );
    if ( NULL == req ) {
        gr_fatal( "gr_tcp_conn_prepare_recv return NULL" );
        return false;
    }

    p->is_result_ok         = false;
    p->transfer_bytes       = 0;

    req->iocp_flags         = 0;
    req->iocp_recved        = 0;
    req->iocp_wsabuf.buf    = & req->buf[ req->buf_len ];
    // 为什么有个 - 1? 把最后一个字节留给\0，保证最后以\0结束，解析http协议会方便一些
    req->iocp_wsabuf.len    = (u_long)(req->buf_max - req->buf_len - 1);

    r = WSARecv( conn->fd,
        & req->iocp_wsabuf, 1,
        & req->iocp_recved,
        & req->iocp_flags,
        & req->iocp_overlapped, NULL );
    if ( 0 != r ) {
        DWORD e = WSAGetLastError();
        if ( WSA_IO_PENDING != e && 0 != e ) {
            gr_fatal( "WSARecv failed, %d", (int)e );
            return false;
        } else {
            gr_debug( "after WSARecv, WSAGetLastError()=%d, "
                      "is WSA_IO_PENDING=%d", e, (int)(e == WSA_IO_PENDING) );
        }
    } else {
        gr_debug( "WSARecv return %d", r );
    }

    return true;
}

static_inline
int hash_recv( gr_poll_t * poll, int fd )
{
    return fd % poll->thread_count;
}

static_inline
bool recv_with_threads(
    gr_poll_t *             poll,
    gr_tcp_conn_item_t *    conn,
    gr_threads_t *          threads
)
{
    int             hash_id = hash_recv( poll, conn->fd );
    gr_thread_t *   thread  = & threads->threads[ hash_id ];
    return recv_with_thread( poll, conn, thread );
}

int gr_poll_add_tcp_recv_fd(
    gr_poll_t *             poll,
    gr_tcp_conn_item_t *    conn,
    gr_threads_t *          threads
)
{
    // 加到IOCP里
    if ( ! CreateIoCompletionPort( (HANDLE)(SOCKET)conn->fd, poll->iocp, (ULONG_PTR)conn, 0 ) ) {
        gr_fatal( "add sock to iocp failed: %d", get_errno() );
        return -1;
    }

    // 异步做WSARecv
    if ( ! recv_with_threads( poll, conn, threads ) ) {
        gr_fatal( "recv_with_threads failed" );
        return -2;
    }
    
    return 0;
}

static_inline
bool send_with_thread(
    gr_poll_t *             poll,
    gr_tcp_conn_item_t *    conn,
    gr_thread_t *           thread
)
{
    gr_tcp_rsp_t *      rsp;
    int                 r;
    per_worker_io_t *   p   = (per_worker_io_t *)thread->cookie;

    rsp = gr_tcp_conn_top_rsp( conn );
    if ( NULL == rsp ) {
        gr_fatal( "rsp_list_head is NULL" );
        return false;
    }

    /* if ( conn->close_type < GR_OPENING ) {
        gr_fatal( "connection in disconnecting" );
        return false;
    } */

    p->is_result_ok         = false;
    p->transfer_bytes       = 0;

    rsp->iocp_flags         = 0;
    rsp->iocp_recved        = 0;
    rsp->iocp_wsabuf.buf    = & rsp->buf[ rsp->buf_sent ];
    rsp->iocp_wsabuf.len    = (u_long)(rsp->buf_len - rsp->buf_sent);

    r = WSASend( conn->fd,
        & rsp->iocp_wsabuf, 1,
        & rsp->iocp_sent,
        rsp->iocp_flags,
        & rsp->iocp_overlapped, NULL );
    if ( 0 != r ) {
        DWORD e = WSAGetLastError();
        if ( WSA_IO_PENDING != e && 0 != e ) {
            gr_fatal( "WSASend failed, %d", (int)e );
            return false;
        } else {
            gr_debug( "after WSASend, WSAGetLastError() return %d, not WSA_IO_PENDING", (int)e );
        }
    } else {
        gr_debug( "WSASend %d bytes return %d", (int)rsp->iocp_wsabuf.len, r );
    }

    return true;
}

static_inline int hash_send( gr_poll_t * poll, int fd )
{
    return fd % poll->thread_count;
}

static_inline
bool send_with_threads(
    gr_poll_t *             poll,
    gr_tcp_conn_item_t *    conn,
    gr_threads_t *          threads
)
{
    int             hash_id = hash_send( poll, conn->fd );
    gr_thread_t *   thread  = & threads->threads[ hash_id ];
    return send_with_thread( poll, conn, thread );
}

int gr_poll_add_tcp_send_fd(
    gr_poll_t *             poll,
    gr_tcp_conn_item_t *    conn,
    gr_threads_t *          threads
)
{
    // 因为tcp_in和tcp_out是同一个IOCP，所以不用加
    /*
    // 加到IOCP里
    if ( ! CreateIoCompletionPort( (HANDLE)(SOCKET)conn->fd, poll->iocp, (ULONG_PTR)conn, 0 ) ) {
        gr_fatal( "add sock to iocp failed: %d", get_errno() );
        return -1;
    } */

    // 异步做WSASend
    if ( ! send_with_threads( poll, conn, threads ) ) {
        gr_fatal( "send_with_threads failed" );
        return -2;
    }
    
    return 0;
}

int gr_poll_wait(
    gr_poll_t *         poll,
    gr_poll_event_t *   events,
    int                 event_count,
    int                 timeout,
    gr_thread_t *       thread
)
{
    DWORD           transfer_bytes  = 0;
    ULONG_PTR       comp_key        = 0;
    LPOVERLAPPED    pol             = NULL;
    BOOL            b;
    union {
        per_worker_accept_t *   accept;
        per_worker_io_t *       io;
    } per_worker;
    gr_tcp_req_t *              req;

    b = GetQueuedCompletionStatus(
        poll->iocp,
        & transfer_bytes,
        & comp_key,
        & pol,
        timeout );
    if ( ! b ) {
        DWORD e = GetLastError();
        if ( WAIT_TIMEOUT != e ) {
            gr_fatal( "GetQueuedCompletionStatus failed, err = %d", (int)e );
        }
        return 0;
    }

    // 用户事件
    if ( event_count < 1 ) {
        gr_fatal( "invalid event_count %d", event_count );
        return -1;
    }

    events[ 0 ].data.ptr                = (void*)comp_key;

    if ( poll->is_accept_thread ) {
        per_worker.accept               = (per_worker_accept_t *)thread->cookie;
        per_worker.accept->is_result_ok = true;
        events[ 0 ].events              = GR_POLLIN;
    } else {
        per_worker.io                   = (per_worker_io_t *)thread->cookie;
        per_worker.io->transfer_bytes   = transfer_bytes;
        per_worker.io->is_result_ok     = true;
        if ( sizeof( per_worker_io_t ) != thread->cookie_len ) {
            thread->cookie_len          = sizeof( per_worker_io_t );
        }

        req = OFFSET_RECORD( pol, gr_tcp_req_t, iocp_overlapped );
        if ( req->entry.is_req ) {
            events[ 0 ].events          = GR_POLLIN;
        } else {
            events[ 0 ].events          = GR_POLLOUT;
        }
    }

    return 1;
}

int gr_poll_accept(
    gr_poll_t *         poll,
    gr_thread_t *       thread,
    int                 listen_fd,
    struct sockaddr *   addr,
    int *               addrlen
)
{
    per_worker_accept_t *   p           = (per_worker_accept_t *)thread->cookie;
    int                     accept_fd   = -1;
    bool                    is_result_ok;

    if ( NULL == p || 0 == thread->cookie_len ) {
        gr_error( "invalid thread cookie" );
        errno = EPERM;
        return -1;
    }

    is_result_ok = p->is_result_ok;
    p->is_result_ok = false;

    if ( ! p->in_accept_ex || ! is_result_ok ) {
        errno = EAGAIN;
        //gr_error( "not in accept_ex status %d or is_result_ok %d",
        //    (int)p->in_accept_ex, (int)p->is_result_ok );
        return -2;
    }

    accept_fd = p->accept_fd;

    if ( NULL != addr && addrlen > 0 ) {
        int local_len           = 0;
        int remote_len          = 0;
        LPSOCKADDR local_addr   = NULL;
        LPSOCKADDR remote_addr  = NULL;
        GetAcceptExSockaddrs(
            p + 1,  // 注意 per_worker_accept_t 后面是给 AcceptEx 的缓冲区
            0,
            sizeof( struct sockaddr_in6 ) + 16,
            sizeof( struct sockaddr_in6 ) + 16,
            (SOCKADDR **)& local_addr,
            & local_len,
            (SOCKADDR **)& remote_addr,
            & remote_len );

        if ( remote_len <= * addrlen ) {
            memcpy( addr, remote_addr, remote_len );
            * addrlen = remote_len;
        } else {
            gr_error( "invalid addrlen=%d, remote_len=%d, local_len=%d",
                * addrlen, remote_len, local_len );
            * addrlen = 0;
            closesocket( accept_fd );
            p->in_accept_ex = false;
            p->accept_fd = -1;
            errno = EPERM;
            return -3;
        }
    }

    p->receive_bytes    = 0;
    p->in_accept_ex     = false;
    p->accept_fd        = -1;

    // 异步做AcceptEx
    if ( ! accept_ex_with_thread( poll, listen_fd, thread ) ) {
        gr_fatal( "accept_ex_with_thread failed" );
        closesocket( accept_fd );
        errno = EFAULT;
        return -4;
    }

    gr_debug( "gr_poll_accept OK: %d", accept_fd );
    return accept_fd;
}

int gr_poll_recv(
    gr_poll_t *             poll,
    gr_thread_t *           thread,
    gr_tcp_conn_item_t *    conn,
    gr_tcp_req_t **         req
)
{
    per_worker_io_t *   p       = (per_worker_io_t *)thread->cookie;
    int                 recved  = (int)p->transfer_bytes;

    assert( sizeof( per_worker_io_t ) == thread->cookie_len );

    if ( recved > 0 ) {
        conn->req->buf_len += recved;
        conn->req->buf[ conn->req->buf_len ] = '\0';
#ifdef GR_DEBUG_CONN
        conn->recv_bytes += recved;
#endif
    } else {
        gr_debug( "recved = %d", recved );
    }

    * req = conn->req;
    return recved;
}

int gr_poll_send(
    gr_poll_t *             poll,
    gr_thread_t *           thread,
    gr_tcp_conn_item_t *    conn
)
{
    per_worker_io_t *   p;
    int                 sent;
    bool                is_result_ok;
    gr_tcp_rsp_t *      rsp;

    p                   = (per_worker_io_t *)thread->cookie;
    sent                = (int)p->transfer_bytes;
    is_result_ok        = p->is_result_ok;
    rsp                 = gr_tcp_conn_top_rsp( conn );

    p->is_result_ok     = false;

    if ( NULL == rsp || ! is_result_ok ) {
        // 没什么可发的
        errno = EAGAIN;
        gr_debug( "rsp %p is NULL or is_result_ok %d is zero",
            rsp, (int)is_result_ok );
        return -1;
    }

    assert( sizeof( per_worker_io_t ) == thread->cookie_len );

    //TODO: 我如何知道发失败了?

    if ( rsp && sent > 0 ) {
        rsp->buf_sent += sent;
#ifdef GR_DEBUG_CONN
        conn->send_bytes += sent;
#endif
        gr_debug( "sent = %d, buf_sent = %d", sent, rsp->buf_sent );
    } else {
        gr_debug( "sent %d <= 0, rsp = %p", sent, rsp );
    }

    assert( rsp->buf_sent <= rsp->buf_len );
    if ( rsp->buf_sent == rsp->buf_len ) {
        // 发完了
        int     r;

        // 将发完的回复包弹出，同时把包删了
        r = gr_tcp_conn_pop_top_rsp( conn, rsp );
        if ( 0 != r ) {
            gr_fatal( "gr_tcp_conn_pop_top_rsp return error %d", r );
            conn->is_network_error = 1;
            if ( conn->close_type > GR_NEED_CLOSE ) {
                conn->close_type = GR_NEED_CLOSE;
            }
            return -2;
        }

        gr_debug( "gr_tcp_conn_pop_top_rsp() OK" );
    }

    if ( NULL != gr_tcp_conn_top_rsp( conn ) ) {
        if ( ! send_with_thread( poll, conn, thread ) ) {
            gr_fatal( "send_with_thread failed" );
            conn->is_network_error = 1;
            if ( conn->close_type > GR_NEED_CLOSE ) {
                conn->close_type = GR_NEED_CLOSE;
            }
            return -3;
        }

        gr_debug( "send_with_thread() OK" );
    }

    return sent;
}

int gr_pool_replace_from(
    gr_poll_t *             poll,
    gr_poll_t *             from_poll
)
{
    HANDLE  t;

    if ( NULL == from_poll->iocp ) {
        gr_fatal( "from_poll->iocp is NULL" );
        return -2;
    }

    t = poll->iocp;
    poll->iocp = from_poll->iocp;
    CloseHandle( t );

    return 0;
}

int gr_poll_recv_done(
    gr_poll_t *             poll,
    gr_thread_t *           thread,
    gr_tcp_conn_item_t *    conn,
    bool                    is_ok
)
{
    if ( is_ok && conn->close_type >= GR_OPENING ) {
        // 异步做WSARecv
        if ( ! recv_with_thread( poll, conn, thread ) ) {
            gr_fatal( "recv_with_thread failed" );
            return -1;
        }

        return 0;
    }

    // 如果不OK，需要断连接
    // 在Windows下，后续收包的动作不做了，所以不会收到用户数据包，也就不会有被动发的返回包
    // 对于服务器主动发包的情况，服务器在断之前调用on_tcp_close回调就可以防止模块继续发包了。
    // worker线程里，处理完一个连接的最后一个请求之后即可安全的删除连接对象。
    //TODO: zouyueming 2013-10-12 07:09 还有一种情况，万一从现在开始就已经没有请求包了呢？worker得不到调用而无法删除连接
    //TODO: zouyueming 2013-10-12 07:09 还有如何保证on_tcp_close只调一次呢？要知道这个服务器框架压根就没打算使用锁。
    //                                  其实这个只是服务器主动发包的场景，大部分服务器是被动回复，所以这个目前优先级不高。

    return 0;
}

int gr_poll_del_tcp_recv_fd(
    gr_poll_t *             poll,
    gr_tcp_conn_item_t *    conn,
    gr_threads_t *          threads
)
{
    return GR_OK;
}

int gr_poll_del_tcp_send_fd(
    gr_poll_t *             poll,
    gr_tcp_conn_item_t *    conn,
    gr_threads_t *          threads
)
{
    return GR_OK;
}

int gr_poll_del_tcp_fd(
    gr_poll_t *             poll,
    gr_tcp_conn_item_t *    conn,
    gr_threads_t *          threads
)
{
    return GR_OK;
}

#endif // #if defined(WIN32) || defined(WIN64)
