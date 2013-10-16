/**
 * @file include/gr_tools.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/03
 * @version $Revision$ 
 * @brief   generic tool
 * Revision History
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-03    Created.
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

#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_TOOLS_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_TOOLS_H_

#include "gr_stdinc.h"

#ifdef __cplusplus
extern "C" {
#endif

int
get_errno();

/**
* @brief 取得可执行文件路径
* @param [out] char * path: 路径缓冲区
* @param [in] size_t path_len: 路径缓冲区字节数，带字符串结束符
* @return 返回实际的路径字节数，不带字符串结束符
*/
size_t
get_exe_path(
    char * path,
    size_t  path_len
);

/**
 * @brief 将字符串两边的空格去掉，返回新的起始位置，和新的长度
 *   @param[in,modify] char * s: 传入字符串，函数内部会对它的内容做修改
 *   @param[in,out] size_t * len: 传入字符串长度（不算结束符），传出去掉空格后的字符串长度，如果此值为-1，则函数会做strlen
 * @return char *: 返回去掉空格后的字符串起始位置。这个指针>= s参数
 * @warning: 本函数会修改字符串内容
 */
char *
str_trim(
    char * s,
    size_t * len
);

/**
 * @brief transform / or \ separate path to current OS separate path
 * @param[in, out] car * path: path
 * @code
       char path[ 256 ] = "./log/today.log";
       path_to_os( path );
 * @endcode
 */
void
path_to_os(
    char * path
);

void
sleep_ms(
    uint32_t ms
);

bool
is_exists(
    const char * path
);

#if defined( WIN32 ) || defined( WIN64 )
    #define gr_yield()      Sleep( 0 )
#elif defined( __linux )
    #define gr_yield()      pthread_yield()
#elif defined( __APPLE__ )
    #define gr_yield()      pthread_yield_np()
#else
    #error unknown platform
#endif

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_TOOLS_H_
