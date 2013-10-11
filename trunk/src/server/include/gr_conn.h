/**
 * @file inxlude/gr_conn.h
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
#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_CONN_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_CONN_H_

#include "gr_stdinc.h"
#include "grocket.h"
#include "gr_queue.h"

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
struct gr_queue_item_compact_t;
typedef struct gr_queue_item_compact_t  gr_queue_item_compact_t;

#if ! defined( WIN32 ) && ! defined( WIN64 )
#define GR_TCP_CONN_ITEM_LEN            64
#define GR_TCP_REQ_LEN                  64
#define GR_UDP_REQ_LEN                  64
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
    GR_OPENING      = 2

} gr_close_type_t;

// 关于该结构的细节，见gr_queue.h文件中的gr_queue_t和gr_queue_item_t
struct gr_queue_item_compact_t
{
    // single link table
    gr_queue_item_compact_t *   next;

    // process thread set true to indicate this item is processed,
    // processed item will be delete on next SwSrQueuePush or SwSrQueueDestroy called
    volatile bool               is_processed;

    // 至少有三个字节是可用的

    // 该字段标注了请求包的类型，该值必须为true
    bool                        is_tcp;

    // 引用计数
    char                        refs;

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
    union
    {
        // 这个成员是给gr_queue_t用的
        gr_queue_item_t                 entry;
        // 由于gr_queue_item_t有好大的空洞可以用来存东西，所以我们内部可以用这个。
        gr_queue_item_compact_t         entry_compact;
    };

    // 当前数据包的检查上下文  
    gr_check_ctxt_t                     check_ctxt;

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

    // 请求列表表项
    gr_tcp_req_t *                      entry_req_list;

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
    union
    {
        // 这个成员是给gr_queue_t用的
        gr_queue_item_t                 entry;
        // 由于gr_queue_item_t有好大的空洞可以用来存东西，所以我们内部使用可以用这个。
        gr_queue_item_compact_t         entry_compact;
    };

    // 当前数据包的检查上下文  
    gr_check_ctxt_t                     check_ctxt;

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
    // TCP请求单向列表
    gr_tcp_req_t *                      req_list_head;
    gr_tcp_req_t *                      req_list_tail;
    // TCP回复单向列表
    gr_tcp_rsp_t *                      rsp_list_head;
    gr_tcp_rsp_t *                      rsp_list_tail;

    // 当前正在接收数据过程中的TCP请求对象，一但数据包收完整了，
    // 则该req放入req_list，同时被扔给工作线程。
    // 同时再创建一个tcp_req用于接收后续数据包。
    gr_tcp_req_t *                      req;

    // 该连接的监听情况
    gr_port_item_t *                    port_item;

    // 该连接的句柄
    int                                 fd;

    // 关闭类型。见 gr_close_type_t
    gr_close_type_t                     close_type;

} __attribute__ ((aligned (64)));

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

gr_tcp_req_t * gr_tcp_conn_prepare_recv(
    gr_tcp_conn_item_t *    conn
);

void gr_tcp_conn_add_rsp(
    gr_tcp_conn_item_t *    conn,
    gr_tcp_rsp_t *          rsp
);

int gr_tcp_conn_pop_top_rsp(
    gr_tcp_conn_item_t *    conn,
    gr_tcp_rsp_t *          confirm_rsp
);

///////////////////////////////////////////////////////////////////////

gr_tcp_req_t * gr_tcp_req_alloc(
    gr_tcp_conn_item_t *    parent,
    int                     buf_max
);

int gr_tcp_req_free(
    gr_tcp_req_t *          req
);

int gr_tcp_req_add_refs(
    gr_tcp_req_t *          req
);

void gr_tcp_req_to_rsp(
    gr_tcp_req_t *          req
);

void gr_tcp_req_set_buf(
    gr_tcp_req_t *          req,
    void *                  buf,
    int                     buf_max,
    int                     buf_len
);

static inline
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

#define gr_tcp_rsp_free     gr_tcp_req_free
#define gr_tcp_rsp_alloc    gr_tcp_req_alloc
#define gr_tcp_rsp_add_refs gr_tcp_req_add_refs

///////////////////////////////////////////////////////////////////////

gr_udp_req_t * gr_udp_req_alloc(
    int                     buf_max
);

int gr_udp_req_free(
    gr_udp_req_t *          req
);

static inline
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
