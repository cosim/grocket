/**
 * @file libgrocket/server_object.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/09
 * @version $Revision$ 
 * @brief   server framework buildin object
 * Revision History
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-09    Created.
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

#include "server_object.h"
#include "gr_ini.h"
#include "gr_global.h"
#include "gr_tools.h"
#include "gr_mem.h"
#include "gr_log.h"
#include <stdarg.h>

static_inline
bool check( gr_i_server_t * self, void * func1, void * func2 )
{
    // 一处安全检查，防止用户把错误的头文件放到了错误的二进制对象指针上
    return (void*)g_ghost_rocket_global.server_interface.library->classes[ CLASS_SERVER ] == (void*)self
        && func1 == func2;
}

static
void * memory_alloc(
    gr_i_server_t *     self,
    size_t              bytes )
{
    if ( ! check( self, (void*)self->memory_alloc, (void*)memory_alloc ) ) {
        // 参数栈可能是错的
        return NULL;
    }

    return gr_malloc( bytes );
}

static
void * memory_calloc(
    gr_i_server_t *     self,
    size_t              bytes )
{
    if ( ! check( self, (void*)self->memory_calloc, (void*)memory_calloc ) ) {
        // 参数栈可能是错的
        return NULL;
    }

    return gr_calloc( 1, bytes );
}

static
void memory_free(
    gr_i_server_t *     self,
    void *              p )
{
    if ( ! check( self, (void*)self->memory_free, (void*)memory_free ) ) {
        // 参数栈可能是错的
        return;
    }

    gr_free( p );
}

static
bool server_class_config_get_bool(
    gr_i_server_t * self,
    const char *    section,
    const char *    name,
    bool            default_value )
{
    gr_ini * ini;

    if ( ! check( self, (void*)self->config_get_bool, (void*)server_class_config_get_bool ) ) {
        // 不能使用default_value，因为参数栈可能是错的
        return false;
    }

    ini = (gr_ini *)g_ghost_rocket_global.config;
    return gr_ini_get_bool( ini, section, name, default_value );
}

static
int server_class_config_get_int(
    gr_i_server_t * self,
    const char *    section,
    const char *    name,
    int             default_value )
{
    gr_ini * ini;

    if ( ! check( self, (void*)self->config_get_int, (void*)server_class_config_get_int ) ) {
        // 不能使用default_value，因为参数栈可能是错的
        return -1;
    }

    ini = (gr_ini *)g_ghost_rocket_global.config;
    return gr_ini_get_int( ini, section, name, default_value );
}

static
const char * server_class_config_get_string(
    gr_i_server_t * self,
    const char *    section,
    const char *    name,
    const char *    default_value )
{
    gr_ini * ini;

    if ( ! check( self, (void*)self->config_get_string, (void*)server_class_config_get_string ) ) {
        // 不能使用default_value，因为参数栈可能是错的
        return NULL;
    }

    ini = (gr_ini *)g_ghost_rocket_global.config;
    return gr_ini_get_string( ini, section, name, default_value );
}

// 设置最大返回包长度
static
void * server_class_set_max_response(
    gr_i_server_t *     self,
    gr_proc_ctxt_t *    ctxt,
    size_t              bytes )
{
    if ( ctxt->pc_result_buf_max >= (int)bytes ) {
        assert( ctxt->pc_result_buf );
        ctxt->pc_result_buf_len = 0;
        return ctxt->pc_result_buf;
    } else {

        void * p;

        // align to K
        assert( g_ghost_rocket_global.rsp_buf_align > 0 );
        bytes = ALIGN_UP( bytes, g_ghost_rocket_global.rsp_buf_align );

        p = gr_malloc( bytes );
        if ( p ) {

            void * op = ctxt->pc_result_buf;

            ctxt->pc_result_buf_len = 0;
            ctxt->pc_result_buf = (char *)p;
            ctxt->pc_result_buf_max = (int)bytes;

            gr_free( op );
            return ctxt->pc_result_buf;
        } else {
            gr_error( "malloc %d bytes failed", (int)bytes );
            return NULL;
        }
    }
}

static
void server_class_log(
    gr_i_server_t * self,
    const char *    file,
    int             line,
    const char *    func,
    gr_log_level_t  level,
    const char *    fmt,
    ... )
{
    if ( g_ghost_rocket_global.server_interface.log_level <= level ) {
        char        buf[ 1024 ] = "";
        va_list     vl;

        if ( ! check( self, (void*)self->log, (void*)server_class_log ) ) {
            // 参数栈可能是错的
            return;
        }

        va_start( vl, fmt );
        vsnprintf( buf, sizeof( buf ), fmt, vl );
        gr_log_write( file, line, func, level, true, "%s", buf );
    }
}

static
void *
http_set_max_response(
    gr_i_server_t * self,
    gr_http_ctxt_t *http,
    size_t          bytes )
{
    if ( ! check( self, (void*)self->http_set_max_response, (void*)http_set_max_response ) ) {
        // 参数栈可能是错的
        return NULL;
    }

    return server_class_set_max_response( self, http->base, 300 + bytes );
}

static
const char *
http_get_req(
    gr_i_server_t *         self,
    gr_http_ctxt_t *        http,
    const char *            name )
{
    size_t i;

    if ( ! check( self, (void*)self->http_get_req, (void*)http_get_req ) ) {
        // 参数栈可能是错的
        return NULL;
    }

    if ( NULL == name || '\0' == * name )
        return NULL;

    assert( http );

    if ( http->params ) {
        for ( i = 0; i != http->params_count; ++ i ) {
            if ( 0 == stricmp( http->params[ i ].name, name ) )
                return http->params[ i ].value;
        }
    }

    if ( http->form ) {
        for ( i = 0; i != http->form_count; ++ i ) {
            if ( 0 == stricmp( http->form[ i ].name, name ) )
                return http->form[ i ].value;
        }
    }

    return NULL;
}

static
int
http_get_req_int(
    gr_i_server_t *     self,
    gr_http_ctxt_t *    http,
    const char *        name,
    int                 default_value )
{
    const char * s;
    
    if ( ! check( self, (void*)self->http_get_req_int, (void*)http_get_req_int ) ) {
        // 参数栈可能是错的
        return -1;
    }

    s = http_get_req( self, http, name );
    if ( NULL == s || * s <= 0 || ( '-' != * s && ! isdigit( * s ) ) )
        return default_value;
    return atoi( s );
}

static
int64_t
http_get_req_int64(
    gr_i_server_t *     self,
    gr_http_ctxt_t *    http,
    const char *        name,
    int64_t             default_value )
{
    const char * s;
    
    if ( ! check( self, (void*)self->http_get_req_int64, (void*)http_get_req_int64 ) ) {
        // 参数栈可能是错的
        return (int64_t)-1;
    }

    s = http_get_req( self, http, name );
    if ( NULL == s || * s <= 0 || ! isdigit( * s ) )
        return default_value;
#if defined( WIN32 ) || defined( WIN64 )
    return (int64_t)_atoi64( s );
#else
    return (int64_t)atoll( s );
#endif
}

static
bool
http_get_req_bool(
    gr_i_server_t *     self,
    gr_http_ctxt_t *    http,
    const char *        name,
    bool                default_value )
{
    const char * s;
    
    if ( ! check( self, (void*)self->http_get_req_bool, (void*)http_get_req_bool ) ) {
        // 参数栈可能是错的
        return false;
    }

    s = http_get_req( self, http, name );
    if ( NULL == s || * s <= 0 )
        return default_value;
    if ( str5cmp( s, 't', 'r', 'u', 'e', '\0' ) || str4cmp( s, 'y', 'e', 's', '\0' ) )
        return true;
    if ( str6cmp( s, 'f', 'a', 'l', 's', 'e', '\0' ) || str3cmp( s, 'n', 'o', '\0' ) )
        return false;
    if ( str2cmp( s, '1', '\0' ) )
        return true;
    if ( str2cmp( s, '0', '\0' ) )
        return false;
    return default_value;
}

static
const char *
http_get_header(
    gr_i_server_t *     self,
    gr_http_ctxt_t *    http,
    const char *        name )
{
    size_t i;

    if ( ! check( self, (void*)self->http_get_header, (void*)http_get_header ) ) {
        // 参数栈可能是错的
        return NULL;
    }

    if ( NULL == name || '\0' == * name )
        return NULL;

    if ( http->header ) {
        for ( i = 0; i != http->header_count; ++ i ) {
            if ( 0 == stricmp( http->header[ i ].name, name ) )
                return http->header[ i ].value;
        }
    }

    return NULL;
}

static
bool
http_append(
    gr_i_server_t *         self,
    gr_http_ctxt_t *        http,
    const void *            data,
    size_t                  len )
{
    if ( ! check( self, (void*)self->http_append, (void*)http_append ) ) {
        // 参数栈可能是错的
        return false;
    }

    if ( NULL == data )
        len = 0;

    if ( len > (size_t)(http->base->result_buf_max - http->base->result_buf_len) ) {
        // 太长了!
        http->is_error = true;
        return false;
    }

    memcpy( & http->base->result_buf[ http->base->result_buf_len ], data, len );
    http->base->result_buf_len += (int)len;
    return true;
}

static
bool
http_send_header(
    gr_i_server_t *     self,
    gr_http_ctxt_t *    http,
    size_t              content_length,
    const char *        content_type )
{
    const char * connection;
    int r;

    if ( ! check( self, (void*)self->http_send_header, (void*)http_send_header ) ) {
        // 参数栈可能是错的
        return false;
    }

    if ( ! http_set_max_response( self, http, content_length ) ) {
        http->is_error = true;
        return false;
    }

    http->base->result_buf_len = 0;

    if ( http->keep_alive )
        connection = "Keep-Alive";
    else
        connection = "close";

    r = snprintf(
        & http->base->result_buf[ http->base->result_buf_len ],
        http->base->result_buf_max - http->base->result_buf_len,
        "HTTP/1.1 200 OK\r\n"
        "Server: Apache\r\n"
        "Connection: %s\r\n"
        "Content-Length: %u\r\n"
        "Content-Type: %s\r\n\r\n",
        connection, (uint32_t)content_length, content_type
    );
    if ( r <= 0 ) {
        http->is_error = true;
        return false;
    }

    http->base->result_buf_len += r;
    return true;
}

static
bool
http_send_header2(
    gr_i_server_t *     self,
    gr_http_ctxt_t *    http,
    size_t              content_length,
    const char *        content_type,
    const char *        connection,
    const char *        status,
    const char *        additional_headers )
{
    int r;

    if ( ! check( self, (void*)self->http_send_header2, (void*)http_send_header2 ) ) {
        // 参数栈可能是错的
        return false;
    }

    if ( ! http_set_max_response( self, http, content_length ) ) {
        http->is_error = true;
        return false;
    }

    http->base->result_buf_len = 0;

    r = snprintf(
        & http->base->result_buf[ http->base->result_buf_len ],
        http->base->result_buf_max - http->base->result_buf_len,
        "HTTP/1.1 %s\r\n"
        "Connection: %s\r\n",
        status, connection
    );
    if ( r <= 0 ) {
        http->is_error = true;
        return false;
    }
    http->base->result_buf_len += r;

    if ( 0 != content_length ) {
        r = snprintf(
            & http->base->result_buf[ http->base->result_buf_len ],
            http->base->result_buf_max - http->base->result_buf_len,
            "Content-Length: %u\r\n",
            (uint32_t)content_length
        );
        if ( r <= 0 ) {
            http->is_error = true;
            return false;
        }
        http->base->result_buf_len += r;
    }

    if ( content_type && * content_type ) {
        r = snprintf(
            & http->base->result_buf[ http->base->result_buf_len ],
            http->base->result_buf_max - http->base->result_buf_len,
            "Content-Type: %s\r\n",
            content_type
        );
        if ( r <= 0 ) {
            http->is_error = true;
            return false;
        }
        http->base->result_buf_len += r;
    }

    if ( additional_headers && * additional_headers ) {
        r = snprintf(
            & http->base->result_buf[ http->base->result_buf_len ],
            http->base->result_buf_max - http->base->result_buf_len,
            "%s\r\n",
            additional_headers
        );
        if ( r <= 0 ) {
            http->is_error = true;
            return false;
        }
        http->base->result_buf_len += r;
    }

    return true;
}

static
bool
http_send(
    gr_i_server_t *         self,
    gr_http_ctxt_t *        http,
    const void *            data,
    size_t                  len,
    const char *            content_type )
{
    if ( ! check( self, (void*)self->http_send, (void*)http_send ) ) {
        // 参数栈可能是错的
        return false;
    }

    if ( NULL == data )
        len = 0;

    if ( ! http_send_header( self, http, len, content_type ) ) {
        return false;
    }

    if ( 'H' != http->request_type ) {
        // 支持head
        return http_append( self, http, data, len );
    }

    return true;
}

bool server_class_construct( server_class_t * sc )
{
    // 初始化功能函数
    sc->face.memory_alloc           = memory_alloc;
    sc->face.memory_calloc          = memory_calloc;
    sc->face.memory_free            = memory_free;
    sc->face.config_get_bool        = server_class_config_get_bool;
    sc->face.config_get_int         = server_class_config_get_int;
    sc->face.config_get_string      = server_class_config_get_string;
    sc->face.set_max_response       = server_class_set_max_response;
    sc->face.log                    = server_class_log;
    sc->face.http_set_max_response  = http_set_max_response;
    sc->face.http_get_req           = http_get_req;
    sc->face.http_get_req_int       = http_get_req_int;
    sc->face.http_get_req_int64     = http_get_req_int64;
    sc->face.http_get_req_bool      = http_get_req_bool;
    sc->face.http_get_header        = http_get_header;
    sc->face.http_append            = http_append;
    sc->face.http_send              = http_send;
    sc->face.http_send_header       = http_send_header;
    sc->face.http_send_header2      = http_send_header2;
    return true;
}
