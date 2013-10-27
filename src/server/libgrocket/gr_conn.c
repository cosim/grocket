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
 *       2     zouyueming   2013-10-22    add entry_compact field is first member check in runtime.
 *       3     zouyueming   2013-10-27    support tcp out disable
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
#include "gr_module.h"

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

    if ( GR_ATOMIC_T_LEN != sizeof( gr_atomic_t ) ) {
        gr_fatal( "sizeof( gr_atomic_t ) = %d, not %d",
            (int)sizeof( gr_atomic_t ), GR_ATOMIC_T_LEN );
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

    {
        gr_tcp_req_t                req;
        gr_queue_item_compact_t *   entry = & req.entry_compact;
        if ( & req != OFFSET_RECORD( entry, gr_tcp_req_t, entry_compact ) ) {
            gr_fatal( "entry_compact must be first member in gr_tcp_req_t" );
            return -1;
        }
    }

    {
        gr_tcp_rsp_t                rsp;
        gr_queue_item_compact_t *   entry = & rsp.entry_compact;
        if ( & rsp != OFFSET_RECORD( entry, gr_tcp_rsp_t, entry_compact ) ) {
            gr_fatal( "entry_compact must be first member in gr_tcp_rsp_t" );
            return -1;
        }
    }

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

    // req_proc_count 值为0说明req_proc_count和req_push_count在维护，这时的数据是不准确的，不能读
    // 所以要小心初始化顺序
    conn->req_push_count    = 1;
    conn->req_proc_count    = 1;

    conn->close_type        = GR_OPENING;

    return conn;
}

void gr_tcp_conn_clear_rsp_list(
    gr_tcp_conn_item_t *    conn
)
{
    gr_tcp_rsp_t *  rsp = conn->rsp_list_head;
    gr_tcp_rsp_t *  next_rsp;
    conn->rsp_list_head = NULL;
    conn->rsp_list_tail = NULL;
    conn->rsp_list_curr = NULL;
    while ( NULL != rsp ) {
        next_rsp = (gr_tcp_rsp_t *)rsp->entry_compact.next;
        gr_tcp_rsp_free( rsp );
        rsp = next_rsp;
    }
}

void gr_tcp_conn_del_receiving_req(
    gr_tcp_conn_item_t *    conn
)
{
    if ( NULL != conn->req ) {
        gr_tcp_req_t * req = conn->req;
        conn->req = NULL;
        gr_tcp_req_free( req );
    }
}

void gr_tcp_conn_free(
    gr_tcp_conn_item_t *    conn
)
{
    if ( NULL != conn ) {

        int fd = conn->fd;

#ifdef GR_DEBUG_CONN
        gr_info( "[req.push=%d][req.proc=%d][rsp.send=%llu]free tcp_conn %p",
            conn->req_push_count, conn->req_proc_count, conn->rsp_send_count, conn );
#else
        gr_info( "[req.push=%d][req.proc=%d]free tcp_conn %p",
            conn->req_push_count, conn->req_proc_count, conn );
#endif
        if ( -1 != conn->fd ) {
            // 关闭连接前回调模块一下
            gr_module_on_tcp_close( conn );
            conn->fd = -1;
        }

        // 删除正在接收的请求
        gr_tcp_conn_del_receiving_req( conn );

        // 删除回复列表
        gr_tcp_conn_clear_rsp_list( conn );

        gr_free( conn );

        gr_debug( "gr_tcp_conn_free leave %p", conn );

        // 关socket
        // we must close the socket at last!!!
        if ( -1 != fd ) {
            gr_socket_close( fd );
        }
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
    gr_tcp_rsp_t * will_del = NULL;
    gr_tcp_rsp_t * t;
    gr_tcp_rsp_t * curr;

    curr = conn->rsp_list_curr;
    if ( QUEUE_ALL_DONE == curr ) {
        curr = NULL;
    }

    // conn->rsp_list_curr 为空说明没有任何表项被处理, 所以必须 conn->rsp_list_curr 非空才进循环
    if ( NULL != conn->rsp_list_curr ) {
        // 从队列头找到要删除的所有数据项,摘成一个子链表: will_del
        will_del = conn->rsp_list_head;
        t = NULL;
        while ( NULL != conn->rsp_list_head && conn->rsp_list_head != curr ) {
            // 记录最后访问过的表项,因为要把最后一个表项的next设为NULL
            t = conn->rsp_list_head;
            // 向后移动conn->rsp_list_head指针
            gr_debug( "[from=%p][to=%p]move conn->rsp_list_head",
                conn->rsp_list_head, conn->rsp_list_head->entry_compact.next );
            conn->rsp_list_head = (gr_tcp_rsp_t *)conn->rsp_list_head->entry_compact.next;
        }
        // 把最后一个表项的next设为NULL, 这样 will_del 就是一个要删除的单链表了
        if ( NULL != t ) {
            t->entry_compact.next = NULL;
            gr_debug( "[last=%p]will del tail node", t );
        }
        // 注意,如果conn->rsp_list_head为NULL,则此时conn->rsp_list_tail还非NULL呢
    }

    // 将节点插入队列
    rsp->entry_compact.next = NULL;
    if ( NULL == conn->rsp_list_head ) {
        // 前面说的应该为NULL的worker->tail被一块儿赋了值
        conn->rsp_list_head = conn->rsp_list_tail = rsp;
        gr_debug( "insert rsp %p to empty", rsp );
    } else if ( conn->rsp_list_head == conn->rsp_list_tail ) {
        // 更新尾节点
        conn->rsp_list_tail = rsp;
        conn->rsp_list_head->entry_compact.next = (gr_queue_item_compact_t *)rsp;
        gr_debug( "insert rsp %p to single node list, head=%p, tail=%p",
            rsp, conn->rsp_list_head, conn->rsp_list_tail );
    } else {
        // 记住当前链表尾节点
        t = conn->rsp_list_tail;
        // 更新尾节点
        conn->rsp_list_tail = rsp;
        // 将原先尾节点的指针指向新增节点
        t->entry_compact.next = (gr_queue_item_compact_t *)rsp;
        gr_debug( "insert rsp %p to after %p, head = %p", rsp, t, conn->rsp_list_head );
    }

    if ( QUEUE_ALL_DONE == conn->rsp_list_curr ) {
        // 如果发现输出线程已经处理完了, 则重置 conn->rsp_list_curr 指针
        conn->rsp_list_curr = conn->rsp_list_head;

        //TODO: 应该在这儿唤醒输出线程干活
    }

    // 到现在输出线程已经接着干活了, 我可以安心的删除节点了
    while ( NULL != will_del && will_del != curr ) {
        t = will_del;
        will_del = (gr_tcp_rsp_t *)will_del->entry_compact.next;
        gr_debug( "[del=%p][next=%p][insert=%p] del processed node", t, will_del, rsp );
        assert( t != rsp );
        gr_tcp_rsp_free( t );
    }
}

int gr_tcp_conn_pop_top_rsp(
    gr_tcp_conn_item_t *    conn,
    gr_tcp_rsp_t *          confirm_rsp
)
{
    gr_tcp_rsp_t *  next;

    // 从队列中删除
    assert( conn->rsp_list_curr == confirm_rsp );
    gr_debug( "[rsp=%p][next=%p]will pop rsp",
        conn->rsp_list_curr, conn->rsp_list_curr->entry_compact.next );
    next = (gr_tcp_rsp_t *)conn->rsp_list_curr->entry_compact.next;

    if ( NULL == next ) {
        conn->rsp_list_curr = QUEUE_ALL_DONE;
    } else {
        conn->rsp_list_curr = next;
    }

#ifdef GR_DEBUG_CONN
    // 增加发包计数
    ++ conn->rsp_send_count;
#endif

    return 0;
}

void gr_tcp_conn_pop_top_req(
    gr_tcp_conn_item_t *    conn,
    bool                    tcp_out_disabled
)
{
    int req_proc_count;

    assert( conn->req_push_count >= conn->req_proc_count );
    
    // 现在不能增加请求弹包数量，否则可能会在其它线程中删除连接对象
    req_proc_count = conn->req_proc_count;

    if ( req_proc_count > GR_REQ_PUSH_COUNT_HI_WALTER ) {
        // 将弹包数量和压包数量同比减去一个固定值，防止溢出

        // 将conn->req_proc_count设成0, 这会让所有取用该值的人知道此值当前无效
        conn->req_proc_count = 0;

        // 因为这和in线程会有并发写，所以用原子操作
        gr_atomic_add( -req_proc_count, & conn->req_push_count );
        // 因为现在没其它人操作req_proc_count字段，所以不用原子操作
    }

    if ( conn->close_type > GR_NEED_CLOSE ) {
        // 最后再增加请求弹包数量, conn->req_proc_count 值非0才表示有效
        ++ conn->req_proc_count;
        // 解锁
        conn->worker_locked = 0;
        // 本函数退出后有可能连接会被删除
        return;
    }

    // 正在断连接过程中

    // 这个地方限制: 一个TCP的处理必须分配到一个固定的工作线程, 这也是合理和安全的
    // 判断当前请求是否是最后一个待处理的请求
    if ( conn->req_proc_count + 1 != conn->req_push_count ) {
        // 当前请求不是最后一个待处理的请求
        // 增加请求弹包数量
        ++ conn->req_proc_count;
        // 解锁
        conn->worker_locked = 0;
        // 本函数退出后有可能连接会被删除
        return;
    }

    if ( ! conn->worker_open ) {
        // 增加请求弹包数量
        ++ conn->req_proc_count;
        // 解锁
        conn->worker_locked = 0;
        // 本函数退出后有可能连接会被删除
        return;
    }

    // 当前请求是最后一个待处理的请求
    // 增加当前正在处理的请求弹包数量
    ++ conn->req_proc_count;
    // 解锁
    conn->worker_locked = 0;
    // 通知TCP连接关闭模块
    gr_tcp_close_from_worker( conn, tcp_out_disabled );
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
    //req->entry_compact.refs     = 1;
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

    gr_debug( "alloc req %p, conn %p", req, parent );

    return req;
}

void gr_tcp_req_free(
    gr_tcp_req_t *      req
)
{
    // 为安全起见, 已经在合适的时候把 parent 设成 NULL 了
    assert( NULL != req /*&& NULL != req->parent*/ );

    req->buf_len = 0;
    req->buf_max = 0;
    if ( NULL != req->buf ) {
        gr_free( req->buf );
        req->buf = NULL;
    }

    gr_debug( "free req %p, conn %p", req, req->parent );

    gr_free( req );
}

/*int gr_tcp_req_add_refs(
    gr_tcp_req_t *          req
)
{
    if ( NULL == req || req->entry_compact.refs >= CHAR_MAX ) {
        gr_fatal( "inalid refs" );
        return -1;
    }

    ++ req->entry_compact.refs;
    return 0;
}*/

/*void gr_tcp_req_to_rsp(
    gr_tcp_req_t *          req
)
{
    // 将它改成false，就是回复了
    req->entry_compact.is_req = false;
}*/

void gr_tcp_req_set_buf(
    gr_tcp_req_t *          req,
    void *                  buf,
    int                     buf_max,
    int                     buf_len
)
{
    if ( req->buf && req->buf != buf ) {
        gr_free( req->buf );
    }

    req->buf       = buf;
    req->buf_max   = buf_max;
    req->buf_len   = buf_len;
    req->buf_sent  = 0;
}

///////////////////////////////////////////////////////////////////////

gr_tcp_rsp_t * gr_tcp_rsp_alloc(
    gr_tcp_conn_item_t *    parent,
    int                     buf_max
)
{
    gr_tcp_rsp_t *  rsp;

    if ( NULL == parent || buf_max < 0 ) {
        return NULL;
    }

    rsp = (gr_tcp_rsp_t *)gr_calloc( 1, sizeof( gr_tcp_rsp_t ) );
    if ( NULL == rsp ) {
        gr_fatal( "gr_calloc %d failed: %d", (int)sizeof( gr_tcp_rsp_t ), get_errno() );
        return NULL;
    }

    // 标记字段，必须为1
    rsp->entry_compact.is_tcp   = true;
    // 引用计数为1
    //rsp->entry_compact.refs     = 1;
    // 打个返回标记
    rsp->entry_compact.is_req   = false;

    rsp->parent = parent;
    rsp->buf_max = buf_max;
    if ( rsp->buf_max > 0 ) {
        rsp->buf = (char *)gr_malloc( rsp->buf_max );
        if ( NULL == rsp->buf ) {
            gr_fatal( "gr_malloc %d failed: %d", rsp->buf_max, get_errno() );
            gr_free( rsp );
            return NULL;
        }
    }

    gr_debug( "gr_tcp_rsp_alloc %p", rsp );
    return rsp;
}


void gr_tcp_rsp_free(
    gr_tcp_rsp_t *      rsp
)
{
    gr_debug( "gr_tcp_rsp_free %p", rsp );

    assert( NULL != rsp && NULL != rsp->parent );

    rsp->buf_len = 0;
    rsp->buf_max = 0;
    if ( NULL != rsp->buf ) {
        gr_free( rsp->buf );
        rsp->buf = NULL;
    }

    gr_free( rsp );
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
    //req->entry_compact.refs     = 1;

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
    assert( NULL != req /*&& req->entry_compact.refs > 0*/ );

    //-- req->entry_compact.refs;

    //if ( 0 == req->entry_compact.refs ) {

        req->buf_len = 0;
        req->buf_max = 0;
        if ( NULL != req->buf ) {
            gr_free( req->buf );
            req->buf = NULL;
        }

        gr_free( req );
    //}
}
