/**
 * @file libgrocket/gr_udp_out.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/03
 * @version $Revision$ 
 * @brief   UDP 输出线程
 * Revision History 大事件记
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-03    Created.
 **/
#include "gr_udp_out.h"
#include "gr_thread.h"
#include "gr_log.h"
#include "gr_global.h"
#include "gr_errno.h"
#include "gr_tools.h"
#include "gr_mem.h"
#include "gr_config.h"
#include "gr_conn.h"
#include "gr_poll.h"

typedef struct
{
    gr_threads_t    threads;

    gr_poll_t *     poll;

    int             concurrent;

} gr_udp_out_t;

void udp_out_worker( gr_thread_t * thread )
{
    /*
#define     UDP_OUT_WAIT_TIMEOUT    100
    int                 count;
    int                 i;
    gr_udp_out_t *      self;
    gr_poll_event_t *   events;
    gr_poll_event_t *   e;
    gr_server_t *       server = & g_ghost_rocket_global.server_interface;
    gr_udp_rsp_t *      rsp;

    self    = (gr_udp_out_t *)thread->param;

    events  = (gr_poll_event_t *)gr_malloc( sizeof( gr_poll_event_t ) * self->concurrent );
    if ( NULL == events ) {
        gr_fatal( "bad_alloc %d", (int)sizeof( gr_poll_event_t ) * self->concurrent );
        return;
    }

    while ( ! thread->is_need_exit ) {

        count = gr_poll_wait( self->poll, events, self->concurrent, UDP_OUT_WAIT_TIMEOUT );
        if ( count < 0 ) {
            gr_fatal( "gr_poll_wait return %d", count );
            continue;
        }

        for ( i = 0; i < count; ++ i ) {
            e = & events[ i ];
            
            if ( NULL != e->data.ptr ) {
                rsp = (gr_udp_rsp_t *)e->data.ptr;
                // TCP新连接
                on_udp_out( self, thread, rsp );
            } else {
                gr_fatal( "invalid e->data.ptr %p", e->data.ptr );
            }
        }
    };
    gr_free( events );
    */
}

int gr_udp_out_init()
{
    gr_udp_out_t *  p;
    int thread_count = gr_config_udp_out_thread_count();
    int r;

    if ( thread_count < 1 ) {
        gr_fatal( "[init]udp_out thread_count invalid" );
        return GR_ERR_INVALID_PARAMS;
    }

    if ( NULL != g_ghost_rocket_global.udp_out ) {
        gr_fatal( "[init]gr_udp_out_init already called" );
        return GR_ERR_WRONG_CALL_ORDER;
    }

    p = (gr_udp_out_t *)gr_calloc( 1, sizeof( gr_udp_out_t ) );
    if ( NULL == p ) {
        gr_fatal( "[init]malloc %d bytes failed, errno=%d,%s",
            (int)sizeof(gr_udp_out_t), errno, strerror( errno ) );
        return GR_ERR_BAD_ALLOC;
    }

    p->concurrent = gr_config_udp_out_concurrent();

    r = GR_ERR_UNKNOWN;

    do {

        p->poll = gr_poll_create( p->concurrent, thread_count, GR_POLLOUT, "udp_out" );
        if ( NULL == p->poll ) {
            r = GR_ERR_INIT_POLL_FALED;
            break;
        }

        r = gr_threads_start(
            & p->threads,
            thread_count,
            NULL,
            udp_out_worker,
            p,
            0,
            true,
            "udp.out" );
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

    g_ghost_rocket_global.udp_out = p;
    return GR_OK;
}

void gr_udp_out_term()
{
    gr_udp_out_t * p = g_ghost_rocket_global.udp_out;

    if ( NULL != p ) {

        gr_threads_close( & p->threads );

        // 线程停了就可以把全局变量清掉了
        g_ghost_rocket_global.udp_out = NULL;

        if ( NULL != p->poll ) {
            gr_poll_destroy( p->poll );
            p->poll = NULL;
        }

        gr_free( p );
        g_ghost_rocket_global.udp_out = NULL;
    }
}

int gr_udp_out_add(
    gr_udp_rsp_t * rsp
)
{
    gr_info( "not implemenet" );
    return 0;
}
