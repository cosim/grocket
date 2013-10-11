/**
 * @file libgrocket/gr_udp_in.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/03
 * @version $Revision$ 
 * @brief   UDP数据接收线程
 * Revision History 大事件记
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-03    Created.
 **/
#include "gr_udp_in.h"
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

// gr_tcp_accept_t 已经是外部接口回调函数声明了, 我只能换一个名字 gr_accept_t
typedef struct
{
    gr_threads_t    threads;

    gr_poll_t *     poll;

    int             concurrent;

} gr_udp_in_t;

static inline
void on_udp_in(
    gr_udp_in_t *       self,
    gr_thread_t *       thread,
    gr_port_item_t *    port_item
)
{
}

void udp_in_worker( gr_thread_t * thread )
{
#define     UDP_IN_WAIT_TIMEOUT    100
    int                 count;
    int                 i;
    gr_udp_in_t *       self;
    gr_poll_event_t *   events;
    gr_poll_event_t *   e;
    gr_server_t *       server = & g_ghost_rocket_global.server_interface;
    gr_port_item_t *    port_item;

    self    = (gr_udp_in_t *)thread->param;

    events  = (gr_poll_event_t *)gr_malloc( sizeof( gr_poll_event_t ) * self->concurrent );
    if ( NULL == events ) {
        gr_fatal( "bad_alloc %d", (int)sizeof( gr_poll_event_t ) * self->concurrent );
        return;
    }

    while ( ! thread->is_need_exit ) {

        count = gr_poll_wait( self->poll, events, self->concurrent, UDP_IN_WAIT_TIMEOUT, thread );
        if ( count < 0 ) {
            gr_fatal( "gr_poll_wait return %d", count );
            continue;
        }

        for ( i = 0; i < count; ++ i ) {
            e = & events[ i ];
            
            if (   e->data.ptr >= (void*)& server->ports[ 0 ]
                && e->data.ptr <= (void*)& server->ports[ server->ports_count - 1 ] )
            {
                port_item = (gr_port_item_t *)e->data.ptr;

                // TCP新连接
                on_udp_in( self, thread, port_item );
            } else {
                gr_fatal( "invalid e->data.ptr %p", e->data.ptr );
            }
        }
    };

    gr_free( events );
}

int gr_udp_in_init()
{
    gr_udp_in_t *  p;
    int thread_count = gr_config_udp_in_thread_count();
    int r;

    if ( thread_count < 1 ) {
        gr_fatal( "[init]udp_in thread_count invalid" );
        return GR_ERR_INVALID_PARAMS;
    }

    if ( NULL != g_ghost_rocket_global.udp_in ) {
        gr_fatal( "[init]gr_udp_in_init already called" );
        return GR_ERR_WRONG_CALL_ORDER;
    }

    p = (gr_udp_in_t *)gr_calloc( 1, sizeof( gr_udp_in_t ) );
    if ( NULL == p ) {
        gr_fatal( "[init]malloc %d bytes failed, errno=%d,%s",
            (int)sizeof(gr_udp_in_t), errno, strerror( errno ) );
        return GR_ERR_BAD_ALLOC;
    }

    p->concurrent = gr_config_udp_in_concurrent();

    r = GR_OK;

    do {
        p->poll = gr_poll_create( p->concurrent, thread_count, GR_POLLIN, "udp_in" );
        if ( NULL == p->poll ) {
            r = GR_ERR_INIT_POLL_FALED;
            break;
        }

        r = gr_threads_start(
            & p->threads,
            thread_count,
            NULL,
            udp_in_worker,
            p,
            gr_poll_raw_buff_for_udp_in_len(),
            true,
            "udp.in" );
        if ( GR_OK != r ) {
            break;
        }

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

    g_ghost_rocket_global.udp_in = p;
    return GR_OK;
}

void gr_udp_in_term()
{
    gr_udp_in_t *  p = g_ghost_rocket_global.udp_in;
    if ( NULL != p ) {

        gr_threads_close( & p->threads );

        // 线程停了就可以把全局变量清掉了
        g_ghost_rocket_global.udp_in = NULL;

        if ( NULL != p->poll ) {
            gr_poll_destroy( p->poll );
            p->poll = NULL;
        }

        gr_free( p );
    }
}

int gr_udp_in_add_listen_ports()
{
    int             i;
    int             r               = 0;
    gr_server_t *   server_interface= & g_ghost_rocket_global.server_interface;
    gr_udp_in_t *   self            = g_ghost_rocket_global.udp_in;

    if ( NULL == self ) {
        gr_fatal( "[init]udp_in is NULL" );
        return -1;
    }

    for ( i = 0; i < server_interface->ports_count; ++ i ) {
        gr_port_item_t * item = & server_interface->ports[ i ];

        if ( ! item->is_tcp ) {
            r = gr_poll_add_listen_fd( self->poll, item->is_tcp, item->fd, item, & self->threads );
            if ( 0 != r ) {
                gr_fatal( "[init]gr_poll_add_listen_fd return %d", r );
                return -2;
            }

            gr_info( "start listen UDP port %d", item->port );
        }
    }

    return 0;
}
