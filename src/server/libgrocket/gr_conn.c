/**
 * @file libgrocket/gr_conn.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   连接相关处理
 * Revision History 大事件记
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-05    Created.
 **/
#include "gr_conn.h"
#include "gr_log.h"
#include "gr_global.h"
#include "gr_tools.h"
#include "gr_mem.h"
#include "gr_socket.h"
#include "gr_config.h"

typedef struct
{
    int     tcp_recv_buf_init;
    int     tcp_recv_buf_max;

} gr_conn_engine_t;

int gr_conn_init()
{
    gr_conn_engine_t *  p;

    if ( sizeof( gr_queue_item_t ) != sizeof( gr_queue_item_compact_t ) ) {
        gr_fatal( "sizeof( gr_queue_item_t ) = %d, sizeof( gr_queue_item_compact_t ) = %d, not equal",
            (int)sizeof( gr_queue_item_t ), (int)sizeof( gr_queue_item_compact_t ) );
        return -1;
    }
    if ( sizeof( gr_udp_req_t ) != sizeof( gr_udp_rsp_t ) ) {
        gr_fatal( "sizeof( gr_tcp_req_t ) = %d, sizeof( gr_udp_req_t ) = %d, not equal",
            (int)sizeof( gr_tcp_req_t ), (int)sizeof( gr_udp_req_t ) );
        return -1;
    }

#if defined( GR_TCP_CONN_ITEM_LEN )
    if ( sizeof( gr_tcp_conn_item_t ) != GR_TCP_CONN_ITEM_LEN ) {
        gr_fatal( "sizeof( gr_conn_item_t ) = %d, not %d",
            (int)sizeof( gr_tcp_conn_item_t ), GR_TCP_CONN_ITEM_LEN );
        return -1;
    }
#endif
#if defined( GR_TCP_REQ_LEN )
    if ( sizeof( gr_tcp_req_t ) != GR_TCP_REQ_LEN ) {
        gr_fatal( "sizeof( gr_tcp_req_t ) = %d, not %d",
            (int)sizeof( gr_tcp_req_t ), GR_TCP_REQ_LEN );
        return -1;
    }
#endif
#if defined( GR_UDP_REQ_LEN )
    if ( sizeof( gr_udp_req_t ) != GR_UDP_REQ_LEN ) {
        gr_fatal( "sizeof( gr_udp_req_t ) = %d, not %d",
            (int)sizeof( gr_udp_req_t ), GR_UDP_REQ_LEN );
        return -1;
    }
#endif

    p = (gr_conn_engine_t *)g_ghost_rocket_global.conn;
    if ( NULL != p ) {
        gr_fatal( "gr_conn_init called" );
        return -2;
    }

    p = (gr_conn_engine_t *)gr_calloc( 1, sizeof( gr_conn_engine_t ) );
    if ( NULL == p ) {
        gr_fatal( "gr_calloc %d bytes failed: %d", (int)sizeof( gr_conn_engine_t ), get_errno() );
        return -3;
    }

    p->tcp_recv_buf_init    = gr_config_get_tcp_recv_buf_init();
    p->tcp_recv_buf_max     = gr_config_get_tcp_recv_buf_max();

    g_ghost_rocket_global.conn = p;
    return 0;
}

void gr_conn_term()
{
    gr_conn_engine_t *  p;
    p = (gr_conn_engine_t *)g_ghost_rocket_global.conn;
    if ( NULL == p ) {
        return ;
    }

    gr_free( p );

    g_ghost_rocket_global.conn = NULL;
}

gr_tcp_conn_item_t *
gr_tcp_conn_alloc(
    gr_port_item_t *    port_item,
    int                 fd
)
{
    gr_tcp_conn_item_t *    conn;

    assert( port_item && -1 != fd );

    //TODO: 以后需要在分配上优化

    conn = (gr_tcp_conn_item_t *)gr_calloc( 1, sizeof( gr_tcp_conn_item_t ) );
    if ( NULL == conn ) {
        gr_fatal( "gr_calloc %d failed: %d", (int)sizeof( gr_tcp_conn_item_t ), get_errno() );
        return NULL;
    }

    conn->port_item     = port_item;
    conn->fd            = fd;

    conn->req = gr_tcp_req_alloc( conn, 0 );
    if ( NULL == conn->req ) {
        gr_fatal( "gr_tcp_req_alloc failed" );
        gr_free( conn );
        return NULL;
    }

    conn->close_type    = GR_OPENING;

    return conn;
}

void gr_tcp_conn_free(
    gr_tcp_conn_item_t *    conn
)
{
    if ( NULL != conn ) {

        // 关socket
        if ( -1 != conn->fd ) {
            gr_socket_close( conn->fd );
            conn->fd = -1;
        }

        if ( NULL != conn->req ) {
            gr_tcp_req_free( conn->req );
            conn->req = NULL;
        }

        gr_free( conn );
    }
}

gr_tcp_req_t * gr_tcp_conn_prepare_recv(
    gr_tcp_conn_item_t *    conn
)
{
    gr_conn_engine_t *  self   = NULL;
    self = (gr_conn_engine_t *)g_ghost_rocket_global.conn;
    if ( NULL == self ) {
        gr_fatal( "gr_conn_init never call" );
        return NULL;
    }

    if ( conn->close_type < GR_OPENING ) {
        gr_fatal( "conn disconnecting" );
        return NULL;
    }

    if ( NULL == conn->req ) {
        // 没分配req
        conn->req = gr_tcp_req_alloc( conn, self->tcp_recv_buf_init );
        if ( NULL == conn->req ) {
            gr_fatal( "gr_tcp_req_alloc with buf %d failed", self->tcp_recv_buf_init );
            return NULL;
        }
    } else if ( 0 == conn->req->buf_max ) {
        // 分配req了，缓冲区长度为0
        conn->req->buf = (char *)gr_malloc( self->tcp_recv_buf_init );
        if ( NULL == conn->req->buf ) {
            gr_fatal( "malloc %d bytes failed: %d", self->tcp_recv_buf_init, get_errno() );
            return NULL;
        }

        conn->req->buf_max = self->tcp_recv_buf_init;    

    } else if ( conn->req->buf_len + 1 == conn->req->buf_max ) {
        // 没有可用的空间，需要扩, 整倍扩
        // 为什么有个 + 1? 把最后一个字节留给\0，保证最后以\0结束，解析http协议会方便一些
        int     new_len;
        char *  new_buf;

        new_len = conn->req->buf_max << 1;
        if ( new_len > self->tcp_recv_buf_max ) {
            if ( conn->req->buf_max < self->tcp_recv_buf_max ) {
                new_len = self->tcp_recv_buf_max;
            } else {
                gr_fatal( "%d out of max %d", conn->req->buf_max, self->tcp_recv_buf_max );
                return NULL;
            }
        }

        new_buf = (char *)gr_realloc( conn->req->buf, new_len );
        if ( NULL == new_buf ) {
            gr_fatal( "realloc %d bytes failed: %d", new_len, get_errno() );
            return NULL;
        }

        conn->req->buf     = new_buf;
        conn->req->buf_max = new_len;    
    }

    return conn->req;
}

void gr_tcp_conn_add_rsp(
    gr_tcp_conn_item_t *    conn,
    gr_tcp_rsp_t *          rsp
)
{
    rsp->entry.next = NULL;

    if ( NULL != conn->rsp_list_head ) {
        conn->rsp_list_tail->entry_compact.next = & rsp->entry_compact;
    } else {
        conn->rsp_list_head = rsp;
        conn->rsp_list_tail = rsp;
    }
}

int gr_tcp_conn_pop_top_rsp(
    gr_tcp_conn_item_t *    conn,
    gr_tcp_rsp_t *          confirm_rsp
)
{
    gr_tcp_rsp_t *              rsp;
    gr_queue_item_compact_t *   next;

    rsp = conn->rsp_list_head;
    if ( NULL == rsp ) {
        return -1;
    }
    if ( rsp != confirm_rsp ) {
        return -2;
    }

    next = rsp->entry_compact.next;

    if ( NULL != next ) {
        conn->rsp_list_head = OFFSET_RECORD( next, gr_tcp_rsp_t, entry_compact );
    } else {
        conn->rsp_list_head = NULL;
        conn->rsp_list_tail = NULL;
    }

    gr_tcp_rsp_free( rsp );
    return 0;
}

///////////////////////////////////////////////////////////////////////

gr_tcp_req_t * gr_tcp_req_alloc(
    gr_tcp_conn_item_t *    parent,
    int                     buf_max
)
{
    gr_tcp_req_t *  req;

    if ( NULL == parent || buf_max < 0 ) {
        return NULL;
    }

    req = (gr_tcp_req_t *)gr_calloc( 1, sizeof( gr_tcp_req_t ) );
    if ( NULL == req ) {
        gr_fatal( "gr_calloc %d failed: %d", (int)sizeof( gr_tcp_req_t ), get_errno() );
        return NULL;
    }

    // 标记字段，必须为1
    req->entry_compact.is_tcp   = true;
    // 引用计数为1
    req->entry_compact.refs     = 1;
    // 打个请求标记
    req->entry_compact.is_req   = true;

    req->parent = parent;
    req->buf_max = buf_max;
    if ( req->buf_max > 0 ) {
        req->buf = (char *)gr_malloc( req->buf_max );
        if ( NULL == req->buf ) {
            gr_fatal( "gr_malloc %d failed: %d", req->buf_max, get_errno() );
            gr_free( req );
            return NULL;
        }
    }

    return req;
}

int gr_tcp_req_free(
    gr_tcp_req_t *      req
)
{
    if ( NULL == req || NULL == req->parent || req->entry_compact.refs <= 0 ) {
        return -1;
    }

    -- req->entry_compact.refs;

    if ( 0 == req->entry_compact.refs ) {

        req->buf_len = 0;
        req->buf_max = 0;
        if ( NULL != req->buf ) {
            gr_free( req->buf );
            req->buf = NULL;
        }

        gr_free( req );
    }

    return 0;
}

int gr_tcp_req_add_refs(
    gr_tcp_req_t *          req
)
{
    if ( NULL == req || req->entry_compact.refs >= CHAR_MAX ) {
        gr_fatal( "inalid refs" );
        return -1;
    }

    ++ req->entry_compact.refs;
    return 0;
}

void gr_tcp_req_to_rsp(
    gr_tcp_req_t *          req
)
{
    // 将它改成false，就是回复了
    req->entry_compact.is_req = false;
}

void gr_tcp_req_set_buf(
    gr_tcp_req_t *          req,
    void *                  buf,
    int                     buf_max,
    int                     buf_len
)
{
    if ( req->buf ) {
        gr_free( req->buf );
    }

    req->buf       = buf;
    req->buf_max   = buf_max;
    req->buf_len   = buf_len;
    req->buf_sent  = 0;
}

///////////////////////////////////////////////////////////////////////

gr_udp_req_t * gr_udp_req_alloc(
    int                     buf_max
)
{
    gr_udp_req_t *  req;

    if ( buf_max < 0 ) {
        return NULL;
    }

    req = (gr_udp_req_t *)gr_calloc( 1, sizeof( gr_udp_req_t ) );
    if ( NULL == req ) {
        return NULL;
    }

    // 标记字段，必须为0
    req->entry_compact.is_tcp   = false;
    // 引用计数为1
    req->entry_compact.refs     = 1;

    req->buf_max = buf_max;
    if ( req->buf_max > 0 ) {
        req->buf = (char *)gr_malloc( req->buf_max );
        if ( NULL == req->buf ) {
            gr_free( req );
            return NULL;
        }
    }

    return req;
}

int gr_udp_req_free(
    gr_udp_req_t *          req
)
{
    if ( NULL == req || req->entry_compact.refs <= 0 ) {
        return -1;
    }

    -- req->entry_compact.refs;

    if ( 0 == req->entry_compact.refs ) {

        req->buf_len = 0;
        req->buf_max = 0;
        if ( NULL != req->buf ) {
            gr_free( req->buf );
            req->buf = NULL;
        }

        gr_free( req );
    }

    return 0;
}
