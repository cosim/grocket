/**
 * @file libgrocket/gr_poll_linux.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   �߲����¼�����linux��
 * Revision History ���¼���
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-05    Created.
 **/
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

    if (   sizeof( gr_poll_data_t ) != sizeof( epoll_data_t )
        || sizeof( gr_poll_event_t ) != sizeof( struct epoll_event ) )
    {
        gr_fatal( "invalid struct implementation" );
        return NULL;
    }

    p = (gr_poll_t *)gr_calloc( 1, sizeof( gr_poll_t ) );
    if ( NULL == p ) {
        gr_fatal( "alloc %d failed", (int)sizeof( gr_poll_t ) );
        return NULL;
    }

    do {

        p->epfd         = -1;
        p->name         = name;

        p->epfd = epoll_create( concurrent );
        if ( -1 == p->epfd ) {
            gr_fatal( "epoll_create( %d ) failed: %d,%s",
                concurrent, (int)errno, strerror(errno) );
            r = -2;
            break;
        }

        gr_debug( "%s epoll_create( %d ) OK. epfd = %d",
            p->name, concurrent, p->epfd );

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
        gr_debug( "%s close( %d )", poll->name, poll->epfd );

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

    // �� accept ���� epoll
    ev.data.ptr = data;
    // ʹ�ñ�Ե����
    ev.events   = EPOLLIN | EPOLLET;
    r = epoll_ctl( poll->epfd, EPOLL_CTL_ADD, fd, & ev );
    if ( 0 != r ) {
        gr_fatal( "epoll_ctl return %d: %d,%s", r, errno, strerror( errno ) );
        return r;
    }

    gr_debug( "%s epoll_ctl( %d, EPOLL_CTL_ADD, %d, %p ) listen ok",
        poll->name, poll->epfd, fd, data );
    return 0;
}

int gr_poll_add_tcp_recv_fd(
    gr_poll_t *             poll,
    gr_tcp_conn_item_t *    conn,
    gr_threads_t *          threads
)
{
    struct epoll_event ev;
    int r;

    // �� fd ���� epoll
    ev.data.ptr = conn;
    // ʹ�ñ�Ե����
    ev.events   = EPOLLIN | EPOLLET;
    r = epoll_ctl( poll->epfd, EPOLL_CTL_ADD, conn->fd, & ev );
    if ( 0 != r ) {
        gr_fatal( "epoll_ctl return %d: %d,%s", r, errno, strerror( errno ) );
        return r;
    }

    gr_debug( "%s epoll_ctl( %d, EPOLL_CTL_ADD, %d, %p ) recv ok",
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

    // �� fd ���� epoll
    ev.data.ptr = conn;
    // ʹ�ñ�Ե����
    ev.events = EPOLLOUT | EPOLLET;

    //TODO: ����߼��ǳ��������ȿ��ǣ��������أ�
    r = epoll_ctl( poll->epfd, EPOLL_CTL_MOD, conn->fd, & ev );
    if ( 0 != r ) {
        if ( ENOENT == errno ) {
            gr_info( "epoll_ctl MOD: ENOENT. retry ADD" );

            r = epoll_ctl( poll->epfd, EPOLL_CTL_ADD, conn->fd, & ev );
            if ( 0 != r ) {
                gr_fatal( "epoll_ctl ADD return %d: %d,%s", r, errno, strerror( errno ) );
            }
        } else {
            gr_fatal( "epoll_ctl MOD return %d: %d,%s", r, errno, strerror( errno ) );
        }
    }

    return r;
}

static inline
int del_tcp_send_fd(
    gr_poll_t *             poll,
    gr_tcp_conn_item_t *    conn
)
{
    int                 r;
    struct epoll_event  ev;

    ev.data.ptr = conn;
    ev.events = 0;
    r = epoll_ctl( poll->epfd, EPOLL_CTL_MOD, conn->fd, & ev );
    if ( 0 != r ) {
        if ( ENOENT != errno ) {
            gr_fatal( "epoll_ctl + MOD failed: %d,%s", errno, strerror( errno ) );
            return -1;
        }
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
    int r;
    
    r = epoll_wait( poll->epfd, (struct epoll_event *)events, event_count, timeout );
    if ( r < 0 ) {
        if ( EINTR == errno ) {
            r = 0;
        } else {
            gr_error( "epoll_wait return error %d,%s", errno, strerror( errno ) );
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

        // ׼��req������
        req = gr_tcp_conn_prepare_recv( conn );
        if ( NULL == req ) {
            gr_fatal( "gr_tcp_conn_prepare_recv return NULL" );
            return -1;
        }

        r = recv(
            conn->fd,
            (char*)& req->buf[ req->buf_len ],
            // Ϊʲô�и� - 1? �����һ���ֽ�����\0����֤�����\0����������httpЭ��᷽��һЩ
            (int)(req->buf_max - req->buf_len - 1),
            MSG_NOSIGNAL
        );
        if ( 0 == r ) {
            // �ͻ��˹�����
            conn->close_type = GR_NEED_CLOSE;
            break;
        } else if ( r < 0 ) {
            if ( EINTR == errno ) {
                continue;
            } else if ( ECONNRESET == errno ) {
                // Connection reset by peer
                conn->close_type = GR_NEED_CLOSE;
            } else if ( EAGAIN == errno ) {
                // û�����ݿɶ���
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
                // ��ʧ����
                gr_error( "send failed: %d", errno );
                return -1;
            }

            // ��������������Ϊ��������Ҫ�������Բ��ܽ���ǰ���Ӵӷ���epoll��ɾ��
            return sent_bytes;
        }

        if ( rsp->buf_sent == rsp->buf_len ) {

            // ������Ļظ�������
            r = gr_tcp_conn_pop_top_rsp( conn, rsp );
            if ( 0 != r ) {
                gr_fatal( "gr_tcp_conn_pop_top_rsp return error %d", r );
                return -2;
            }
        }
    }

    // ����֮�󣬽���ǰ���Ӵӷ���epoll��ɾ��
    r = del_tcp_send_fd( poll, conn );
    if ( 0 != r ) {
        gr_fatal( "del_tcp_send_fd return error %d", r );
        return -3;
    }

    // ��Ϊû�����������ڽ���ǰ���Ӵӷ���epoll��ɾ����Ҫ�ټ��һ����û��Ҫ���͵İ�
    if ( conn->rsp_list_head ) {
        // ����У�Ҫȥ���°�û����İ����꣬��Ϊ�����Ѿ�û�з��Ͷ����ˡ�
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

    //TODO: �����OK����Ҫ������
    if ( conn->close_type >= GR_NEED_CLOSE ) {
        struct epoll_event  ev;
        int                 r;
        int                 fd;

        fd = conn->fd;
        conn->close_type >= GR_CLOSING;

        if ( -1 != fd ) {
            ev.data.ptr = NULL;
            ev.events = 0;

            //TODO: ����߼��ǳ��������ȿ��ǣ��������أ�
            r = epoll_ctl( poll->epfd, EPOLL_CTL_DEL, fd, & ev );
            if ( 0 != r ) {
                gr_warning( "%s epoll_ctl EPOLL_CTL_DEL return error %d: %d,%s", poll->name, r, errno, strerror( errno ) );
            } else {
                gr_info( "%s epoll_ctl EPOLL_CTL_DEL ok", poll->name );
            }
        }
    }

    return 0;
}

#endif // #if defined(__linux)
