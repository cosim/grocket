/**
 * @file libgrocket/gr_conn.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   tcp connection, udp connection, request
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

#include "gr_conn.h"
#include "gr_log.h"
#include "gr_global.h"
#include "gr_tools.h"
#include "gr_mem.h"
#include "gr_socket.h"
#include "gr_config.h"
#include "gr_tcp_close.h"

#define GR_REQ_PUSH_COUNT_HI_WALTER     1500000000

typedef struct
{
    int     tcp_recv_buf_init;
    int     tcp_recv_buf_max;

} gr_conn_engine_t;

int gr_conn_init()
{
    gr_conn_engine_t *  p;

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

    // req_pop_count 值为0说明req_pop_count和req_push_count在维护，这时的数据是不准确的，不能读
    // 所以要小心初始化顺序
    conn->req_push_count    = 1;
    conn->req_pop_count     = 1;

    conn->close_type        = GR_OPENING;

    return conn;
}

static inline
void clear_rsp_list(
    gr_tcp_conn_item_t *    conn
)
{
    gr_tcp_rsp_t *  rsp = conn->rsp_list_head;
    gr_tcp_rsp_t *  next_rsp;
    conn->rsp_list_head = NULL;
    conn->rsp_list_tail = NULL;
    while ( NULL != rsp ) {
        next_rsp = (gr_tcp_rsp_t *)rsp->entry_compact.next;
        gr_tcp_rsp_free( rsp );
        rsp = next_rsp;
    }
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

        // 删除正在接收的请求
        if ( NULL != conn->req ) {
            gr_tcp_req_free( conn->req );
            conn->req = NULL;
        }

        // 删除回复列表
        clear_rsp_list( conn );

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
    rsp->entry_compact.next = NULL;

    if ( NULL != conn->rsp_list_head ) {
        conn->rsp_list_tail->entry_compact.next = & rsp->entry_compact;
    } else {
        conn->rsp_list_head = rsp;
        conn->rsp_list_tail = rsp;
    }
}

int gr_tcp_conn_pop_top_rsp(
    gr_tcp_conn_item_t *    conn,
    gr_tcp_rsp_t *          confirm_rsp,
    bool                    and_destroy_it
)
{
    gr_queue_item_compact_t *   next;

    if ( conn->rsp_list_head != confirm_rsp ) {
        gr_error( "conn->rsp_list_head not equal with confirm_rsp" );
        return -2;
    }

    next = conn->rsp_list_head->entry_compact.next;

    if ( NULL != next ) {
        conn->rsp_list_head = OFFSET_RECORD( next, gr_tcp_rsp_t, entry_compact );
    } else {
        conn->rsp_list_head = NULL;
        conn->rsp_list_tail = NULL;
    }

gr_info( "================= will kill rsp %p, %d, refs=%d",
    confirm_rsp, (int)and_destroy_it, (int)confirm_rsp->entry_compact.refs );

    if( and_destroy_it ) {
        gr_tcp_rsp_free( confirm_rsp );
    }
    return 0;
}

void gr_tcp_conn_pop_top_req(
    gr_tcp_conn_item_t *    conn
)
{
    gr_atomic_t req_pop_count;

    assert( conn->req_push_count >= conn->req_pop_count );
    
    // 现在不能增加请求弹包数量，否则可能会在其它线程中删除连接对象
    req_pop_count = conn->req_pop_count;

    if ( req_pop_count > GR_REQ_PUSH_COUNT_HI_WALTER ) {
        // 将弹包数量和压包数量同比减去一个固定值，防止溢出

        // 将conn->req_pop_count设成0, 这会让所有取用该值的人知道此值当前无效
        conn->req_pop_count = 0;

        // 因为这和in线程会有并发写，所以用原子操作
        gr_atomic_add( -req_pop_count, & conn->req_push_count );
        // 因为现在没其它人操作req_pop_count字段，所以不用原子操作
    }

    if ( conn->close_type > GR_NEED_CLOSE ) {
        // 最后再增加请求弹包数量, conn->req_pop_count 值非0才表示有效
        ++ conn->req_pop_count;
        // 本函数退出后有可能连接会被删除
        return;
    }

    // 正在断连接过程中

    // 这个地方限制: 一个TCP的处理必须分配到一个固定的工作线程, 这也是合理和安全的
    // 判断当前请求是否是最后一个待处理的请求
    if ( conn->req_pop_count + 1 != conn->req_push_count ) {
        // 当前请求不是最后一个待处理的请求
        // 增加请求弹包数量
        ++ conn->req_pop_count;
        // 本函数退出后有可能连接会被删除
        return;
    }

    // 当前请求是最后一个待处理的请求
    // 通知TCP连接关闭模块
    gr_tcp_close_from_work( conn );
    // 本函数退出后有可能连接会被删除
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

gr_info( "================ new req %p", req );
    return req;
}

void gr_tcp_req_free(
    gr_tcp_req_t *      req
)
{
    assert( NULL != req && NULL != req->parent && req->entry_compact.refs > 0 );

    -- req->entry_compact.refs;

    if ( 0 == req->entry_compact.refs ) {

        req->buf_len = 0;
        req->buf_max = 0;
        if ( NULL != req->buf ) {
            gr_free( req->buf );
            req->buf = NULL;
        }

        gr_free( req );

gr_info( "================= real free req %p", req );
    }
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

void gr_udp_req_free(
    gr_udp_req_t *          req
)
{
    assert( NULL != req && req->entry_compact.refs > 0 );

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
}
