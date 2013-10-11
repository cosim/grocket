/**
 * @file libgrocket/gr_tcp_in.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/03
 * @version $Revision$ 
 * @brief   TCP数据接收线程
 * Revision History 大事件记
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-03    Created.
 **/
#include "gr_tcp_in.h"
#include "tcp_io.h"

void tcp_in_worker( gr_thread_t * thread )
{
#define     TCP_IN_WAIT_TIMEOUT    100
    int                     count;
    int                     i;
    gr_tcp_in_t *           self;
    gr_poll_event_t *       events;
    gr_poll_event_t *       e;
    gr_server_t *           server;
    gr_tcp_conn_item_t *    conn;
    //gr_port_item_t *    port_item;

    server  = & g_ghost_rocket_global.server_interface;
    self    = (gr_tcp_in_t *)thread->param;

    events  = (gr_poll_event_t *)gr_malloc( sizeof( gr_poll_event_t ) * self->concurrent );
    if ( NULL == events ) {
        gr_fatal( "bad_alloc %d", (int)sizeof( gr_poll_event_t ) * self->concurrent );
        return;
    }

    while ( ! thread->is_need_exit ) {

        count = gr_poll_wait( self->poll, events, self->concurrent, TCP_IN_WAIT_TIMEOUT, thread );
        if ( count < 0 ) {
            gr_fatal( "gr_poll_wait return %d", count );
            continue;
        } else if ( 0 == count ) {
            continue;
        }

        for ( i = 0; i < count; ++ i ) {
            e = & events[ i ];

            // TCP数据流
            conn = (gr_tcp_conn_item_t *)e->data.ptr;
            on_tcp_recv( self, thread, conn );
        }
    };

    gr_free( events );
}

int gr_tcp_in_init()
{
    gr_tcp_in_t *  p;
    int thread_count = gr_config_tcp_in_thread_count();
    int r;

    if ( thread_count < 1 ) {
        gr_fatal( "[init]tcp_in thread_count invalid" );
        return GR_ERR_INVALID_PARAMS;
    }

    if ( NULL != g_ghost_rocket_global.tcp_in ) {
        gr_fatal( "[init]gr_tcp_in_init already called" );
        return GR_ERR_WRONG_CALL_ORDER;
    }

    p = (gr_tcp_in_t *)gr_calloc( 1, sizeof( gr_tcp_in_t ) );
    if ( NULL == p ) {
        gr_fatal( "[init]malloc %d bytes failed, errno=%d,%s",
            (int)sizeof(gr_tcp_in_t), errno, strerror( errno ) );
        return GR_ERR_BAD_ALLOC;
    }

    p->concurrent = gr_config_tcp_in_concurrent();

    r = GR_OK;

    do {

        const char * name = "tcp.input ";

        p->poll = gr_poll_create( p->concurrent, thread_count, GR_POLLIN, name );
        if ( NULL == p->poll ) {
            r = GR_ERR_INIT_POLL_FALED;
            break;
        }

        r = gr_threads_start(
            & p->threads,
            thread_count,
            NULL,
#if defined( WIN32 ) || defined( WIN64 )
            tcp_io_windows,
#else
            tcp_in_worker,
#endif
            p,
            gr_poll_raw_buff_for_tcp_in_len(),
            true,
            name );
        if ( GR_OK != r ) {
            break;
        }

        gr_debug( "tcp_in_init OK" );

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

    g_ghost_rocket_global.tcp_in = p;
    return GR_OK;
}

void gr_tcp_in_term()
{
    gr_tcp_in_t *  p = (gr_tcp_in_t *)g_ghost_rocket_global.tcp_in;
    if ( NULL != p ) {

        gr_threads_close( & p->threads );

        if ( NULL != p->poll ) {
            gr_poll_destroy( p->poll );
            p->poll = NULL;
        }

        gr_free( p );
        g_ghost_rocket_global.tcp_in = NULL;
    }
}

int gr_tcp_in_add_conn(
    gr_tcp_conn_item_t *    conn
)
{
    int             r;
    gr_tcp_in_t *   self;
    
    self = (gr_tcp_in_t *)g_ghost_rocket_global.tcp_in;
    if ( NULL == self ) {
        gr_fatal( "gr_tcp_in_init never call" );
        return -1;
    }

    // 将该socket加到poll里
    r = gr_poll_add_tcp_recv_fd(
        self->poll,
        conn,
        & self->threads
    );
    if ( 0 != r ) {
        gr_fatal( "gr_poll_add_tcp_recv_fd return %d", r );
        return -2;
    }

    return 0;
}

#if defined( WIN32 ) || defined( WIN64 )

void * gr_tcp_in_get_poll()
{
    gr_tcp_in_t *   self;
    
    self = (gr_tcp_in_t *)g_ghost_rocket_global.tcp_in;
    if ( NULL == self ) {
        gr_fatal( "gr_tcp_in_init never call" );
        return NULL;
    }

    return self->poll;
}

#endif // #if defined( WIN32 ) || defined( WIN64 )
