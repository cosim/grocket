/**
 * @file libgrocket/gr_poll_bsd.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/07
 * @version $Revision$ 
 * @brief   �߲����¼�����bsd & Mac��
 * Revision History ���¼���
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-07    Created.
 **/
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

    // �� accept ���� kqueue
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

int gr_poll_wait(
    gr_poll_t *         poll,
    gr_poll_event_t *   events,
    int                 event_count,
    int                 timeout,
    gr_thread_t *       thread
)
{
    //TODO: һ��ֻ��һ���¼����Ժ����Ż�
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

            // ����������
            return sent_bytes;
        }

        if ( rsp->buf_sent == rsp->buf_len ) {

            // ������Ļظ�������
            r = gr_tcp_conn_pop_top_rsp( conn, rsp );
            if ( 0 != r ) {
                gr_error( "gr_tcp_conn_pop_top_rsp return error %d", r );
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
        struct kevent   ev;
        struct timespec ts;
        int             r;
        int             fd;

        fd = conn->fd;
        conn->close_type >= GR_CLOSING;

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

    return 0;
}

#endif // #if defined( __APPLE__ )
