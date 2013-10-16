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
//#include "gr_queue.h"

int gr_tcp_close_init()
{
    return GR_OK;
}

void gr_tcp_close_term()
{
}

void gr_tcp_close_from_in( gr_tcp_conn_item_t * conn )
{
    // 如果还有未收完的请求，删了
    if ( NULL != conn->req ) {
        gr_tcp_req_t * req = conn->req;
        conn->req = NULL;
        gr_tcp_req_free( req );
    }

    // 如果没打需要关闭的标记，打上
    if ( conn->close_type > GR_NEED_CLOSE ) {
        conn->close_type = GR_NEED_CLOSE;
    }

    // 该标记表示连接已经不在数据收线程里
    conn->tcp_in_open = false;
}

void gr_tcp_close_from_out( gr_tcp_conn_item_t * conn )
{
    // 如果没打需要关闭的标记，打上
    if ( conn->close_type > GR_NEED_CLOSE ) {
        conn->close_type = GR_NEED_CLOSE;
    }

    // 该标记表示连接已经不在数据发送线程里
    conn->tcp_out_open = false;
}

void gr_tcp_close_from_work( gr_tcp_conn_item_t * conn )
{
    assert( conn->close_type <= GR_NEED_CLOSE );

    // 最后再增加请求弹包数量
    ++ conn->req_pop_count;

    // 该标记表示连接已经不在工作线程里
    conn->worker_open = false;

    // 现在可能到了删除连接的时间了
}
