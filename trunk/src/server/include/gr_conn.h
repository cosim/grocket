/**
 * @file inxlude/gr_conn.h
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

#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_CONN_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_CONN_H_

#include "gr_stdinc.h"
#include "gr_compiler_switch.h"
#include "grocket.h"
#include "gr_atomic.h"
#include "gr_log.h"
#include "gr_global.h"
#include "gr_thread.h"

#ifdef __cplusplus
extern "C" {
#endif

        struct gr_conn_t;
typedef struct gr_conn_t                gr_conn_t;
        struct gr_tcp_conn_item_t;
typedef struct gr_tcp_conn_item_t       gr_tcp_conn_item_t;
        struct gr_tcp_req_t;
typedef struct gr_tcp_req_t             gr_tcp_req_t;
        struct gr_udp_req_t;
typedef struct gr_udp_req_t             gr_udp_req_t;
        struct gr_queue_item_t;
typedef struct gr_queue_item_t  gr_queue_item_t;

#if ! defined( WIN32 ) && ! defined( WIN64 )
    #ifdef GR_DEBUG_CONN
        #define GR_TCP_CONN_ITEM_LEN    128
    #else
        #define GR_TCP_CONN_ITEM_LEN    64
    #endif
    #define GR_TCP_REQ_LEN              64
    #define GR_UDP_REQ_LEN              64
#endif

#define gr_tcp_rsp_t    gr_tcp_req_t
#define gr_udp_rsp_t    gr_udp_req_t

#pragma pack( push, 4 )

typedef enum
{
    // 已经关闭
    GR_CLOSED       = 0,

    // 正在关闭流程中，tcp_in或tcp_out在发现GR_NEED_CLOSE状态后，
    // 将状态改为GR_CLOSING之后，开始关闭一个连接的流程。
    GR_CLOSING      = 1,

    // 需要关闭。这通常是gr_poll_t发现的错误，建议关连接。
    GR_NEED_CLOSE   = 2,

    // 连接是正常有效的。
    GR_OPENING      = 3

    // 用三位存状态，所以最大值是7
} gr_tcp_close_type_t;

// 关于该结构的细节，见gr_queue.h文件中的gr_queue_t和gr_queue_item_t
struct gr_queue_item_t
{
    // single link table
    gr_queue_item_t *   next;

    // process thread set true to indicate this item is processed,
    // processed item will be delete on next SwSrQueuePush or SwSrQueueDestroy called
    volatile bool               is_processed;

    // 至少有三个字节是可用的

    // 该字段标注了请求包的类型，对于TCP，该值必须为true
    bool                        is_tcp;

    // 引用计数。
    //char                        refs;

    // 是否请求，不是请求就是应答
    bool                        is_req;
};

#pragma pack( pop )

// TCP 请求
struct gr_tcp_req_t
{
    // 该成员必须在最前面
    // 本结构要么以回复的身份在gr_tcp_conn_item_t的rsp_list里，
    // 要么以请求的身份在work的队列里，所以一个就够了

    // 由于gr_queue_item_t有好大的空洞可以用来存东西，所以我们内部可以用这个。
    gr_queue_item_t             entry;

    // 当前数据包的检查上下文  
    gr_check_base_t                     check_ctxt;

#if defined( WIN32 ) || defined( WIN64 )
    // Windows收, IOCP相关
    OVERLAPPED                          iocp_overlapped;
    WSABUF                              iocp_wsabuf;
    union {
        DWORD                           iocp_recved;
        DWORD                           iocp_sent;
    };
    DWORD                               iocp_flags;
#endif

    // 在gr_tcp_conn_item_t 中的请求列表表项
    gr_tcp_req_t *                      req_list_next;

    // 请求或回复所属的连接对象指针。为什么需要这个指针呢？
    // 因为在gr_conn_item_t里并没有存储gr_req_t的指针列表。
    // 它只存了一个没收完的req指针，一但收全了就扔给工作线程去了。
    gr_tcp_conn_item_t *                parent;
    // 请求或回复数据包指针
    char *                              buf;
    // 请求或回复数据包最大长度限制，包括\0的空间，实际存放的数据最多是buf_max - 1字节，后面保证会有\0
    int                                 buf_max;
    // 请求或回复数据包实际数据长度
    int                                 buf_len;
    // 已经发送的字节数
    int                                 buf_sent;

    // 在非Windows的64位系统下, 以上60字节

} __attribute__ ((aligned (64)));

// UDP 请求
struct gr_udp_req_t
{
    // 该成员必须在最前面

    // 由于gr_queue_item_t有好大的空洞可以用来存东西，所以我们内部使用可以用这个。
    gr_queue_item_t             entry;

    // 当前数据包的检查上下文  
    gr_check_base_t                     check_ctxt;

#if defined( WIN32 ) || defined( WIN64 )
    // Windows收, IOCP相关
    OVERLAPPED                          iocp_overlapped;
    WSABUF                              iocp_wsabuf;
    DWORD                               iocp_recved;
    DWORD                               iocp_flags;
#endif

    // 请求或回复数据包指针
    char *                              buf;

    // 客户端地址
    union
    {
        struct sockaddr                 addr;
        struct sockaddr_in              addr_v4;
        struct sockaddr_in6             addr_v6; // 28字节
    };

    // 请求或回复数据包最大长度限制，包括\0的空间，实际存放的数据最多是buf_max - 1字节，后面保证会有\0
    uint16_t                            buf_max;

    // 请求或回复数据包实际数据长度
    uint16_t                            buf_len;

    // 在非Windows的64位系统下, 以上64字节

} __attribute__ ((aligned (64)));

struct gr_tcp_conn_item_t
{
    // 当前连接往工作线程中压入的请求数量。
    // 平时操作都是直接 ++，等请求数超过1500000000(最大2147483647)，用 gr_atomic_add 和 req_proc_count 一起减少防止溢出。
    // 应该 99.9999% 都不需要原子操作。检查并减少的工作在worker线程调用的 gr_tcp_conn_pop_top_req 函数中做。
    gr_atomic_t                         req_push_count;
    // 工作线程已经处理完的请求数量。
    // 平时操作都是直接 ++，等请求数超过1500000000(最大2147483647)，用 gr_atomic_add 和 req_push_count 一起减少防止溢出。
    // 应该 99.9999% 都不需要原子操作。检查并减少的工作在worker线程调用的 gr_tcp_conn_pop_top_req 函数中做。
    // 本字段值为0说明req_proc_count和req_push_count在维护，这时的数据是不准确的，不能读
    gr_atomic_t                         req_proc_count;

    // TCP回复单向列表表头。worker线程在压包前操作它，删除已经处理完的请求(是否压包后做删除会更好一点儿?)
    gr_tcp_rsp_t *                      rsp_list_head;
    // TCP回复单向列表表尾。worker线程通过该指针向队尾压包
    gr_tcp_rsp_t *                      rsp_list_tail;
    // TCP回复单向列表当前处理项。out线程操作它，从rsp_list_head向rsp_list_tail移动。
    gr_tcp_rsp_t *                      rsp_list_curr;

    // 当前正在接收数据过程中的TCP请求对象，一但数据包收完整了，
    // 则该req被扔给工作线程，同时再创建一个tcp_req用于接收后续数据包。
    gr_tcp_req_t *                      req;

    // 该连接的监听情况
    gr_port_item_t *                    port_item;

    // 给用户存一个指针的空间
    gr_conn_buddy_t                     buddy;

    // 当前连接的句柄
    int                                 fd;

    // 关闭类型。见 gr_tcp_close_type_t
    unsigned char                       close_type;

    // 是否已经检测出连接异常，此值为1，则不允许收发数据
    unsigned char                       is_network_error;

    // 是否正在 worker 的处理过程中
    unsigned char                       worker_locked;

    // 是否在监听接收数据。此值为false才不会收到数据
    unsigned char                       tcp_in_open     : 1;

    // 是否在允许发送状态中。允许发送状态，模块扔一个包，我就得发。必须要模块确认之后才能关闭发送
    unsigned char                       tcp_out_open    : 1;

    // 是否在允许处理状态中。
    unsigned char                       worker_open     : 1;

    unsigned char                       _reserve_5_bits  : 5;

    // 在64位系统上, 以上64字节

#ifdef GR_DEBUG_CONN
    // out 线程实际发出的回复包数量
    uint64_t                            rsp_send_count;
    uint64_t                            recv_bytes;
    uint64_t                            send_bytes;
    char                                reserved[ 30 ];
#endif

} __attribute__ ((aligned (64)));

#define QUEUE_ALL_DONE  ((void *)1)

int gr_conn_init();

void gr_conn_term();

gr_tcp_conn_item_t *
gr_tcp_conn_alloc(
    gr_port_item_t *    port_item,
    int                 fd
);

void gr_tcp_conn_free(
    gr_tcp_conn_item_t *    conn
);

void gr_tcp_conn_del_receiving_req(
    gr_tcp_conn_item_t *    conn
);

static_inline
bool gr_tcp_conn_has_pending_rsp(
    gr_tcp_conn_item_t *    conn
)
{
    return NULL != conn->rsp_list_head
        && QUEUE_ALL_DONE != conn->rsp_list_curr;
}

void gr_tcp_conn_clear_rsp_list(
    gr_tcp_conn_item_t *    conn
);

gr_tcp_req_t * gr_tcp_conn_prepare_recv(
    gr_tcp_conn_item_t *    conn
);

void gr_tcp_conn_add_rsp(
    gr_tcp_conn_item_t *    conn,
    gr_tcp_rsp_t *          rsp
);

static_inline
gr_tcp_rsp_t * gr_tcp_conn_top_rsp(
    gr_tcp_conn_item_t *    conn
)
{
    if ( NULL == conn->rsp_list_curr ) {
        conn->rsp_list_curr = conn->rsp_list_head;
    } else if ( QUEUE_ALL_DONE == conn->rsp_list_curr ) {
        return NULL;
    }

    return conn->rsp_list_curr;
}

int gr_tcp_conn_pop_top_rsp(
    gr_tcp_conn_item_t *    conn,
    gr_tcp_rsp_t *          confirm_rsp
);

static_inline
void gr_tcp_conn_add_req( gr_tcp_conn_item_t * conn )
{
    ++ (conn)->req_push_count;
    gr_debug( "add req_push_count %d", (conn)->req_push_count );
}

static_inline
void gr_tcp_conn_pop_tail_req( gr_tcp_conn_item_t * conn )
{
    -- (conn)->req_push_count;
    gr_debug( "dec req_push_count %d", (conn)->req_push_count );
}

void gr_tcp_conn_pop_top_req(
    gr_tcp_conn_item_t *    conn,
    bool                    tcp_out_disabled
);

// gr_tcp_req_free 和 gr_udp_req_free 的函数声明与本函数指针必须兼容
typedef void ( * gr_func_req_free_t )( void * req );

///////////////////////////////////////////////////////////////////////

gr_tcp_req_t * gr_tcp_req_alloc(
    gr_tcp_conn_item_t *    conn,
    int                     buf_max
);

void gr_tcp_req_free(
    gr_tcp_req_t *          req,
    bool                    is_recycle
);

/*int gr_tcp_req_add_refs(
    gr_tcp_req_t *          req
);

void gr_tcp_req_to_rsp(
    gr_tcp_req_t *          req
);*/

void gr_tcp_req_set_buf(
    gr_tcp_req_t *          req,
    void *                  buf,
    int                     buf_max,
    int                     buf_len
);

static_inline
int
gr_tcp_req_package_length(
    gr_tcp_req_t *          req
)
{
    if (   GR_PACKAGE_HTTP_REQ == req->check_ctxt.package_type
        || GR_PACKAGE_HTTP_REPLY == req->check_ctxt.package_type
    ) {
        // HTTP
        return (int)( req->check_ctxt.http_body_offset + req->check_ctxt.http_content_length );
    } else {
        return (int)req->check_ctxt.package_length;
    }
}

///////////////////////////////////////////////////////////////////////

gr_tcp_rsp_t * gr_tcp_rsp_alloc(
    gr_tcp_conn_item_t *    parent,
    int                     buf_max
);

void gr_tcp_rsp_free(
    gr_tcp_rsp_t *          rsp
);

void gr_tcp_rsp_set_buf(
    gr_tcp_rsp_t *  rsp,
    void *          buf,
    int             buf_max,
    int             buf_len,
    int             sent
);

///////////////////////////////////////////////////////////////////////

gr_udp_req_t * gr_udp_req_alloc(
    int                     buf_max
);

void gr_udp_req_free(
    gr_udp_req_t *          req
);

static_inline
int
gr_udp_req_package_length(
    gr_udp_req_t *          req
)
{
    if (   GR_PACKAGE_HTTP_REQ == req->check_ctxt.package_type
        || GR_PACKAGE_HTTP_REPLY == req->check_ctxt.package_type
    ) {
        // HTTP
        return (int)( req->check_ctxt.http_body_offset + req->check_ctxt.http_content_length );
    } else {
        return (int)req->check_ctxt.package_length;
    }
}

#ifdef __cplusplus
}
#endif

#endif // ! #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_CONN_H_
