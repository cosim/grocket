/**
 * @file libgrocket/gr_tcp_close.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/13
 * @version $Revision$ 
 * @brief   TCP Close module
 * Revision History
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-13    Created.
 **/
/* 
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

#include "gr_tcp_close.h"
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
#include "gr_tcp_in.h"
#include "gr_tcp_out.h"
#include "gr_worker.h"

int gr_tcp_close_init()
{
    return GR_OK;
}

void gr_tcp_close_term()
{
}

/*
问: 如何判断一个TCP连接是否在in线程里?
答: 只要accept成功一个连接并将其加入in线程, 则 tcp_in_open 值就已经为 true 了. 该值不会轻易变成 false.
    除非遇到连接异常, 或者用户在检查数据包过程中决定断掉连接

问: 如何判断一个TCP连接是否在out线程里?
答: gr_tcp_conn_item_t 的 rsp_list_head 指针为空说明已经没有需要 tcp_out 发送的数据了

问: 如何判断一个TCP连接是否在worker线程/进程里?
答: gr_tcp_conn_item_t 的 req_push_count 与 req_proc_count 相等, 同时 req_proc_count 值不为 0
    说明已经没有待 worker 处理请的请求了

问: 会不会产生 tcp_in, tcp_out, worker 一起删除连接的情况？
    举例:
    1) tcp_in 收到了断连接消息, 于是删除了 gr_tcp_conn_item_t 的 req 成员,
       将 tcp_in_open 状态改为 false
    2) tcp_out 发完了最后一个回复包后, 发现 gr_tcp_conn_item_t 的 rsp_list_head 为空指针,
       将 tcp_out_open 状态改为 false
    3) worker 处理完了该连接的最后一个请求, 发现该请求处理后无返回包, 
       递增 gr_tcp_conn_item_t 的 req_proc_count 值, 发现与 req_push_count 值相同,
       将 worker_open 状态改为 false

    1.1) 或者再简单的一个场景: 在运行 test_tcp_client 时, 直接把 test_tcp_client kill 掉,
       这时tcp_in和tcp_out几乎同时遇到连接错误.

    以上三步都做完以后, 三个线程同时判断 tcp_in_open, tcp_out_open, worker_open 状态.
    发现满足条件, 于是同时删除当前TCP连接, 怎么办?

答: 
        由于没有锁, 所以一定要保证连接肯定被删除，不能所有线程都觉得似乎别人会删，然后自己没删除，
    最后没人删。肯定是谁删, 谁就要等到最后, 这样等其它人都退出后才能安全的删除，但要是真“等”起
    来性能又没法接受。先考虑以下几个事实：
    1、在 tcp_in 发现连接断的时候，worker 肯定没断，因为随时会有请求包要求 worker 处理。
    2、worker 没把连接释放掉，tcp_out 肯定不能断，因为随时会有回复包要发。
    3、所以释放的顺序必须是：tcp_in, worker, tcp_out，连接应该是由 tcp_out 真正删除。
    4、服务器的 tcp_out 线程一般 CPU 是有空闲能力的，因为客户端接收能力有限。

    办法1: 所有删除TCP连接的动作发给accept线程去做, 由accept线程判断TCP连接是否已经被删除
           这个办法需要判断TCP连接是否已经被删除有点儿恶心
    办法2: 代码中使用的方法，in 发现连接断了，通知 out，只需要注册一个写事件就可以了，由out决定何时删。
           这里细节不少，见代码。
*/

static_inline
bool is_out_busy( gr_tcp_conn_item_t * conn )
{
    if ( NULL == conn->rsp_list_head ) {
        // 没有待发送的包
        return false;
    }

    if ( QUEUE_ALL_DONE == conn->rsp_list_curr ) {
        // 这说明所有包都已经发送完毕
        return false;
    }

    // 在忙
    return true;
}

static_inline
bool is_worker_busy( gr_tcp_conn_item_t * conn )
{
    //TODO: 其实 worker_locked 是一个冗余字段，正在查错，为了排错加的
    if ( conn->req_push_count > conn->req_proc_count || conn->worker_locked ) {
        // 这说明 Worker 线程还有工作
        gr_debug( "conn->req_push_count = %d, conn->req_proc_count = %d, conn->worker_locked = %d, ignore",
            (int)conn->req_push_count, (int)conn->req_proc_count, (int)conn->worker_locked
);
        return true;
    }

    return false;
}

void gr_tcp_close_from_in(
    gr_tcp_conn_item_t *    conn,
    bool                    tcp_out_disabled )
{
    // 在 tcp_in 模块中发现连接异常, 会将连接的 close_type 字段设为 GR_NEED_CLOSE
    // 在 tcp_in 模块在接收数据过程中通过调用用户模块来判断数据包完整与否, 如果协议错, 则不会设 GR_NEED_CLOSE
    // 以上两种错误都会导致 gr_tcp_close_from_in 函数被调用

    // 如果没打需要关闭的标记，打上
    if ( conn->close_type > GR_NEED_CLOSE ) {
        // 如果在这儿设置 GR_NEED_CLOSE 标记, 则是用户模块解析数据包协议错要求断连接
        conn->close_type = GR_NEED_CLOSE;
    } else {
        // 进入这个分支说明在收数据的过程中, poll 模块已经发现连接异常, 必须断连接
    }

    // 如果还有未收完的请求，删了
    gr_tcp_conn_del_receiving_req( conn );

    // 如果 worker 已经没数据了，则设一下状态
    //TODO: 这个虽然和 worker 有冲突的可能，但这个冲突还是比较小的，因为 gr_tcp_close_from_worker
    // 已经不设 conn->worker_open 字段了
    if ( conn->worker_open && ! is_worker_busy( conn ) ) {
        conn->worker_open = false;
    }

    // 该标记表示连接已经不在数据收线程里
    if ( conn->tcp_in_open ) {
        conn->tcp_in_open = false;
    }

    if ( tcp_out_disabled ) {
        gr_tcp_close_from_out( conn, true );
    } else {
        // 把连接在自己这儿已经死亡的喜讯告诉tcp out
        gr_tcp_out_notify_close( conn );
    }
}

void gr_tcp_close_from_worker(
    gr_tcp_conn_item_t *    conn,
    bool                    tcp_out_disabled )
{
    assert( conn->close_type <= GR_NEED_CLOSE );

    if ( tcp_out_disabled ) {
        gr_tcp_close_from_out( conn, true );
    } else {
        // 把连接在自己这儿已经死亡的喜讯告诉tcp out
        gr_tcp_out_notify_close( conn );
    }
}

void gr_tcp_close_from_out(
    gr_tcp_conn_item_t *    conn,
    bool                    tcp_out_disabled )
{
    // 想关闭连接时才会进来
    assert( GR_NEED_CLOSE == conn->close_type );

    // 未发完的数据包该删的都应该已经删了，但对于禁用 tcp_out 线程的情况，可能没删
    if ( NULL != conn->rsp_list_head ) {
        gr_tcp_conn_clear_rsp_list( conn );
    }

    // 如果没从poll中删除，则删一下
    gr_tcp_out_del_tcp_conn( conn );

    //TODO: 这个不加的话，耗时请求会不会有问题？但加了肯定会有冲突的可能
    // 如果 worker 已经没数据了，则设一下状态
    if ( conn->worker_open && ! is_worker_busy( conn ) ) {
        conn->worker_open = false;
    }

    // 该标记表示连接已经不在数据发送线程里
    if ( conn->tcp_out_open ) {
        conn->tcp_out_open = false;
    }

    if ( conn->tcp_in_open || conn->worker_open ) {
        gr_debug( "[tcp.output]in_open = %d, out_open = %d, worker_open = %d, ignore",
            (int)conn->tcp_in_open, (int)conn->tcp_out_open, (int)conn->worker_open );
        return;
    }

    // 真正的删。同一个连接必须要保证只进来一次，不然崩溃。
    gr_tcp_conn_free( conn );
}
