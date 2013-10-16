/**
 * @file libgrocket/gr_worker.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   worker
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

// 别忘了worker的队列只能一个线程push

#include "gr_worker.h"
#include "gr_thread.h"
#include "gr_log.h"
#include "gr_global.h"
#include "gr_errno.h"
#include "gr_tools.h"
#include "gr_mem.h"
#include "gr_config.h"
#include "gr_module.h"
#include "gr_conn.h"
#include "gr_tcp_out.h"
#include "gr_udp_out.h"
#include "gr_event.h"

        struct gr_worker_t;
typedef struct gr_worker_t      gr_worker_t;
        struct gr_worker_item_t;
typedef struct gr_worker_item_t gr_worker_item_t;

typedef struct
{
    gr_proc_ctxt_t  bin_ctxt;
    gr_proc_http_t  http_ctxt;
} per_thread_t;

struct gr_worker_t
{
    gr_threads_t        threads;

    gr_worker_item_t *  items;
};

struct gr_worker_item_t
{
    gr_queue_item_compact_t *   head;
    gr_queue_item_compact_t *   tail;
    gr_queue_item_compact_t *   curr;

    gr_event_t                  event;
    volatile bool               in_event;
};


static
void free_tcp_req( gr_tcp_req_t * req )
{
    gr_tcp_conn_item_t * conn = req->parent;

    // 调用真正的释放TCP请求的函数
    gr_tcp_req_free( req );

    // 将当前req已经处理完并弹出的消息告诉conn，该函数之后有可能连接对象就被删掉了
    gr_tcp_conn_pop_top_req( conn );
}

static inline
void worker_free_queue_item( gr_queue_item_compact_t * queue_item )
{
    static gr_func_req_free_t  free_queue_item_funcs[ 2 ] =
    {
        (gr_func_req_free_t)gr_udp_req_free,
        (gr_func_req_free_t)free_tcp_req
    };

    // avoid if statment
    assert( 1 == queue_item->is_tcp || 0 == queue_item->is_tcp );
    free_queue_item_funcs[ queue_item->is_tcp ]( queue_item );
}

#define QUEUE_ALL_DONE  ((gr_queue_item_compact_t *)1)

static inline
void alarm_event_if_need(
    gr_worker_item_t *          worker
)
{
    if ( worker->in_event ) {
        worker->in_event = false;
        gr_event_alarm( & worker->event );
    }
}

static inline
void worker_queue_push(
    gr_worker_item_t *          worker,
    gr_queue_item_compact_t *   item
)
{
    gr_queue_item_compact_t * will_del = NULL;
    gr_queue_item_compact_t * t;
    gr_queue_item_compact_t * curr;
    
    curr = worker->curr;
    if ( QUEUE_ALL_DONE == curr ) {
        curr = NULL;
    }

    // worker->curr 为空说明没有任何表项被处理
    if ( NULL != worker->curr ) {
        // 从队列头找到要删除的所有数据项,摘成一个子链表 will_del
        t = NULL;
        will_del = worker->head;
        while ( NULL != worker->head && worker->head != curr ) {
            t = worker->head;
            worker->head = worker->head->next;
        }
        if ( NULL != t ) {
            t->next = NULL;
        }
        // 注意,如果worker->head为NULL,则此时worker->tail还非NULL呢
    }

    // 将节点插入队列
    item->next = NULL;
    if ( NULL == worker->head ) {
        worker->head = worker->tail = item;
    } else {
        worker->tail = item;
        worker->tail->next = item;
    }

    if ( QUEUE_ALL_DONE == worker->curr ) {
        worker->curr = worker->head;

        // 只有在工作线程确实没事儿干时才需要判断是否在事件里
        alarm_event_if_need( worker );
    }

    // 到现在工作线程已经接着干活了, 我可以安心的删除节点了
    while ( NULL != will_del ) {
        t = will_del;
        will_del = will_del->next;
        worker_free_queue_item( t );
    }
}

static inline
void worker_queue_destroy(
    gr_worker_item_t *          worker
)
{
    gr_queue_item_compact_t *   item;

    worker->tail = NULL;
    while ( NULL != ( item = worker->head ) ) {
        worker->head = item->next;
        worker_free_queue_item( item );
    }

    gr_event_destroy( & worker->event );

}

static inline
gr_queue_item_compact_t * worker_queue_top_inner(
    gr_worker_item_t *      worker
)
{
    if ( NULL == worker->curr ) {
        worker->curr = worker->head;
    } else if ( QUEUE_ALL_DONE == worker->curr ) {
        return NULL;
    }

    return worker->curr;
}

static inline
gr_queue_item_compact_t * worker_queue_top(
    gr_worker_item_t *      worker
)
{
    size_t i;
    size_t j;
    gr_queue_item_compact_t * p = NULL;

    // 高并发时,用类似自旋锁的思想,尽量不进内核事件
    for ( j = 0; j < 4; ++ j ) {

        for ( i = 0; i < 3; ++ i ) {
            p = worker_queue_top_inner( worker );
            if ( NULL != p ) {
                return p;
            }
        }

        gr_yield();
    }

    return NULL;
}

static inline
void worker_queue_pop_top(
    gr_worker_item_t *          worker,
    gr_queue_item_compact_t *   item
)
{
    assert( worker->curr == item );
    worker->curr = worker->curr->next;

    if ( NULL == worker->curr ) {
        worker->curr = QUEUE_ALL_DONE;
    }
}

static inline
void process_tcp(
    gr_worker_t *   self,
    gr_thread_t *   thread,
    gr_tcp_req_t *  req
)
{
    per_thread_t *      per_thread      = (per_thread_t *)thread->cookie;
    gr_proc_ctxt_t *    ctxt            = & per_thread->bin_ctxt;
    // 默认已处理字节数是输入字节数
    int                 processed_len   = req->buf_len;
    // 记录一下原始的请求包
    char *              req_buf         = req->buf;
    int                 req_buf_max     = req->buf_max;
    int                 req_buf_len     = req->buf_len;
    int     r;

    ctxt->check_ctxt    = & req->check_ctxt;
    ctxt->port          = req->parent->port_item->port;
    ctxt->fd            = req->parent->fd;
    ctxt->thread_id     = thread->id;

gr_info( "================= proc_tcp %p, refs=%d", req, (int)req->entry_compact.refs );

    // 调用模块处理函数处理TCP请求
    gr_module_proc_tcp( req, ctxt, & processed_len );

    // 检查模块处理函数处理结果
    if ( processed_len < 0 ) {
        // processed_len 小于0表示需要服务器断掉连接，返回数据包也不要发
        ctxt->pc_result_buf_len    = 0;

        if ( req->parent->close_type > GR_NEED_CLOSE ) {
            req->parent->close_type = GR_NEED_CLOSE;
        }
    } else if ( 0 == processed_len ) {
        // processed_len 等于0表示需要服务器断掉连接,但当前返回数据包继续发
        if ( req->parent->close_type > GR_NEED_CLOSE ) {
            req->parent->close_type = GR_NEED_CLOSE;
        }
    }

    if ( ctxt->pc_result_buf_len <= 0 ) {
        // 没有回复数据包
        return;
    }

    // 有回复数据包

    // 检查网络是否异常
    if ( req->parent->close_type < GR_NEED_CLOSE || req->parent->is_network_error ) {
        // 如果状态已经进行到GR_NEED_CLOSE的下一步，则说明模块已经确认连接关闭，不需要再发数据包了。
        // 如果网络异常，则不需要发送数据
        // 把数据长度清0即可，等下次用。
        ctxt->pc_result_buf_len    = 0;
        return;
    }

    // 增加引用计数。因为 worker 在调用完本函数后会删除它，而在本函数中会直接将req转变成rsp扔到conn->rsp_list中
    gr_tcp_req_add_refs( req );

    // 将返回包应用到req中，这个req将被改造成rsp
    gr_tcp_req_set_buf( req, ctxt->pc_result_buf, ctxt->pc_result_buf_max, ctxt->pc_result_buf_len );
    // 由于用户模块的返回数据已经从ctxt移动到返回包里了，所以要将ctxt中记录的返回数据信息清掉。
    ctxt->pc_result_buf        = NULL;
    ctxt->pc_result_buf_max    = 0;
    ctxt->pc_result_buf_len    = 0;

    // 将req变成rsp
    gr_tcp_req_to_rsp( req );

    // 将rsp扔到连接的队列中
    gr_tcp_conn_add_rsp( req->parent, req );

    // 将rsp放入tcp_out中发送
    r = gr_tcp_out_add( req );
    if ( 0 != r ) {
        gr_fatal( "gr_tcp_out_add faiiled" );
        //TODO: 按理说，这个地方出错了应该断连接了。但这儿应该是机制出错了，不是断连接这么简单，可能需要重启服务器才能解决。
        // 所以干脆不处理了。理论上这种可能性出现的概率为0
        return;
    }
}

static inline
void process_udp(
    gr_worker_t *   self,
    gr_thread_t *   thread,
    gr_udp_req_t *  req
)
{
    // echo
    gr_udp_out_add( req );
}

static inline
int hash_worker_tcp( gr_tcp_req_t * req, gr_worker_t * self )
{
    // TCP，按SOCKET描述符算HASH
    return req->parent->fd % self->threads.thread_count;
}

static inline
int hash_worker_udp( gr_udp_req_t * req, gr_worker_t * self )
{
    // UDP, 按客户端IP算HASH
    if ( AF_INET == req->addr.sa_family ) {
        // IPV4
        return req->addr_v4.sin_addr.s_addr % self->threads.thread_count;
    } else if ( AF_INET6 == req->addr.sa_family ) {
        // IPV6
        //TODO: 性能需要优化一下
        const unsigned char *   p = (const unsigned char *)& req->addr_v6.sin6_addr;
        int                     n = 0;
        size_t                  i;
        for ( i = 0; i < sizeof( req->addr_v6.sin6_addr ); ++ i ) {
            n = n * 13 + p[ i ];
        }
        return abs( n ) % self->threads.thread_count;
    }

    return 0;
}

static inline
int gr_worker_add(
    gr_queue_item_compact_t *   queue_item,
    bool                        is_emerge
)
{
    int             hash_id;
    gr_worker_t *   self;
    
    self = (gr_worker_t *)g_ghost_rocket_global.worker;
    if ( NULL == self ) {
        return -1;
    }

    if ( queue_item->is_tcp ) {
        gr_tcp_req_t *  req     = (gr_tcp_req_t *)queue_item;
        hash_id = hash_worker_tcp( req, self );
        // 该标记表示连接已经在工作线程里
        req->parent->worker_open = true;
    } else {
        gr_udp_req_t *  req = (gr_udp_req_t *)queue_item;
        hash_id = hash_worker_udp( req, self );
    }

    worker_queue_push( & self->items[ hash_id ], queue_item );

    return 0;
}

static
void worker_init_routine( gr_thread_t * thread )
{
    gr_module_worker_init( thread->id );
}

static
void worker_routine( gr_thread_t * thread )
{
#define     WORK_WAIT_TIMEOUT   100
    gr_worker_t *               self        = (gr_worker_t *)thread->param;
    gr_worker_item_t *          item        = & self->items[ thread->id ];
    gr_queue_item_compact_t *   queue_item;

    typedef int ( * func_proc_item_t )( gr_worker_t * self, gr_thread_t * thread, void * req );
    static func_proc_item_t  proc_item_funcs[ 2 ] =
    {
        (func_proc_item_t)process_udp,
        (func_proc_item_t)process_tcp
    };

    while ( ! thread->is_need_exit ) {

        // 取包
        queue_item = worker_queue_top( item );
        if ( NULL != queue_item ) {

            // 处理
            assert( 1 == queue_item->is_tcp || 0 == queue_item->is_tcp );
            proc_item_funcs[ queue_item->is_tcp ]( self, thread, queue_item );

            // 弹包
            worker_queue_pop_top( item, queue_item );

        } else {
            // 用事件等
            item->in_event = true;
            gr_event_wait( & item->event, 1 );
            item->in_event = false;
        }
    };

    gr_module_worker_term( thread->id );
}

int gr_worker_init()
{
    gr_worker_t *  p;
    int thread_count = gr_config_worker_thread_count();
    int r;
    int i;

    if ( thread_count < 1 ) {
        gr_fatal( "[init]gr_worker_init thread_count invalid" );
        return GR_ERR_INVALID_PARAMS;
    }

    if ( NULL != g_ghost_rocket_global.worker ) {
        gr_fatal( "[init]gr_work_init already called" );
        return GR_ERR_WRONG_CALL_ORDER;
    }

    p = (gr_worker_t *)gr_calloc( 1, sizeof( gr_worker_t ) );
    if ( NULL == p ) {
        gr_fatal( "[init]malloc %d bytes failed, errno=%d,%s",
            (int)sizeof(gr_worker_t), errno, strerror( errno ) );
        return GR_ERR_BAD_ALLOC;
    }

    r = GR_ERR_UNKNOWN;

    do {

        const char * name = "svr.worker";

        p->items = (gr_worker_item_t *)gr_calloc( 1,
            sizeof( gr_worker_item_t ) * thread_count );
        if ( NULL == p->items ) {
            gr_fatal( "gr_calloc %d bytes failed: %d",
                (int)sizeof( gr_worker_item_t ) * thread_count,
                get_errno() );
            r = GR_ERR_BAD_ALLOC;
            break;
        }

        // 初始化事件对象
        for ( i = 0; i < thread_count; ++ i ) {
            gr_event_create( & p->items[ i ].event );
        }

        r = gr_threads_start(
            & p->threads,
            thread_count,
            worker_init_routine,
            worker_routine,
            p,
            sizeof( per_thread_t ),
            true,
            name );
        if ( GR_OK != r ) {
            gr_fatal( "gr_threads_start return error %d", r );
            break;
        }

        gr_debug( "worker_init OK" );

        r = GR_OK;
    } while ( false );

    if ( GR_OK != r ) {

        gr_threads_close( & p->threads );

        for ( i = 0; i < thread_count; ++ i ) {
            worker_queue_destroy( & p->items[ i ] );
        }

        if ( NULL != p->items ) {
            gr_free( p->items );
            p->items = NULL;
        }

        gr_free( p );
        return r;
    }

    g_ghost_rocket_global.worker = p;
    return GR_OK;
}

void gr_worker_term()
{
    gr_worker_t *  p = (gr_worker_t *)g_ghost_rocket_global.worker;
    if ( NULL != p ) {

        int i;

        gr_threads_close( & p->threads );

        for ( i = 0; i < p->threads.thread_count; ++ i ) {
            worker_queue_destroy( & p->items[ i ] );
        }

        if ( NULL != p->items ) {
            gr_free( p->items );
            p->items = NULL;
        }

        g_ghost_rocket_global.worker = NULL;
        gr_free( p );
    }
}

int gr_worker_add_tcp(
    gr_tcp_req_t *  req,
    bool            is_emerge
)
{
    int                         r;
    int                         package_len;
    int                         left_len;
    gr_queue_item_compact_t *   queue_item;
    gr_tcp_req_t *              new_req;
    
    new_req = NULL;

    queue_item = (gr_queue_item_compact_t *)req;
    assert( queue_item->is_tcp );

    // 取得完整数据包长度
    package_len = gr_tcp_req_package_length( req );
    // 计算请求对象里还剩多少字节数据
    left_len    = req->buf_len - package_len;
    if ( left_len > 0 ) {

        // pipe line 请求支持

        // 缓冲区里是多包数据，剩余的数据不能放在当前请求中，要放回连接对象。
        // 从这儿可以看出，如果模块支持多包数据同时接收，最好是在判断包长度时判断多个包，
        // 让剩下来的半包数据最小，这样拷贝的开销才最小。
        // 必须将 req 扔给 worker，不能将新分配的 new_req 扔给 worker 因为老的 req 有很多状态
        // 要是新分配再拷贝，太不划算。

        // 分配个新的请求对象，将剩余数据拷贝到新分配的请求对象中。如果如果剩余的字节数较小会比较划算。
        new_req = gr_tcp_req_alloc( req->parent, req->buf_max );
        if ( NULL == new_req ) {
            gr_fatal( "gr_tcp_req_alloc failed" );
            return -2;
        }

        // 将完整包以外的剩余数据拷贝到新分配的请求对象中
        new_req->buf_len = left_len;
        memcpy( new_req->buf, & req->buf[ package_len ], left_len );

        // 将剩余的数据放回conn中
        req->parent->req = new_req;

        // 修改请求包数据长度和实际包长度一致
        req->buf_len = package_len;
    } else {
        // 这说明请求中存放的是单包数据
        // 将请求对象从conn中摘出来
        req->parent->req = NULL;
    }

    // 将请求放到连接的请求列表尾
    gr_tcp_conn_add_req( req->parent );

    // 试图将请求加入worker
    r = gr_worker_add( queue_item, is_emerge );
    if ( 0 == r ) {
        return 0;
    }

    // 向worker中压包失败

    // 要将请求从请求列表尾重新摘出来
    gr_tcp_conn_pop_tail_req( req->parent );

    // 还要再把请求对象扔回连接对象
    if ( NULL != new_req ) {

        // 有多包数据，要删除刚分配出的存放剩余数据的请求
        // 恢复原来数据长度
        req->buf_len = package_len + left_len;

        // 删除刚刚分配的请求包
        gr_tcp_req_free( new_req );
    }
    req->parent->req = req;

    return r;
}

int gr_worker_add_udp(
    gr_tcp_req_t *  req,
    bool            is_emerge
)
{
    gr_queue_item_compact_t * queue_item = (gr_queue_item_compact_t *)req;
    if ( false != queue_item->is_tcp ) {
        return -1;
    }

    return gr_worker_add( queue_item, is_emerge );
}