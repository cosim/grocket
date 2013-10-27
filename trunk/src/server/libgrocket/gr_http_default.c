/**
 * @file libgrocket/gr_http_default.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/23
 * @version $Revision$ 
 * @brief   http default command
 * Revision History
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-23    Created.
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
#include "gr_http.h"
#include "gr_log.h"
#include "gr_global.h"
#include "gr_tools.h"

void http_hello( gr_http_ctxt_t * http, gr_conn_buddy_t * conn_buddy, int * processed_len );


// 现在这个函数有这么个问题：返回包的HTTP包头如何预留空间？
static_inline
bool
binary_over_http(
    gr_http_ctxt_t *    http,
    int *               processed_len
)
{
    return false;

    /*
    assert( http->object );

    if (   This->proc_package
        && ( '/' == http->directory[0] && '\0' == http->directory[1] )
        && '\0' == * http->object
        && ( http->body && http->body_len )
    ) {
        struct GsCheckPackageCtxt check_ctxt;
        struct GsProcPackageCtxt proc_ctxt;
        bool ierr = false;
        bool ifull = false;

        // 访问网站的 / 同时有二进制POST数据
        // 检查是不是用HTTP走二进制数据通道

        // 检查包类型
        if ( This->check_package ) {

            check_ctxt.is_tcp = http->check_ctxt->is_tcp;

            This->check_package( http->body, http->body_len, & check_ctxt, & ierr, & ifull );
            
            if ( ierr || ! ifull ) {
                return false;
            }

            if (   GS_PACKAGE_ERROR == check_ctxt.package_type
                || GS_PACKAGE_HTTP_REQ == check_ctxt.package_type
                || GS_PACKAGE_HTTP_REPLY == check_ctxt.package_type )
            {
                // 不允许用HTTP包含HTTP协议
                return false;
            }

            if ( http->body_len != (size_t)check_ctxt.package_length ) {
                return false;
            }

        } else {
            memset( & check_ctxt, 0, sizeof( struct GsCheckPackageCtxt ) );
            check_ctxt.is_tcp = http->check_ctxt->is_tcp;
            check_ctxt.package_type = GS_PACKAGE_PRIVATE;
        }

        // process
        proc_ctxt.check_ctxt = & check_ctxt;
	    proc_ctxt.port = http->port;
        proc_ctxt.sock = http->sock;
        proc_ctxt.thread_id = http->thread_id;

        proc_ctxt.result_buf = http->result_buf;
        int                         result_buf_max;
        int *                       result_buf_len;

        int                         processed_len;

        This->proc_package( http->body, (int)http->body_len, proc_ctxt );

        if ( 0 == processed_len ) {
            http->keep_alive = FALSE;
        } else if ( processed_len < 0 ) {
            * http->is_error = TRUE;
        }

        return TRUE;

    } // 检查是不是用HTTP走二进制数据通道

    return FALSE;
    */
}

typedef void
( * func_http )(
    gr_http_ctxt_t *    http,
    gr_conn_buddy_t *   conn_buddy,
    int *               processed_len
);

struct http_item
{
    const char *    directory;

    const char *    object;

    func_http       func;
};

static struct http_item g_items[] =
{
    { "/system",        "hello",        http_hello },
};

void
http_execute_inner(
    gr_http_ctxt_t *    http,
    gr_conn_buddy_t *   conn_buddy,
    int *               processed_len,
    bool *              is_processed
)
{
    size_t              i;
    struct http_item *  p;

    if ( binary_over_http( http, processed_len ) ) {
        * is_processed = true;
        return;
    }

    for ( i = 0; i != COUNT_OF( g_items ); ++ i ) {
        p = & g_items[ i ];
        if (   0 == strcmp( p->directory, http->directory )
            && 0 == strcmp( p->object, http->object )
        ) {
            * is_processed = true;
            p->func( http, conn_buddy, processed_len );
            return;
        }
    }
}

void http_hello( gr_http_ctxt_t * http, gr_conn_buddy_t * conn_buddy, int * processed_len )
{
    const char buf[] = "hello2";
    http->keep_alive = true;

    g_buildin->http_send( g_buildin, http, buf, sizeof( buf ) - 1, "text/plain" );
}
