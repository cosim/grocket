/**
 * @file libgrocket/tcp_io.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/07
 * @version $Revision$ 
 * @brief   
 * Revision History ���¼���
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-07    Created.
 **/
#ifndef _tcp_io_h_
#define _tcp_io_h_

// ����Windows���ñ�Ӧ�����蹫�������ݽṹ�ʹ��뱻������FUCK��
// windows������ͬһ��socketͬʱ�ӵ�����iocp������ұ���
// �� tcp_in �� tcp_out ��Windows�ºϲ�������ƽ̨����Ӱ�졣
// �շ�������뻹������һ�ݡ�������������ļ�

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
#include "gr_worker.h"

///////////////////////////////////////////////////////////////////////
//
// Ϊ�����״����Ұ�gr_tcp_in_t ��gr_tcp_out_t ����ͳһ����һ��
//

typedef struct
{
    gr_threads_t    threads;

    gr_poll_t *     poll;

    int             concurrent;

} gr_tcp_io_t;

typedef gr_tcp_io_t gr_tcp_in_t;
typedef gr_tcp_io_t gr_tcp_out_t;

#if defined( WIN32 ) || defined( WIN64 )

void tcp_io_windows( gr_thread_t * thread );

#endif // #if defined( WIN32 ) || defined( WIN64 )

///////////////////////////////////////////////////////////////////////
//
// �ⱾӦ����tcp_out��˽�д���
//

static inline
void on_tcp_send_error(
    gr_tcp_in_t *           self,
    gr_thread_t *           thread,
    gr_tcp_conn_item_t *    conn
)
{
}

static inline
void on_tcp_send(
    gr_tcp_out_t *          self,
    gr_thread_t *           thread,
    gr_tcp_conn_item_t *    conn
)
{
    int             r;

    // �����ݣ������Ѿ���ѭ������
    r = gr_poll_send( self->poll, thread, conn );
    if ( r < 0 ) {
        // Ҫô�������Ҫô�Է���������
        gr_error( "gr_poll_send return error %d", r );
        on_tcp_send_error( self, thread, conn );
        return;
    }

    gr_info( "on_tcp_send called" );
}

///////////////////////////////////////////////////////////////////////
//
// �ⱾӦ����tcp_in��˽�д���
//
static inline
void on_tcp_recv_error(
    gr_tcp_in_t *           self,
    gr_thread_t *           thread,
    gr_tcp_conn_item_t *    conn
)
{
    int r;
    
    // �Ȱѵ�ǰ�����ڵ�ǰpoll��ͣ��
    r = gr_poll_recv_done( self->poll, thread, conn, false );
    if ( 0 != r ) {
        gr_error( "gr_poll_recv_done return error %d", r );
    }

    // Ȼ����accept�̴߳�������ӵĶ�������Ϊ������������ġ�
}

static inline
void on_tcp_not_full(
    gr_tcp_in_t *           self,
    gr_thread_t *           thread,
    gr_tcp_conn_item_t *    conn
)
{
    int r;
    
    r = gr_poll_recv_done( self->poll, thread, conn, true );
    if ( 0 != r ) {
        gr_error( "gr_poll_recv_done return error %d", r );
        on_tcp_recv_error( self, thread, conn );
    }
}

static inline
void on_tcp_full(
    gr_tcp_in_t *           self,
    gr_thread_t *           thread,
    gr_tcp_conn_item_t *    conn,
    gr_tcp_req_t *          req
)
{
    // ����ǰreq�ӵ������߳���ȥ
    // ��ǰ���ӵİ�������
    int     r;
 
    r = gr_worker_add_tcp( req, false );
    if ( 0 != r ) {
        gr_fatal( "gr_worker_add_tcp return error %d", r );
        on_tcp_recv_error( self, thread, conn );
        return;
    }

    // �����ˣ��º�֪ͨ
    r = gr_poll_recv_done( self->poll, thread, conn, true );
    if ( 0 != r ) {
        gr_error( "gr_poll_recv_done return error %d", r );
        on_tcp_recv_error( self, thread, conn );
        return;
    }
}

static inline
void on_tcp_recv(
    gr_tcp_in_t *           self,
    gr_thread_t *           thread,
    gr_tcp_conn_item_t *    conn
)
{
    int             r;
    gr_tcp_req_t *  req = NULL;
    bool            is_error = false;
    bool            is_full = false;

    // �����ݣ������Ѿ���ѭ����һֱ�յ�û������
    r = gr_poll_recv( self->poll, thread, conn, & req );
    if ( r <= 0 ) {
        // Ҫô�������Ҫô�Է���������
        if ( 0 == r ) {
            gr_info( "user disconnect connection" );
        } else {
            gr_error( "gr_poll_recv return error %d", r );
        }
        on_tcp_recv_error( self, thread, conn );
        return;
    }

    // �����յ���r�ֽ����ݣ��ж����ݰ��Ƿ�����
    gr_module_check_tcp( req, & is_error, & is_full );
    if ( is_error ) {
        // Э���
        gr_error( "gr_module_check_tcp is_error is true" );
        on_tcp_recv_error( self, thread, conn );
        return;
    }
    if ( ! is_full ) {
        on_tcp_not_full( self, thread, conn );
        return;
    }

    // �յ���һ���������ݰ�
    on_tcp_full( self, thread, conn, req );
}

#endif // #ifndef _tcp_io_h_
