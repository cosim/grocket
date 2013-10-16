/**
 * @file libgrocket/gr_poll_bsd.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/07
 * @version $Revision$ 
 * @brief   high performance network event. BSD & Mac OS X
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

#include "gr_poll.h"
#include "gr_log.h"
#include "gr_global.h"
#include "gr_errno.h"
#include "gr_tools.h"
#include "gr_mem.h"

#if defined( __FreeBSD__ ) || defined( __APPLE__ )

#include <sys/event.h>

struct gr_poll_t
{
    int             kqfd;

    const char *    name;
};

int gr_poll_raw_buff_for_accept_len()
{
    return 0;
}

int gr_poll_raw_buff_for_udp_in_len()
{
    return 0;
}

int gr_poll_raw_buff_for_tcp_in_len()
{
    return 0;
}

int gr_poll_raw_buff_for_tcp_out_len()
{
    return 0;
}

gr_poll_t * gr_poll_create(
    int             concurrent,
    int             thread_count,
    GR_POLL_EVENT   poll_type,
    const char *    name
)
{
    gr_poll_t * p;
    int r = 0;

    p = (gr_poll_t *)gr_calloc( 1, sizeof( gr_poll_t ) );
    if ( NULL == p ) {
        gr_fatal( "alloc %d failed", (int)sizeof( gr_poll_t ) );
        return NULL;
    }

    do {

        p->kqfd         = -1;
        p->name         = name;

        p->kqfd = kqueue();
        if ( -1 == p->kqfd ) {
            gr_fatal( "kqueue() failed: %d,%s", (int)errno, strerror(errno) );
            r = -2;
            break;
        }

        gr_debug( "%s kqueue() OK. kqfd = %d", p->name, p->kqfd );

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

    if ( -1 != poll->kqfd ) {
        gr_debug( "%s close( %d )", poll->name, poll->kqfd );

        close( poll->kqfd );
        poll->kqfd = -1;
    }

    gr_free( poll );
}

int gr_poll_add_listen_fd(
    gr_poll_t * poll,
    bool is_tcp,
    int fd,
    void * data,
    gr_threads_t * threads )
{
    struct kevent   ev;
    struct timespec ts;
    int             r;

    // 将 accept 加入 kqueue
    EV_SET( & ev,
        fd,                 // ident
        EVFILT_READ,        // filter
        EV_ADD,             // flags
        0,                  // fflags
        0,                  // data
        data );   // udata

    ts.tv_sec = 0;
    ts.tv_nsec = 0;
    r = kevent( poll->kqfd, & ev, 1, NULL, 0, & ts );
    if ( -1 == r ) {
        gr_fatal( "kevent return error %d: %d,%s", r, errno, strerror( errno ) );
        return r;
    }

    gr_debug( "%s kevent( %d, EVFILT_READ, EV_ADD, %d, %p ) listen ok",
        poll->name, poll->kqfd, fd, data );
    return 0;
}

int gr_poll_add_tcp_recv_fd(
    gr_poll_t *             poll,
    gr_tcp_conn_item_t *    conn,
    gr_threads_t *          threads
)
{
    struct kevent   ev;
    struct timespec ts;
    int             r;

    EV_SET( & ev,
        conn->fd,           // ident
        EVFILT_READ,        // filter
        EV_ADD,             // flags
        0,                  // fflags
        0,                  // data
        conn );   // udata
    ts.tv_sec = 0;
    ts.tv_nsec = 0;
    r = kevent( poll->kqfd, & ev, 1, NULL, 0, & ts );
    if ( -1 == r ) {
        gr_fatal( "%s kevent return error %d: %d,%s", poll->name, r, errno, strerror( errno ) );
        return r;
    }

    gr_debug( "%s kevent( %d, EVFILT_READ, EV_ADD, %d, %p ) recv ok",
        poll->name, poll->kqfd, conn->fd, conn );
    return 0;
}

int gr_poll_add_tcp_send_fd(
    gr_poll_t *             poll,
    gr_tcp_conn_item_t *    conn,
    gr_threads_t *          threads
)
{
    struct kevent   ev;
    struct timespec ts;
    int             r;

    ts.tv_sec = 0;
    ts.tv_nsec = 0;

    EV_SET( & ev,
        conn->fd,           // ident
        EVFILT_WRITE,       // filter
        EV_ADD,// flags
        0,                  // fflags
        0,                  // data
        conn );             // udata
    r = kevent( poll->kqfd, & ev, 1, NULL, 0, & ts );
    if ( -1 == r ) {
        gr_fatal( "%s kevent return error %d: %d,%s", poll->name, r, errno, strerror( errno ) );
        return r;
    }

    gr_debug( "%s kevent( %d, EVFILT_READ, EV_ADD, %d, %p ) recv ok",
        poll->name, poll->kqfd, conn->fd, conn );
    return 0;
}

static inline
int del_tcp_send_fd(
    gr_poll_t *             poll,
    gr_tcp_conn_item_t *    conn
)
{
    struct kevent   ev;
    struct timespec ts;
    int             r;

    ts.tv_sec = 0;
    ts.tv_nsec = 0;

    EV_SET( & ev,
        conn->fd,           // ident
        EVFILT_WRITE,       // filter
        EV_DELETE,         // flags
        0,                  // fflags
        0,                  // data
        conn );             // udata
    r = kevent( poll->kqfd, & ev, 1, NULL, 0, & ts );
    if ( -1 == r ) {
        gr_fatal( "%s kevent return error %d: %d,%s", poll->name, r, errno, strerror( errno ) );
        return r;
    }

    gr_debug( "%s kevent( %d, EVFILT_WRITE, EV_DELETE, %d, %p ) recv ok",
        poll->name, poll->kqfd, conn->fd, conn );
    return 0;
}

static inline
void del_tcp_recv_fd(
    gr_poll_t *             poll,
    gr_tcp_conn_item_t *    conn
)
{
    struct kevent   ev;
    struct timespec ts;
    int             r;
    int             fd;

    fd = conn->fd;

    if ( -1 != fd ) {
        ts.tv_sec = 0;
        ts.tv_nsec = 0;
        EV_SET( & ev,
            fd,                 // ident
            EVFILT_READ,        // filter
            EV_DELETE,          // flags
            0,                  // fflags
            0,                  // data
            conn );             // udata
        r = kevent( poll->kqfd, & ev, 1, NULL, 0, & ts );
        if ( 0 != r ) {
            gr_warning( "%s kevent EV_DELETE return error %d: %d,%s", poll->name, r, errno, strerror( errno ) );
        } else {
            gr_info( "%s kevent EV_DELETE for EVFILT_READ ok", poll->name );
        }
    }
}

int gr_poll_wait(
    gr_poll_t *         poll,
    gr_poll_event_t *   events,
    int                 event_count,
    int                 timeout,
    gr_thread_t *       thread
)
{
    //TODO: 一次只给一个事件，以后再优化
    int             r;
    struct kevent   event;
    struct timespec ts;

    ts.tv_sec = timeout / 1000;
    ts.tv_nsec = (timeout % 1000) * 1000000;
    if ( ts.tv_nsec > 1000000000 ) {
        ++ ts.tv_sec;
        ts.tv_nsec -= 1000000000;
    }

    r = kevent( poll->kqfd, NULL, 0, & event, 1, & ts );
    if ( r < 0 ) {
        if ( EINTR == errno ) {
            r = 0;
        } else {
            gr_error( "kevent %d return %d, and errno is %d,%s",
                poll->kqfd, r, errno, strerror(errno) );
        }
    } else if ( r > 0 ) {
        assert( 1 == r );

        switch ( event.filter )
        {
        case EVFILT_READ:
            events[ 0 ].events      = GR_POLLIN;
            events[ 0 ].data.ptr    = event.udata;
            break;
        case EVFILT_WRITE:
            events[ 0 ].events      = GR_POLLOUT;
            events[ 0 ].data.ptr    = event.udata;
            break;
        default:
            gr_error( "%s kevent %d invalid filter %d", poll->name, poll->kqfd, (int)event.filter );
            r = 0;
            break;
        }
    }

    return r;
}

int gr_poll_accept(
    gr_poll_t *         poll,
    gr_thread_t *       thread,
    int                 listen_fd,
    struct sockaddr *   addr,
    int *               addrlen
)
{
    return accept( listen_fd, addr, addrlen );
}

int gr_poll_recv(
    gr_poll_t *             poll,
    gr_thread_t *           thread,
    gr_tcp_conn_item_t *    conn,
    gr_tcp_req_t **         ret_req
)
{
    gr_tcp_req_t *  req;
    int             r;
    int             recv_bytes = 0;
    
    while ( true ) {

        // 准备req收数据
        req = gr_tcp_conn_prepare_recv( conn );
        if ( NULL == req ) {
            gr_fatal( "gr_tcp_conn_prepare_recv return NULL" );
            return -1;
        }

        r = recv(
            conn->fd,
            (char*)& req->buf[ req->buf_len ],
            // 为什么有个 - 1? 把最后一个字节留给\0，保证最后以\0结束，解析http协议会方便一些
            (int)(req->buf_max - req->buf_len - 1),
            MSG_NOSIGNAL
        );
        if ( 0 == r ) {
            // 客户端关连接
            conn->close_type        = GR_NEED_CLOSE;
            conn->is_network_error  = true;
            break;
        } else if ( r < 0 ) {
            if ( EINTR == errno ) {
                continue;
            } else if ( ECONNRESET == errno ) {
                // Connection reset by peer
                conn->close_type        = GR_NEED_CLOSE;
                conn->is_network_error  = true;
            } else if ( EAGAIN == errno ) {
                // 没有数据可读了
            }
            break;
        }

        recv_bytes += r;
        req->buf_len += r;
    }

    if ( recv_bytes > 0 ) {
        req->buf[ req->buf_len ] = '\0';
    }

    * ret_req = conn->req;

    return recv_bytes;
}

int gr_poll_send(
    gr_poll_t *             poll,
    gr_thread_t *           thread,
    gr_tcp_conn_item_t *    conn
)
{
    gr_tcp_rsp_t *      rsp;
    int                 r;
    int                 sent_bytes = 0;

RETRY:

    while ( true ) {

        rsp = conn->rsp_list_head;
        if ( NULL == rsp ) {
            break;
        }

        while ( rsp->buf_sent < rsp->buf_len ) {
            
            r = send(
                conn->fd,
                & rsp->buf[ rsp->buf_sent ],
                rsp->buf_len - rsp->buf_sent,
                MSG_NOSIGNAL
            );

            if ( r >= 0 ) {
                rsp->buf_sent  += r;
                sent_bytes      += r;
                continue;
            }

            if ( EINTR == errno )
                continue;

            if ( EAGAIN != errno ) {
                // 发失败了
                gr_error( "send failed: %d", errno );
                return -1;
            }

            // 缓冲区已满
            return sent_bytes;
        }

        if ( rsp->buf_sent == rsp->buf_len ) {

            // 将发完的回复包弹出，同时把包删了
            r = gr_tcp_conn_pop_top_rsp( conn, rsp, true );
            if ( 0 != r ) {
                gr_error( "gr_tcp_conn_pop_top_rsp return error %d", r );
                return -2;
            }
        }
    }

    // 发完之后，将当前连接从发送epoll中删除
    r = del_tcp_send_fd( poll, conn );
    if ( 0 != r ) {
        gr_fatal( "del_tcp_send_fd return error %d", r );
        return -3;
    }

    // 因为没有锁，所以在将当前连接从发送epoll中删除后还要再检查一下有没有要发送的包
    if ( conn->rsp_list_head ) {
        // 如果有，要去重新把没发完的包发完，因为现在已经没有发送动力了。
        goto RETRY;
    }

    return sent_bytes;
}


int gr_poll_recv_done(
    gr_poll_t *             poll,
    gr_thread_t *           thread,
    gr_tcp_conn_item_t *    conn,
    bool                    is_ok
)
{
    if ( is_ok ) {
        return 0;
    }

    del_tcp_recv_fd( poll, conn );
    return 0;
}

int gr_poll_send_failed(
    gr_poll_t *             poll,
    gr_thread_t *           thread,
    gr_tcp_conn_item_t *    conn
)
{
    del_tcp_send_fd( poll, conn );
}

#endif // #if defined( __APPLE__ )
