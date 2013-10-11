/**
 * @file include/gr_tools.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/03
 * @version $Revision$ 
 * @brief   通用工具函数
 * Revision History 大事件记
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-03    Created.
 **/
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

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_TOOLS_H_
