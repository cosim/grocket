/**
 * @file libgrocket/gr_http.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/23
 * @version $Revision$ 
 * @brief   HTTP protocol
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
#include "gr_mem.h"
#include "gr_config.h"
#include "gr_errno.h"
#include "gr_module.h"

#define IS_PARSE_HTTP_POST_DATA     1

void
http_execute_inner(
    gr_http_ctxt_t *    http,
    gr_conn_buddy_t *   conn_buddy,
    int *               processed_len,
    bool *              is_processed
);

        struct parse_buf_t;
typedef struct parse_buf_t  parse_buf_t;

// per thread struct
struct parse_buf_t
{
    gr_http_param_t *   header;
    int                 header_max;

    gr_http_param_t *   query;
    int                 query_max;

    gr_http_param_t *   form;
    int                 form_max;

}  __attribute__ ((aligned (64)));

typedef struct
{
    int             parse_buf_count;

    parse_buf_t     parse_bufs[ 1 ];

} gr_http_t;

int gr_http_init()
{
    gr_http_t * p;
    int         r;
    int         i;
    int         req;
    int         worker_thread_count = gr_config_worker_thread_count();

    if ( NULL != g_ghost_rocket_global.http ) {
        gr_fatal( "[init]gr_http_init already called" );
        return GR_ERR_WRONG_CALL_ORDER;
    }

    if ( worker_thread_count < 1 ) {
        gr_fatal( "[init]worker_thread_count %d invalid", worker_thread_count );
        return GR_ERR_INVALID_PARAMS;
    }

    req = (int)( sizeof( gr_http_t ) + sizeof( parse_buf_t ) * ( worker_thread_count - 1 ) );
    p = (gr_http_t *)gr_calloc( 1, req );
    if ( NULL == p ) {
        gr_fatal( "[init]malloc %d bytes failed, errno=%d,%s",
            req, errno, strerror( errno ) );
        return GR_ERR_BAD_ALLOC;
    }

    r = GR_ERR_UNKNOWN;

    do {

        p->parse_buf_count      = worker_thread_count;

        for ( i = 0; i != p->parse_buf_count; ++ i ) {

            parse_buf_t * buf = & p->parse_bufs[ i ];

            buf->header_max = 100;
            buf->query_max = 100;
            buf->form_max = 100;

            buf->header = gr_calloc( 1, sizeof( gr_http_param_t ) * buf->header_max );
            if ( NULL == buf->header ) {
                r = GR_ERR_INVALID_PARAMS;
                break;
            }

            buf->query = gr_calloc( 1, sizeof( gr_http_param_t ) * buf->query_max );
            if ( NULL == buf->query ) {
                r = GR_ERR_INVALID_PARAMS;
                break;
            }

            buf->form = gr_calloc( 1, sizeof( gr_http_param_t ) * buf->form_max );
            if ( NULL == buf->form ) {
                r = GR_ERR_INVALID_PARAMS;
                break;
            }
        }

        r = GR_OK;
    } while ( false );

    if ( GR_OK != r ) {
        for ( i = 0; i != p->parse_buf_count; ++ i ) {
            parse_buf_t * buf = & p->parse_bufs[ i ];
            // free对空指针不做处理，所以这个代码是安全的
            gr_free( buf->header );
            gr_free( buf->query );
            gr_free( buf->form );
        }

        gr_free( p );
        return r;
    }

    g_ghost_rocket_global.http = p;
    return GR_OK;
}

void gr_http_term()
{
    gr_http_t * p = (gr_http_t *)g_ghost_rocket_global.http;

    if ( NULL != p ) {

        int i;

        for ( i = 0; i != p->parse_buf_count; ++ i ) {
            parse_buf_t * buf = & p->parse_bufs[ i ];
            // free对空指针不做处理，所以这个代码是安全的
            gr_free( buf->header );
            gr_free( buf->query );
            gr_free( buf->form );
        }

        gr_free( p );
        g_ghost_rocket_global.http = NULL;
    }
}

static_inline
int
check_http_packet_type(
    const unsigned char * buf, int len
)
{
    if ( len <= 0 ) {
        return GR_PACKAGE_ERROR;
    }

    switch( len )
    {
    case 1:
        // http
        if ( buf[ 0 ] == 'G' || buf[ 0 ] == 'P' || buf[ 0 ] == 'H' || buf[ 0 ] == 'C' ) {
            return GR_PACKAGE_HTTP_REQ;
        }
        break;
    case 2:
        // http reply
        if ( str2cmp( buf, 'H', 'T' ) ) {
            return GR_PACKAGE_HTTP_REPLY;
        }
        // http
        if (   str2cmp( buf, 'G', 'E' )
            || str2cmp( buf, 'P', 'O' )
            || str2cmp( buf, 'H', 'E' )
            || str2cmp( buf, 'C', 'O' )
        ) {
            return GR_PACKAGE_HTTP_REQ;
        }
        break;
    case 3:
        // http reply
        if ( str3cmp( buf, 'H', 'T', 'T' ) ) {
            return GR_PACKAGE_HTTP_REPLY;
        }
        // http
        if (   str3cmp( buf, 'G', 'E', 'T' )
            || str3cmp( buf, 'P', 'O', 'S' )
            || str3cmp( buf, 'H', 'E', 'A' )
            || str3cmp( buf, 'C', 'O', 'N' )
        ) {
            return GR_PACKAGE_HTTP_REQ;
        }
        break;
    case 4:
        // http reply
        if ( str4cmp( buf, 'H', 'T', 'T', 'P' ) ) {
            return GR_PACKAGE_HTTP_REPLY;
        }
        // http request
        if (   str4cmp( buf, 'G', 'E', 'T', ' ' )
            || str4cmp( buf, 'P', 'O', 'S', 'T' )
            || str4cmp( buf, 'H', 'E', 'A', 'D' )
            || str4cmp( buf, 'C', 'O', 'N', 'N' )
        ) {
            return GR_PACKAGE_HTTP_REQ;
        }
        break;
    default:
        // http reply
        if ( str5cmp( buf, 'H', 'T', 'T', 'P', '/' ) ) {
            return GR_PACKAGE_HTTP_REPLY;
        }
        // http
        if (
               str4cmp( buf, 'G', 'E', 'T', ' ' )
            || str5cmp( buf, 'P', 'O', 'S', 'T', ' ' )
            || str5cmp( buf, 'H', 'E', 'A', 'D', ' ' )
            || str5cmp( buf, 'C', 'O', 'N', 'N', 'E' )
        ) {
            return GR_PACKAGE_HTTP_REQ;
        }
        break;
    }
    return GR_PACKAGE_ERROR;
}

static_inline
bool
parser_is_space(
    char c
)
{
    return c > 0 && isspace( c );
}

static_inline
size_t
parser_ignore_spaces(
    const char * _beg,
    const char * _end
)
{
    const char * cur;

    if ( NULL == _beg || NULL == _end || _beg == _end ) {
        return 0;
    }

    for ( cur = _beg; cur != _end; ++ cur ) {
        if ( ! parser_is_space( * cur ) ) {
            break;
        }
    }

    return cur - _beg;
}

static_inline
bool
is_http_full_packet(
    const char * buf,
    int len,
    bool is_http_reply,
    bool * is_error,
    int * header_offset,
    int * body_offset,
    int * content_length
)
{
    char * p;
    char * t;
    char * header = NULL;
    char * body = NULL;
    char c;

    assert( buf && is_error && header_offset && body_offset && content_length );

    * content_length = (int)-1;
    * header_offset = 0;
    * body_offset = 0;
    * is_error = false;

    if ( NULL == buf || len <= 0 ) {
        * is_error = true;
        return false;
    }

    // receive module must add \0
    assert( buf[ len ] == '\0' );

    // first, find status line
    p = (char *)memchr( buf, '\n', len );
    if ( NULL == p ) {
        // status line not ready
        return false;
    }
    assert( p > buf );

    header = p + 1;

    // find first empty line
    t = header;
    while( 1 ) {
        p = (char*)memchr( t, '\n', len - ( t - buf ) );
        if ( NULL == p ) {
            // headers not full
            return false;
        }
        if ( p == buf + len - 1 ) {
            // \n is last charactor
            if ( p == header || ( p == header + 1 && * (p - 1) == '\r' ) ) {
                // status line follow empty line, this is full HTTP 0.9 package
                body = p + 1;
                break;
            }
            return false;
        }

        if ( p == header || ( p == header + 1 && * (p - 1) == '\r' ) ) {
            // status line follow empty line
            body = p + 1;
            break;
        }

        if ( '\r' == * ( p + 1 ) ) {
            body = p + 3;
            break;
        } else if ( '\n' == * ( p + 1 ) ) {
            body = p + 2;
            break;
        }

        t = p + 1;
    }

    assert( header );
    assert( header != buf );

    /*
    if ( NULL == body ) {
        // status line follow empty line:
        // GET /abc
        // \n
        // \n
        // 
        context->http_header_offset = (uint16_t)( header - buf );
        context->http_body_offset = 0;
        context->http_body_length = 0;
        * r = ME_OK;
        return true;
    }
    */

    * header_offset = (int)( header - buf );

    c = * ( body - 1 );
    * ( body - 1 ) = '\0';

    // 找 Content-Length
    t = strstr( header, "Content-Length:" );
    if ( t ) {
        // ignore "Content-Length:"
        t += 15;
        // ignore leading space
        t += parser_ignore_spaces( t, t + strlen( t ) );
        if ( * t > 0 && isdigit( * t ) ) {
            * content_length = atol( t );
            if ( content_length > 0 ) {
                // body is full?
                int old_content_len = (int)( buf + len - body );
                if ( * content_length > old_content_len ) {
                    // body not full
                    * ( body - 1 ) = c;
                    return false;
                }
            }
        }
    }

    * ( body - 1 ) = c;
    * body_offset = (int)( body - buf );
    if ( (int)-1 == * content_length ) {
        * content_length = 0;
    }

    return true;
}

void
http_check(
    const void * data,
    int len,
    gr_check_ctxt_t * ctxt,
    bool * is_error,
    bool * is_full
)
{
    ctxt->cc_package_type = (uint16_t)check_http_packet_type( data, len );
    if ( GR_PACKAGE_ERROR != ctxt->cc_package_type ) {

        // HTTP || HTTP_REPLY

        bool is_e;
        int header_offset;
        int body_offset;
        int content_length;

        assert( GR_PACKAGE_HTTP_REQ == ctxt->cc_package_type
             || GR_PACKAGE_HTTP_REPLY == ctxt->cc_package_type );

        if ( ! is_http_full_packet( data, len,
            (GR_PACKAGE_HTTP_REPLY == ctxt->cc_package_type) ? false : true,
            & is_e,
            & header_offset,
            & body_offset,
            & content_length  ) )
        {
            if ( is_e ) {
                // HTTP wrong
                * is_error = true;
                * is_full = false;
            } else {
                // HTTP not full
                * is_error = false;
                * is_full = false;
            }
            return;
        }

        // package is full
        * is_error = false;
        * is_full = true;

        if ( body_offset + content_length > len ) {

            if ( 0 == content_length && body_offset == len + 1 ) {
                // content_length is 0, and suffix by 0x0d 0x0a 0x0d, missin 0x0a
                * is_full = false;
                return;
            } else {
                // faint! somebody try to play me!
                * is_error = true;
                * is_full = false;

        	    //output_log( "error", "%s:%d:%s bad packet: content_length=%d,body_offset=%d,len=%d",
        		//    __FILE__, __LINE__, __FUNCTION__, content_length, body_offset, len );
                return;
            }
        }

        ctxt->cc_http_body_offset = (uint16_t)body_offset;
        ctxt->cc_http_content_length = (uint32_t)content_length;
        ctxt->cc_http_header_offset = (uint16_t)header_offset;
    }
}


#define ISXDIGIT(c) (isxdigit(((unsigned char)(c))))
#define IS_SLASH(s) ((s == '/') || (s == '\\'))

static_inline
char
x2c(
    const char *what
)
{
    char digit;

    digit = ((what[0] >= 'A')
            ? ((what[0] & 0xdf) - 'A') + 10
            : (what[0] - '0'));
    digit *= 16;
    digit += (what[1] >= 'A'
            ? ((what[1] & 0xdf) - 'A') + 10
            : (what[1] - '0'));
    return digit;
}

static_inline
size_t
http_decode(
    char * s
)
{
    int badesc;
    int badpath;
    char * x;
    char * y;
    size_t len;

    // watch performance

    if ( NULL == s )
        return 0;

    // replace + to ' '
    // alone loop, but fast
    y = strchr( s, '+' );
    while ( y ) {
        * y = ' ';
        y = strchr( y + 1, '+' );
    }

    badesc = 0;
    badpath = 0;

    y = strchr( s, '%' );
    if ( y ) {
        // do not use strchr, this code better than strchr
        for ( x = y; *y; ++x, ++y ) {
            if ( *y != '%' ) {
                *x = *y;
            } else if ( ! ISXDIGIT( *(y + 1) ) || ! ISXDIGIT( *(y + 2) ) ) {
                badesc = 1;
                *x = '%';
            } else {
                *x = x2c(y + 1);
                y += 2;
                if (IS_SLASH(*x) || *x == '\0') {
                    badpath = 1;
                }
            }
        }
        *x = '\0';

        len = x - s;
    } else {
        len = strlen( s );
    }

    if (badesc) {
        gr_error( "invalid params" );
        return 0;
    }
    //else if (badpath)
    //    return 0;

    return len;
}

static_inline
parse_buf_t * get_parse_buf(
    gr_proc_ctxt_t * ctxt
)
{
    gr_http_t * p = (gr_http_t *)g_ghost_rocket_global.http;
    assert( ctxt->pc_thread_id < p->parse_buf_count );
    return & p->parse_bufs[ ctxt->pc_thread_id ];
}

static_inline
void
http_execute(
    gr_http_ctxt_t *    http,
    gr_conn_buddy_t *   conn_buddy,
    int *               processed_len
)
{
    bool is_processed = false;

    // default process
    http_execute_inner( http, conn_buddy, processed_len, & is_processed );
    if ( ! is_processed ) {

        // module process
        gr_module_proc_http( http, conn_buddy, processed_len );
    }
}

void
http_proc(
    const char *        buf,
    int                 len,
    gr_proc_ctxt_t *    ctxt,
    gr_conn_buddy_t *   conn_buddy,
    int *               processed_len
)
{
    char *                  p;
    char *                  status_line_lf;
    char *                  request_question = NULL;
    char *                  header;
    gr_http_param_t *       request_params = NULL;
    size_t                  request_params_count = 0;
    gr_http_param_t *       request_header = NULL;
    size_t                  request_header_count = 0;
    gr_http_param_t *       request_form = NULL;
    size_t                  request_form_count = 0;
    size_t                  http_body_length;
    parse_buf_t *           parse_buf;
    const char *            user_connection = NULL;
    const char *            user_proxy_connection = NULL;

    gr_http_ctxt_t          http_context;

    parse_buf = get_parse_buf( ctxt );

    * processed_len = 0;
    ctxt->pc_result_buf_len = 0;

    http_body_length = (size_t)ctxt->pc_http_content_length;

    //memset( & http_context, 0, sizeof( struct GsHttpContext ) );
    http_context.params_count = 0;
    http_context.params = NULL;
    http_context.header_count = 0;
    http_context.header = NULL;
    http_context.form_count = 0;
    http_context.form = NULL;
    http_context.body_len = 0;
    http_context.body = NULL;
    http_context.content_type = NULL;
    http_context.user_agent = NULL;

    do {

        header = (char *)& buf[ ctxt->pc_http_header_offset ];

        // request type
        if ( GR_PACKAGE_HTTP_REPLY == ctxt->pc_package_type ) {
            // '\0': reply package
            http_context.request_type = '\0';
        } else {
            http_context.request_type = * buf;
            assert( 'G' == http_context.request_type
                 || 'P' == http_context.request_type
                 || 'H' == http_context.request_type
                 || 'C' == http_context.request_type );
        }

        // request directory
        // request string and request string length
        switch( http_context.request_type ) {
        case 'G': // GET [space]
            http_context.directory = (char *)buf + 3 + 1;
            break;
        case 'H': // HEAD [space]
        case 'P': // POST [space]
            http_context.directory = (char *)buf + 4 + 1;
            break;
        case 'C': // CONNECT [space]
            if ( ! str8cmp( buf, 'C', 'O', 'N', 'N', 'E', 'C', 'T', ' ' ) ) {
                // invalid package
                * processed_len = -1;
                return;
            }
            http_context.directory = (char *)buf + 7 + 1;
            break;
        case '\0':
            // reply
            http_context.directory = "/";
            break;
        default:
            http_context.directory = "";
            break;
        };

        if ( GR_PACKAGE_HTTP_REPLY == ctxt->pc_package_type ) {

            // status line must follow a \n
            status_line_lf = strchr( buf, '\n' );

            // http version
            p = strchr( buf, ' ' );

            if ( p ) {

                * p = '\0';
                http_context.version = (char *)buf;
            } else {
                // invalid package

                * processed_len = -2;
                return;
            }

            // query object
            http_context.object = (char*)"";
            * request_question = '\0';

            ++ p;

            // HTTP/1.1 200 OK\r\n
            if ( * p > 0 && isdigit( * p ) ) {

                // HTTP error code
                http_context.http_reply_code = atoi( p );

            } else {

                // invalid package
                * processed_len = -3;
                return;
            }

        } else {

            http_context.http_reply_code = 0;

            if ( NULL == http_context.directory ) {
                // invalid package
                * processed_len = -4;
                return;
            }

            // status line must follow a \n
            status_line_lf = strchr( http_context.directory, '\n' );
            if ( status_line_lf ) {
                * status_line_lf = '\0';
            } else {
                // invalid package
                * processed_len = -5;
                return;
            }

            // http version
            p = strchr( http_context.directory, ' ' );
            if ( p ) {
                * p = '\0';
                http_context.version = p + 1;
            } else {
                http_context.version = (char*)"HTTP/0.9";
            }

            // query object
            request_question = strchr( http_context.directory, '?' );
            if ( NULL == request_question ) {
                p = strrchr( http_context.directory, '/' );
                if ( p ) {
                    * p ++ = '\0';
                    http_context.object = p;
                } else {
                    // not normal http request, maybe a proxy request
                    http_context.object = "";
                }
            } else {
                * request_question = '\0';

                p = strrchr( http_context.directory, '/' );
                if ( p ) {
                    * p ++ = '\0';
                } else {
                    // invalid package
                    * processed_len = -6;
                    return;
                }
                http_context.object = p;

                * request_question = '?';
            }

            // request question
            request_question = strchr( http_context.object, '?' );
            if ( request_question ) {
                * request_question ++ = '\0';
            }

            http_decode( http_context.object );
            http_decode( http_context.directory );

            // parse query string
            if ( request_question && * request_question ) {

                char * s;
                char * qs = request_question;
                size_t param_count = 0;

                param_count = 0;
                while( qs && * qs ) {

                    s = strchr( qs, '&' );

                    ++ param_count;

                    qs = s;
                    if ( qs )
                        ++ qs;
                }

                if ( (int)param_count > parse_buf->query_max ) {
                    * processed_len = -7;
                    return;
                }

                qs = request_question;

                request_params = parse_buf->query;
                assert( request_params );

                request_params_count = param_count;

                param_count = 0;

                while( qs && * qs ) {

                    s = strchr( qs, '&' );

                    if ( s )
                        * s = '\0';

                    request_params[ param_count ].name = (char *)qs;

                    request_params[ param_count ].value = strchr( request_params[ param_count ].name, '=' );
                    if ( request_params[ param_count ].value ) {
                        * request_params[ param_count ].value = '\0';
                        ++ request_params[ param_count ].value;
                    } else {
                        * processed_len = -8;
                        return;
                    }

                    http_decode( request_params[ param_count ].name );
                    http_decode( request_params[ param_count ].value );

                    if ( s )
                        ++ s;
                    qs = s;

                    ++ param_count;
                }
            }

        }

        // parse HTTP header
        if ( NULL == header ) {
            * processed_len = -9;
            return;
        }

        if ( * ( header - 2 ) == '\r' )
            * ( header - 2 ) = '\0';
        else
            * ( header - 1 ) = '\0';

        {
            char * s;
            char * qs = header;
            size_t param_count = 0;

            param_count = 0;
            while( qs && * qs ) {

                if ( '\r' == * qs || '\n' == * qs ) {
                    break;
                }

                s = strchr( qs, '\n' );

                ++ param_count;

                qs = s;
                if ( qs )
                    ++ qs;
            }

            if ( qs && ( '\r' == * qs || '\n' == * qs ) ) {

                if ( '\r' == * qs ) {
                    * qs = '\0';
                    qs += 2;
                } else {
                    * qs ++ = '\0';
                }

                if ( '\0' == * qs ) {
                    http_body_length = 0;
                } else {
                    if ( '\r' == * qs )
                        ++ qs;
                    if ( '\n' == * qs )
                        ++ qs;
                }
            }

            if ( (int)param_count > parse_buf->header_max ) {
                * processed_len = -10;
                return;
            }

            qs = header;

            request_header = parse_buf->header;
            assert ( request_header );

            request_header_count = param_count;

            param_count = 0;
            while( qs && * qs && param_count < request_header_count ) {

                s = strchr( qs, '\n' );

                if ( s )
                    * s = '\0';

                request_header[ param_count ].name = (char *)qs;

                request_header[ param_count ].value = strchr( request_header[ param_count ].name, ':' );
                if ( request_header[ param_count ].value ) {
                    * request_header[ param_count ].value = '\0';
                    ++ request_header[ param_count ].value;
                } else {
                    * processed_len = -11;
                    return;
                }

                // ignore space
                request_header[ param_count ].value = str_trim( request_header[ param_count ].value, NULL );

                http_decode( request_header[ param_count ].name );
                http_decode( request_header[ param_count ].value );

                // member Content-Type
                if ( NULL == http_context.content_type ) {
                    if ( 0 == strcmp( "Content-Type", request_header[ param_count ].name ) ) {
                        http_context.content_type = request_header[ param_count ].value;
                    }
                }

                // member User-Agent
                if ( NULL == http_context.user_agent ) {
                    if ( 0 == strcmp( "User-Agent", request_header[ param_count ].name ) ) {
                        http_context.user_agent = request_header[ param_count ].value;
                    }
                }

                // 
                if ( NULL == user_connection ) {
                    if ( 0 == strcmp( "Connection", request_header[ param_count ].name ) ) {
                        user_connection = request_header[ param_count ].value;
                    }
                }
                if ( NULL == user_proxy_connection ) {
                    if ( 0 == strcmp( "Proxy-Connection", request_header[ param_count ].name ) ) {
                        user_proxy_connection = request_header[ param_count ].value;
                    }
                }

                if ( s )
                    ++ s;
                qs = s;

                ++ param_count;
            }

        }

        if ( http_body_length ) {

            //const char * charset = NULL;

            if ( NULL == http_context.content_type || '\0' == * http_context.content_type ) {
                http_context.content_type = "application/x-www-form-urlencoded";
                //http_context.content_type = (char*)"application/octet-stream";
            }

            if ( IS_PARSE_HTTP_POST_DATA && 0 == strcmp( http_context.content_type, "application/x-www-form-urlencoded" ) ) {

                char * form = (char *)buf + ctxt->pc_http_body_offset;
                char * s;
                char * qs = form;
                size_t param_count = 0;

                param_count = 0;
                while( qs && * qs ) {

                    s = strchr( qs, '&' );

                    ++ param_count;

                    qs = s;
                    if ( qs )
                        ++ qs;
                }

                if ( (int)param_count > parse_buf->form_max ) {
                    * processed_len = -12;
                    return;
                }

                qs = form;

                request_form = parse_buf->form;
                assert ( request_form );

                request_form_count = param_count;

                param_count = 0;

                while( qs && * qs ) {

                    s = strchr( qs, '&' );

                    if ( s )
                        * s = '\0';

                    request_form[ param_count ].name = (char *)qs;

                    request_form[ param_count ].value = strchr( request_form[ param_count ].name, '=' );
                    if ( request_form[ param_count ].value ) {
                        * request_form[ param_count ].value = '\0';
                        ++ request_form[ param_count ].value;
                    } else {
                        * processed_len = -13;
                        return;
                    }


                    http_decode( request_form[ param_count ].name );
                    http_decode( request_form[ param_count ].value );

                    if ( s )
                        ++ s;
                    qs = s;

                    ++ param_count;
                }

            } else {

                http_context.body = (char *)buf + ctxt->pc_http_body_offset;
                http_context.body_len = http_body_length;

            }
        }

        http_context.params = request_params;
        http_context.params_count = request_params_count;
        http_context.header = request_header;
        http_context.header_count = request_header_count;
        http_context.form = request_form;
        http_context.form_count = request_form_count;

        if ( NULL == http_context.content_type )
            http_context.content_type = "";
        if ( NULL == http_context.user_agent )
            http_context.user_agent = "";

        if ( NULL == http_context.directory || '\0' == * http_context.directory )
            http_context.directory = (char*)"/";
        if ( NULL == http_context.object )
            http_context.object = (char*)"";

        http_context.base = ctxt;
        //memcpy( & http_context.base, & ctxt->base, sizeof( GsProcCtxt ) );

        http_context.is_error = false;
        http_context.keep_alive = true;

        http_execute( & http_context, conn_buddy, processed_len );

    } while ( 0 );

    if ( ! http_context.is_error ) {

        if ( http_context.keep_alive ) {
            p = (char*)user_connection;
            if ( NULL == p )
                p = (char*)user_proxy_connection;
            if ( NULL == p || str6cmp( p, 'c', 'l', 'o', 's', 'e', '\0' ) ) {
                // short connection
                http_context.keep_alive = false;
            }
        }

        if ( http_context.keep_alive ) {
            * processed_len = (int)(
                    ctxt->pc_http_body_offset
                  + ctxt->pc_http_content_length );
        } else {
            * processed_len = 0;
        }

    } else {
        * processed_len =
            - (int)( ctxt->pc_http_body_offset
                   + ctxt->pc_http_content_length );
    }
}
