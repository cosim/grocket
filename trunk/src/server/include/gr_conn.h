/**
 * @file inxlude/gr_conn.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   ������ش���
 * Revision History ���¼���
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-05    Created.
 **/
#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_CONN_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_CONN_H_

#include "gr_stdinc.h"
#include "grocket.h"
#include "gr_list.h"
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
    // �Ѿ��ر�
    GR_CLOSED       = 0,

    // ���ڹر������У�tcp_in��tcp_out�ڷ���GR_NEED_CLOSE״̬��
    // ��״̬��ΪGR_CLOSING֮�󣬿�ʼ�ر�һ�����ӵ����̡�
    GR_CLOSING      = 1,

    // ��Ҫ�رա���ͨ����gr_poll_t���ֵĴ��󣬽�������ӡ�
    GR_NEED_CLOSE   = 2,

    // ������������Ч�ġ�
    GR_OPENING      = 2

} gr_close_type_t;

// ���ڸýṹ��ϸ�ڣ���gr_queue.h�ļ��е�gr_queue_t��gr_queue_item_t
struct gr_queue_item_compact_t
{
    // single link table
    gr_queue_item_compact_t *   next;

    // process thread set true to indicate this item is processed,
    // processed item will be delete on next SwSrQueuePush or SwSrQueueDestroy called
    volatile bool               is_processed;

    // �����������ֽ��ǿ��õ�

    // ���ֶα�ע������������ͣ���ֵ����Ϊtrue
    bool                        is_tcp;

    // ���ü���
    char                        refs;

    // �Ƿ����󣬲����������Ӧ��
    bool                        is_req;
};

#pragma pack( pop )

// TCP ����
struct gr_tcp_req_t
{
    // �ó�Ա��������ǰ��
    // ���ṹҪô�Իظ��������gr_tcp_conn_item_t��rsp_list�
    // Ҫô������������work�Ķ��������һ���͹���
    union
    {
        // �����Ա�Ǹ�gr_queue_t�õ�
        gr_queue_item_t                 entry;
        // ����gr_queue_item_t�кô�Ŀն����������涫�������������ڲ������������
        gr_queue_item_compact_t         entry_compact;
    };

    // ��ǰ���ݰ��ļ��������  
    gr_check_ctxt_t                     check_ctxt;

#if defined( WIN32 ) || defined( WIN64 )
    // Windows��, IOCP���
    OVERLAPPED                          iocp_overlapped;
    WSABUF                              iocp_wsabuf;
    union {
        DWORD                           iocp_recved;
        DWORD                           iocp_sent;
    };
    DWORD                               iocp_flags;
#endif

    // �����б����
    gr_tcp_req_t *                      entry_req_list;

    // �����ظ����������Ӷ���ָ�롣Ϊʲô��Ҫ���ָ���أ�
    // ��Ϊ��gr_conn_item_t�ﲢû�д洢gr_req_t��ָ���б�
    // ��ֻ����һ��û�����reqָ�룬һ����ȫ�˾��Ӹ������߳�ȥ�ˡ�
    gr_tcp_conn_item_t *                parent;
    // �����ظ����ݰ�ָ��
    char *                              buf;
    // �����ظ����ݰ���󳤶����ƣ�����\0�Ŀռ䣬ʵ�ʴ�ŵ����������buf_max - 1�ֽڣ����汣֤����\0
    int                                 buf_max;
    // �����ظ����ݰ�ʵ�����ݳ���
    int                                 buf_len;
    // �Ѿ����͵��ֽ���
    int                                 buf_sent;

    // �ڷ�Windows��64λϵͳ��, ����60�ֽ�

} __attribute__ ((aligned (64)));

// UDP ����
struct gr_udp_req_t
{
    // �ó�Ա��������ǰ��
    union
    {
        // �����Ա�Ǹ�gr_queue_t�õ�
        gr_queue_item_t                 entry;
        // ����gr_queue_item_t�кô�Ŀն����������涫�������������ڲ�ʹ�ÿ����������
        gr_queue_item_compact_t         entry_compact;
    };

    // ��ǰ���ݰ��ļ��������  
    gr_check_ctxt_t                     check_ctxt;

#if defined( WIN32 ) || defined( WIN64 )
    // Windows��, IOCP���
    OVERLAPPED                          iocp_overlapped;
    WSABUF                              iocp_wsabuf;
    DWORD                               iocp_recved;
    DWORD                               iocp_flags;
#endif

    // �����ظ����ݰ�ָ��
    char *                              buf;

    // �ͻ��˵�ַ
    union
    {
        struct sockaddr                 addr;
        struct sockaddr_in              addr_v4;
        struct sockaddr_in6             addr_v6; // 28�ֽ�
    };

    // �����ظ����ݰ���󳤶����ƣ�����\0�Ŀռ䣬ʵ�ʴ�ŵ����������buf_max - 1�ֽڣ����汣֤����\0
    uint16_t                            buf_max;

    // �����ظ����ݰ�ʵ�����ݳ���
    uint16_t                            buf_len;

    // �ڷ�Windows��64λϵͳ��, ����64�ֽ�

} __attribute__ ((aligned (64)));


struct gr_tcp_conn_item_t
{
    // TCP�������б�
    gr_tcp_req_t *                      req_list_head;
    gr_tcp_req_t *                      req_list_tail;
    // TCP�ظ������б�
    gr_tcp_rsp_t *                      rsp_list_head;
    gr_tcp_rsp_t *                      rsp_list_tail;

    // ��ǰ���ڽ������ݹ����е�TCP�������һ�����ݰ��������ˣ�
    // ���req����req_list��ͬʱ���Ӹ������̡߳�
    // ͬʱ�ٴ���һ��tcp_req���ڽ��պ������ݰ���
    gr_tcp_req_t *                      req;

    // �����ӵļ������
    gr_port_item_t *                    port_item;

    // �����ӵľ��
    int                                 fd;

    // �ر����͡��� gr_close_type_t
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
