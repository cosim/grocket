/**
 * @file include/gr_poll.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   �߲����¼�����
 * Revision History ���¼���
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-05    Created.
 **/
#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_POLL_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_POLL_H_

#include "gr_stdinc.h"
#include "grocket.h"
#include "gr_thread.h"
#include "gr_conn.h"

#if defined( __linux )
    #include <sys/epoll.h>
#else
    // �ǵģ�linux���׵����ģ�����ϵͳ���Ǻ�����ġ�
    #define EPOLLIN     0x001
    #define EPOLLOUT    0x004
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct gr_poll_t;
typedef struct gr_poll_t gr_poll_t;

struct gr_poll_event_t;
typedef struct gr_poll_event_t gr_poll_event_t;

// ��enumֵ�����epollһģһ��
typedef enum
{
    GR_POLLIN   = EPOLLIN,
    GR_POLLOUT  = EPOLLOUT,

} GR_POLL_EVENT;

// ��Linux�£����ڹ������epollһģһ��
typedef union
{
    void *                  ptr;
    int                     fd;
    uint32_t                u32;
    uint64_t                u64;
} gr_poll_data_t;

// ��Linux�£����ڹ������epollһģһ��
struct gr_poll_event_t
{
    uint32_t                events;
    gr_poll_data_t          data;
} __attribute__ ((__packed__));

gr_poll_t * gr_poll_create(
    int                     concurrent,
    int                     thread_count,
    GR_POLL_EVENT           poll_type,
    const char *            name
);

void gr_poll_destroy(
    gr_poll_t *             poll
); 

int gr_poll_add_listen_fd(
    gr_poll_t *             poll,
    bool                    is_tcp,
    int                     fd,
    void *                  data,
    gr_threads_t *          threads
);

int gr_poll_add_tcp_recv_fd(
    gr_poll_t *             poll,
    gr_tcp_conn_item_t *    conn,
    gr_threads_t *          threads
);

int gr_poll_add_tcp_send_fd(
    gr_poll_t *             poll,
    gr_tcp_conn_item_t *    conn,
    gr_threads_t *          threads
);

int gr_poll_raw_buff_for_accept_len();
int gr_poll_raw_buff_for_udp_in_len();
int gr_poll_raw_buff_for_tcp_in_len();
int gr_poll_raw_buff_for_tcp_out_len();

int gr_poll_wait(
    gr_poll_t *             poll,
    gr_poll_event_t *       events,
    int                     event_count,
    int                     timeout,
    gr_thread_t *           thread
);

int gr_poll_accept(
    gr_poll_t *             poll,
    gr_thread_t *           thread,
    int                     listen_fd,
    struct sockaddr *       addr,
    int *                   addrlen
);

int gr_poll_recv(
    gr_poll_t *             poll,
    gr_thread_t *           thread,
    gr_tcp_conn_item_t *    conn,
    gr_tcp_req_t **         req
);

int gr_poll_recv_done(
    gr_poll_t *             poll,
    gr_thread_t *           thread,
    gr_tcp_conn_item_t *    conn,
    bool                    is_ok
);

int gr_poll_send(
    gr_poll_t *             poll,
    gr_thread_t *           thread,
    gr_tcp_conn_item_t *    conn
);

#if defined( WIN32 ) || defined( WIN64 )

// windows������ͬһ��socketͬʱ�ӵ�����iocp��
// �ṩ�ú����Ա����� tcp_out ʹ�� tcp_in �� IOCP
int gr_pool_replace_from(
    gr_poll_t *             poll,
    gr_poll_t *             from_poll
);

#endif // #if defined( WIN32 ) || defined( WIN64 )

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_POLL_H_
