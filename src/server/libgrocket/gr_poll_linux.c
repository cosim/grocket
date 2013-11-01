/**
 * @file libgrocket/gr_poll_linux.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   high performance network event. Linux
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

#include "gr_poll.h"
#include "gr_log.h"
#include "gr_global.h"
#include "gr_errno.h"
#include "gr_tools.h"
#include "gr_mem.h"

#if defined(__linux)

struct gr_poll_t
{
    int             epfd;

    const char *    name;
    
    bool            need_in;
    bool            already_in;
    
    bool            need_out;
    bool            already_out;
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

    if (   sizeof( gr_poll_data_t )  != sizeof( epoll_data_t )
        || sizeof( gr_poll_event_t ) != sizeof( struct epoll_event ) )
    {
        gr_fatal( "[init]invalid struct implementation" );
        return NULL;
    }

    p = (gr_poll_t *)gr_calloc( 1, sizeof( gr_poll_t ) );
    if ( NULL == p ) {
        gr_fatal( "[init]alloc %d failed", (int)sizeof( gr_poll_t ) );
        return NULL;
    }

    do {

        p->epfd         = -1;
        p->name         = name;
        
        if ( EPOLLIN & poll_type ) {
            p->need_in  = true;
        }
        if ( EPOLLOUT & poll_type ) {
            p->need_out = true;
        }
        if ( ! p->need_in && ! p->need_out ) {
            gr_fatal( "[init][poll_type=%d]invalid poll_type", (int)poll_type );
            r = -2;
            break;
        }

        p->epfd = epoll_create( concurrent );
        if ( -1 == p->epfd ) {
            gr_fatal( "[init]epoll_create( %d ) failed: %d,%s",
                concurrent, (int)errno, strerror(errno) );
            r = -3;
            break;
        }

        gr_debug( "[init]%s epoll_create( %d ) OK. epfd = %d, poll = %p",
            p->name, concurrent, p->epfd, p );

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

    if ( -1 != poll->epfd ) {
        gr_debug( "[term]%s close( %d )", poll->name, poll->epfd );

        close( poll->epfd );
        poll->epfd = -1;
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
    struct epoll_event ev;
    int r;

    // 将 accept 加入 epoll
    ev.data.ptr             = data;
    // 使用边缘触发
    ev.events               = EPOLLIN | EPOLLET;
    poll->already_in        = true;
    if ( poll->need_out ) {
        ev.events           |= EPOLLOUT;
        poll->already_out   = true;
    }
        
    r = epoll_ctl( poll->epfd, EPOLL_CTL_ADD, fd, & ev );
    if ( 0 != r ) {
        gr_fatal( "[%s]epoll_ctl return %d: %d,%s", poll->name, r, errno, strerror( errno ) );
        return r;
    }

    gr_debug( "[%s]epoll_ctl( %d, EPOLL_CTL_ADD, %d, %p ) listen ok",
        poll->name, poll->epfd, fd, data );
    return 0;
}

int gr_poll_add_tcp_recv_fd(
    gr_poll_t *             poll,
    gr_tcp_conn_item_t *    conn,
    gr_threads_t *          threads
)
{
    struct epoll_event  ev;
    int                 r;

    // 将 fd 加入 epoll
    ev.data.ptr             = conn;
    // 使用边缘触发
    ev.events               = EPOLLIN | EPOLLET;
    poll->already_in        = true;
    if ( poll->need_out ) {
        ev.events           |= EPOLLOUT;
        poll->already_out   = true;
    }

    r = epoll_ctl( poll->epfd, EPOLL_CTL_ADD, conn->fd, & ev );
    if ( 0 != r ) {
        gr_fatal( "[%s]epoll_ctl return %d: %d,%s", poll->name, r, errno, strerror( errno ) );
        return r;
    }

    gr_debug( "[%s]epoll_ctl( %d, EPOLL_CTL_ADD, %d, %p ) recv ok",
        poll->name, poll->epfd, conn->fd, conn );
    return 0;
}

int gr_poll_add_tcp_send_fd(
    gr_poll_t *             poll,
    gr_tcp_conn_item_t *    conn,
    gr_threads_t *          threads
)
{
    struct epoll_event ev;
    int r;

    // 将 fd 加入 epoll
    ev.data.ptr             = conn;
    // 使用边缘触发
    ev.events               = EPOLLOUT | EPOLLET;
    poll->already_out       = true;
    if ( poll->need_in ) {
        ev.events           |= EPOLLIN;
        poll->already_in    = true;
    }

    //TODO: 这个逻辑是长连接优先考虑，短连接呢？
    r = epoll_ctl( poll->epfd, EPOLL_CTL_MOD, conn->fd, & ev );
    if ( 0 != r ) {
        if ( ENOENT == errno ) {
            gr_info( "[%s]epoll_ctl MOD: ENOENT. retry ADD", poll->name );

            r = epoll_ctl( poll->epfd, EPOLL_CTL_ADD, conn->fd, & ev );
            if ( 0 != r ) {
                gr_fatal( "[%s]epoll_ctl ADD return %d: %d,%s",
                    poll->name, r, errno, strerror( errno ) );
            }
        } else {
            gr_fatal( "[%s]epoll_ctl MOD return %d: %d,%s",
                poll->name, r, errno, strerror( errno ) );
        }
    }

    return r;
}

static_inline
int del_tcp_send_fd(
    gr_poll_t *             poll,
    gr_tcp_conn_item_t *    conn,
    int *                   e
)
{
    int                 r;
    struct epoll_event  ev;

    ev.data.ptr         = conn;
    ev.events           = poll->already_in ? ( EPOLLIN | EPOLLET ) : 0;
    poll->already_out   = false;
    r = epoll_ctl( poll->epfd, EPOLL_CTL_MOD, conn->fd, & ev );
    if ( NULL != e ) {
        * e = errno;
    }
    if ( 0 != r ) {
        if ( ENOENT != errno ) {
            gr_fatal( "[%s]epoll_ctl + MOD failed: %d,%s", poll->name, errno, strerror( errno ) );
        }
    }

    return 0;
}

static_inline
int del_tcp_recv_fd(
    gr_poll_t *             poll,
    gr_tcp_conn_item_t *    conn,
    int *                   e
)
{
    struct epoll_event  ev;
    int                 r;

    ev.data.ptr         = NULL;
    ev.events           = poll->already_out ? ( EPOLLOUT | EPOLLET ) : 0;
    poll->already_in    = false;

    r = epoll_ctl( poll->epfd, EPOLL_CTL_DEL, conn->fd, & ev );
    if ( NULL != e ) {
        * e = errno;
    }
    if ( 0 != r ) {
        gr_warning( "[%s]epoll_ctl EPOLL_CTL_DEL return error %d: %d,%s",
            poll->name, r, errno, strerror( errno ) );
    }

    return r;
}

int gr_poll_wait(
    gr_poll_t *         poll,
    gr_poll_event_t *   events,
    int                 event_count,
    int                 timeout,
    gr_thread_t *       thread
)
{
    int r;
    
    r = epoll_wait( poll->epfd, (struct epoll_event *)events, event_count, timeout );
    if ( r < 0 ) {
        if ( EINTR == errno ) {
            r = 0;
        } else {
            gr_error( "[%s]epoll_wait return error %d,%s",
                poll->name, errno, strerror( errno ) );
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
            gr_fatal( "[%s]gr_tcp_conn_prepare_recv return NULL", poll->name );
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
#ifdef GR_DEBUG_CONN
            gr_warning( "[%s][req.push=%d][req.proc=%d][rsp.sent=%llu]recv return 0",
                        poll->name, conn->req_push_count, conn->req_proc_count, conn->rsp_send_count, r );
#else
            gr_warning( "[%s][req.push=%d][req.proc=%d]recv return 0",
                        poll->name, conn->req_push_count, conn->req_proc_count );
#endif
            conn->close_type        = GR_NEED_CLOSE;
            conn->is_network_error  = true;
            break;
        } else if ( r < 0 ) {
            if ( EINTR == errno ) {
                continue;
            } else if ( ECONNRESET == errno ) {
                // Connection reset by peer
#ifdef GR_DEBUG_CONN
                gr_warning( "[%s][e=ECONNRESET][req.push=%d][req.proc=%d][rsp.sent=%llu]recv return %d",
                            poll->name, conn->req_push_count, conn->req_proc_count, conn->rsp_send_count, r );
#else
                gr_warning( "[%s][e=ECONNRESET][req.push=%d][req.proc=%d]recv return %d",
                            poll->name, conn->req_push_count, conn->req_proc_count, r );
#endif
                conn->close_type        = GR_NEED_CLOSE;
                conn->is_network_error  = true;
            } else if ( EAGAIN == errno ) {
                // 没有数据可读了
            }
            break;
        }

        recv_bytes += r;
        req->buf_len += r;
#ifdef GR_DEBUG_CONN
        conn->recv_bytes += r;
#endif
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
    int                 e;
    int                 need_send;

RETRY:

    while ( true ) {

        rsp = gr_tcp_conn_top_rsp( conn );
        if ( NULL == rsp ) {
            break;
        }

        while ( rsp->buf_sent < rsp->buf_len ) {
            
            need_send = rsp->buf_len - rsp->buf_sent;
            r = send(
                conn->fd,
                & rsp->buf[ rsp->buf_sent ],
                need_send,
                MSG_NOSIGNAL
            );

            if ( r >= 0 ) {
                rsp->buf_sent   += r;
                sent_bytes      += r;
#ifdef GR_DEBUG_CONN
                conn->send_bytes += r;
#endif
                gr_debug( "[%s][rsp=%p][conn=%p][poll=%p][epfd=%d] send %d bytes",
                          poll->name, rsp, conn, poll, poll->epfd, r );

                if ( r < need_send ) {
                    // 缓冲区发满了, 没出错。不要再调一次 send 了，如果进系统调用后发现不能发就赔了  
                    if ( 0 == r ) {
                        gr_warning( "sent return 0, err = %d:%s!!!!!!!!!!!!!", errno, strerror( errno ) );
                    }
                    return sent_bytes;
                }

                continue;
            }

            if ( EINTR == errno ) {
                continue;
            }

            if ( EAGAIN != errno ) {
                // 发失败了
                gr_error( "[%s]send failed: %d,%s", poll->name, errno, strerror( errno ) );

                conn->is_network_error = 1;
                // 将当前连接从 poll中删除
                r = del_tcp_send_fd( poll, conn, &e );
                if ( 0 != r && ENOENT != e ) {
                    gr_warning( "[%s]del_tcp_send_fd return error %d", poll->name, r );
                }
                if ( conn->close_type > GR_NEED_CLOSE ) {
                    conn->close_type = GR_NEED_CLOSE;
                }

                return -1;
            }

            // 缓冲区已满，因为还有数据要发，所以不能将当前连从从发送epoll中删除
            return sent_bytes;
        }

        assert( rsp->buf_sent <= rsp->buf_len );
        if ( rsp->buf_sent == rsp->buf_len ) {

            // 将发完的回复包弹出，同时把包删了
            r = gr_tcp_conn_pop_top_rsp( conn, rsp );
            if ( 0 != r ) {
                gr_fatal( "[%s]gr_tcp_conn_pop_top_rsp return error %d", poll->name, r );

                conn->is_network_error = 1;
                // 将当前连接从 poll中删除
                r = del_tcp_send_fd( poll, conn, &e );
                if ( 0 != r && ENOENT != e ) {
                    gr_warning( "[%s]del_tcp_send_fd return error %d", poll->name, r );
                }
                if ( conn->close_type > GR_NEED_CLOSE ) {
                    conn->close_type = GR_NEED_CLOSE;
                }

                return -2;
            }
        }
    }

    // 发完之后，将当前连接从发送epoll中删除
    r = del_tcp_send_fd( poll, conn, &e );
    if ( 0 != r && ENOENT != e ) {
        gr_fatal( "[%s]del_tcp_send_fd return error %d", poll->name, r );
        conn->is_network_error = 1;
        if ( conn->close_type > GR_NEED_CLOSE ) {
            conn->close_type = GR_NEED_CLOSE;
        }
        return -3;
    }

    // 因为没有锁，所以在将当前连接从发送epoll中删除后还要再检查一下有没有要发送的包
    if ( NULL != gr_tcp_conn_top_rsp( conn ) ) {
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

    del_tcp_recv_fd( poll, conn, NULL );
    return 0;
}

int gr_poll_del_tcp_recv_fd(
    gr_poll_t *             poll,
    gr_tcp_conn_item_t *    conn,
    gr_threads_t *          threads
)
{
    int e;
    int r;

    r = del_tcp_recv_fd( poll, conn, & e );
    if ( 0 == e ) {
        return GR_OK;
    }
    if ( ENOENT == e ) {
        return GR_ERR_NOT_FOUND;
    }

    gr_error( "[%s]del_tcp_recv_fd failed: %d,%s", poll->name, e, strerror( e ) );
    return GR_ERR_SYSTEM_CALL_FAILED;
}

int gr_poll_del_tcp_fd(
    gr_poll_t *             poll,
    gr_tcp_conn_item_t *    conn,
    gr_threads_t *          threads
)
{
    int                 r;
    struct epoll_event  ev;

    ev.data.ptr         = conn;
    ev.events           = 0;
    poll->already_out   = false;
    r = epoll_ctl( poll->epfd, EPOLL_CTL_DEL, conn->fd, & ev );
    if ( 0 != r ) {
        int e = errno;
        if ( ENOENT == e ) {
            gr_fatal( "[%s]epoll_ctl + DEL not found", poll->name );
            return GR_ERR_NOT_FOUND;
        } else {
            gr_fatal( "[%s]epoll_ctl + DEL failed: %d,%s", poll->name, e, strerror( e ) );
            return GR_ERR_SYSTEM_CALL_FAILED;
        }
    }

    return GR_OK;
}

int gr_poll_del_tcp_send_fd(
    gr_poll_t *             poll,
    gr_tcp_conn_item_t *    conn,
    gr_threads_t *          threads
)
{
    int e;
    int r;

    r = del_tcp_send_fd( poll, conn, & e );
    if ( 0 == e ) {
        return GR_OK;
    }
    if ( ENOENT == e ) {
        return GR_ERR_NOT_FOUND;
    }
    gr_error( "[%s]del_tcp_recv_fd failed: %d,%s", poll->name, e, strerror( e ) );
    return GR_ERR_SYSTEM_CALL_FAILED;
}

int gr_pool_replace_from(
    gr_poll_t *             poll,
    gr_poll_t *             from_poll
)
{
    int  t;

    if ( NULL == from_poll || NULL == poll ) {
        gr_fatal( "[%s]from_poll %p or poll %p is NULL", poll->name, from_poll, poll );
        return -1;
    }

    if ( -1 == from_poll->epfd ) {
        gr_fatal( "[%s]from_poll->epfd is -1", poll->name );
        return -2;
    }

    t = poll->epfd;
    poll->epfd = from_poll->epfd;
    close( t );

    return 0;
}

#endif // #if defined(__linux)
