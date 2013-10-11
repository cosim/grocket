/**
 * @file libgrocket/gr_poll_windows.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   高并发事件处理Windows版
 * Revision History 大事件记
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-05    Created.
 **/
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

static inline
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

static inline int hash_accept( gr_poll_t * poll, int fd )
{
    return fd % poll->thread_count;
}

static inline
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

static inline
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
    req = gr_tcp_conn_prepare_recv( conn );
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
        }
    }

    return true;
}

static inline int hash_recv( gr_poll_t * poll, int fd )
{
    return fd % poll->thread_count;
}

static inline
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

static inline
bool send_with_thread(
    gr_poll_t *             poll,
    gr_tcp_conn_item_t *    conn,
    gr_thread_t *           thread
)
{
    gr_tcp_rsp_t *      rsp;
    int                 r;
    per_worker_io_t *   p   = (per_worker_io_t *)thread->cookie;

    rsp = conn->rsp_list_head;
    if ( NULL == rsp ) {
        gr_fatal( "rsp_list_head is NULL" );
        return false;
    }

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
        }
    }

    return true;
}

static inline int hash_send( gr_poll_t * poll, int fd )
{
    return fd % poll->thread_count;
}

static inline
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
        if ( req->entry_compact.is_req ) {
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
    rsp                 = conn->rsp_list_head;

    p->is_result_ok     = false;

    if ( NULL == rsp || ! is_result_ok ) {
        // 没什么可发的
        errno = EAGAIN;
        return -1;
    }

    assert( sizeof( per_worker_io_t ) == thread->cookie_len );

    if ( rsp && sent > 0 ) {

        rsp->buf_sent += sent;
        if ( rsp->buf_sent == rsp->buf_len ) {
            // 发完了
            int     r;

            // 将发完的回复包弹出
            r = gr_tcp_conn_pop_top_rsp( conn, rsp );
            if ( 0 != r ) {
                gr_fatal( "gr_tcp_conn_pop_top_rsp return error %d", r );
                return -1;
            }
        }

        if ( NULL != conn->rsp_list_head ) {
            if ( ! send_with_thread( poll, conn, thread ) ) {
                gr_fatal( "send_with_thread failed" );
                return -2;
            }
        }
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

void tcp_io_windows( gr_thread_t * thread )
{
#define     TCP_IO_WAIT_TIMEOUT    100
    int                     count;
    int                     i;
    gr_tcp_io_t *           self;
    gr_poll_event_t *       events;
    gr_poll_event_t *       e;
    gr_tcp_conn_item_t *    conn;

    self    = (gr_tcp_io_t *)thread->param;

    events  = (gr_poll_event_t *)gr_malloc( sizeof( gr_poll_event_t ) * self->concurrent );
    if ( NULL == events ) {
        gr_fatal( "bad_alloc %d", (int)sizeof( gr_poll_event_t ) * self->concurrent );
        return;
    }

    while ( ! thread->is_need_exit ) {

        count = gr_poll_wait( self->poll, events, self->concurrent, TCP_IO_WAIT_TIMEOUT, thread );
        if ( count < 0 ) {
            gr_fatal( "gr_poll_wait return %d", count );
            continue;
        } else if ( 0 == count ) {
            continue;
        }

        for ( i = 0; i < count; ++ i ) {
            e = & events[ i ];

            conn = (gr_tcp_conn_item_t *)e->data.ptr;

            if ( e->events & GR_POLLIN ) {
                on_tcp_recv( self, thread, conn );
            } else if ( e->events & GR_POLLOUT ) {
                on_tcp_send( self, thread, conn );
            }
        }
    };

    gr_free( events );
}

int gr_poll_recv_done(
    gr_poll_t *             poll,
    gr_thread_t *           thread,
    gr_tcp_conn_item_t *    conn,
    bool                    is_ok
)
{
    if ( is_ok ) {
        // 异步做WSARecv
        if ( ! recv_with_thread( poll, conn, thread ) ) {
            gr_fatal( "recv_with_thread failed" );
            return -1;
        }

        return 0;
    }

    //TODO: 如果不OK，需要断连接
    if ( conn->close_type >= GR_NEED_CLOSE ) {
        conn->close_type >= GR_CLOSING;
    }
    return 0;
}

#endif // #if defined(WIN32) || defined(WIN64)
