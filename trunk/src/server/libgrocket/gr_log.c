/**
 * @file libgrocket/gr_log.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   log
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

#include "gr_log.h"
#include "gr_global.h"
#include "gr_errno.h"
#include "gr_mem.h"
#include "gr_config.h"

typedef struct
{
    bool    enable_tid;
} gr_log_t;

int gr_log_open()
{
    gr_log_t *  p;
    int r;

    remove( "log.txt" );

    if ( NULL != g_ghost_rocket_global.log ) {
        gr_fatal( "[init]gr_log_open already called" );
        return GR_ERR_WRONG_CALL_ORDER;
    }

    p = (gr_log_t *)gr_calloc( 1, sizeof( gr_log_t ) );
    if ( NULL == p ) {
        gr_fatal( "[init]malloc %d bytes failed, errno=%d,%s",
            (int)sizeof(gr_log_t), errno, strerror( errno ) );
        return GR_ERR_BAD_ALLOC;
    }

    r = GR_ERR_UNKNOWN;

    do {

        p->enable_tid = gr_config_log_enable_tid();

        r = GR_OK;
    } while ( false );

    if ( GR_OK != r ) {
        gr_free( p );
        return r;
    }

    g_ghost_rocket_global.log = p;
    return GR_OK;
}

void gr_log_close()
{
    gr_log_t * p = (gr_log_t *)g_ghost_rocket_global.log;

    if ( NULL != p ) {

        gr_free( p );
        g_ghost_rocket_global.log = NULL;
    }
}

void gr_log_on_config_ready()
{
    gr_log_t * p = (gr_log_t *)g_ghost_rocket_global.log;
    if ( NULL != p ) {
        p->enable_tid = gr_config_log_enable_tid();
    }
}

static inline
void log_write_file(
    gr_log_t *      log,
    const char *    data,
    int             data_len
)
{
    //TODO:

    FILE * fp = fopen( "log.txt", "ab" );
    if ( NULL != fp ) {
        fwrite( data, 1, data_len, fp );
        fclose( fp );
    }
}

void gr_log_write(
    const char *    file,
    int             line,
    const char *    func,
    gr_log_level_t  level,
    const char *    fmt,
    ...
)
{
    char            buf[ 1024 ];
    const char *    p;
    const char *    level_s;
    va_list         ap;
    int             r;
    int             r2;
    gr_log_t *      log_obj = (gr_log_t *)g_ghost_rocket_global.log;

#if defined( WIN32 ) || defined( WIN64 )
    // windows 的 vsnprintf 在缓冲区不足时会直接返回 -1,
    // 导致我不知道写了多少字节数据, 所以先清空缓冲区,反正我也没打算在 Windows 上运行多快.
    memset( buf, 0, sizeof( buf ) );
#endif

    if ( GR_LOG_FATAL == level )
        level_s = "fatal  |";
    else if ( GR_LOG_ERROR == level )
        level_s = "error  |";
    else if ( GR_LOG_WARNING == level )
        level_s = "warning|";
    else if ( GR_LOG_INFO == level )
        level_s = "info   |";
    else if ( GR_LOG_DEBUG == level )
        level_s = "debug  |";
    else
        level_s = "unknown|";

    p = strrchr( file, S_PATH_SEP_C );
    if ( p ) {
        file = p + 1;
    }

    #define LEADDING_BYTES  8
    // 将日志内容打印出来
    va_start( ap, fmt );
    r = vsnprintf( & buf[ LEADDING_BYTES ], sizeof( buf ) - LEADDING_BYTES, fmt, ap );
    va_end ( ap );
    buf[ sizeof( buf ) - 1 ] = '\0';
    // 将日志类型前缀拷贝过去
    memcpy( buf, level_s, LEADDING_BYTES );

    if ( r >= sizeof( buf ) - LEADDING_BYTES ) {
        // 缓冲区不足. 这是给非 Windows准备的
        r = sizeof( buf ) - 1;
    } else if ( r < 0 ) {
        // 在 windows 上缓冲区不足会返回 -1, 在非Windows上不会进入这个分支
        r = (int)strlen( buf );
    } else {
        r += LEADDING_BYTES;
    }

    // 日志内容和后面的文件名区分开来
    #define MIN_LOG_LEN     64
    if ( r < MIN_LOG_LEN ) {
        memset( & buf[ r ], ' ', MIN_LOG_LEN - r );
        r = MIN_LOG_LEN;
    }
    buf[ r ] = '\0';

    if ( level >= GR_LOG_WARNING || NULL == log_obj || log_obj->enable_tid ) {
        r2 = snprintf( & buf[ r ], sizeof( buf ) - r, " | [pid=%d][tid=%d][%s:%d:%s]",
            getpid(), gettid(), file, (int)line, func);
    } else {
        r2 = snprintf( & buf[ r ], sizeof( buf ) - r, " | [pid=%d][%s:%d:%s]",
            getpid(), file, (int)line, func);
    }
    if ( r2 >= (int)sizeof( buf ) - r ) {
        // 缓冲区不足. 这是给非 Windows准备的
        r = sizeof( buf ) - 1;
    } else if ( r < 0 ) {
        // 在 windows 上缓冲区不足会返回 -1, 在非Windows上不会进入这个分支
        r += (int)strlen( & buf[ r ] );
    } else {
        r += r2;
    }

    // 打回车换行, 这个必须加成功,缓冲区不够就截日志, 
    // 前面加至少两个半角空格是防止遇到乱码导致换行不成功
    #define LOG_TAIL "  " S_CRLF
    if ( sizeof( buf ) - r >= sizeof( LOG_TAIL ) ) {
        // 缓冲区足够. 连 '\0'也拷贝进去了
        memcpy( & buf[ r ], LOG_TAIL, sizeof( LOG_TAIL ) );
        r += sizeof( LOG_TAIL ) - 1;
    } else {
        // 缓冲区不足, 截断日志
        memcpy( & buf[ sizeof( buf ) - sizeof( LOG_TAIL ) ], LOG_TAIL, sizeof( LOG_TAIL ) );
        r = sizeof( buf ) - 1;
    }

    log_write_file( log_obj, buf, r );
}
