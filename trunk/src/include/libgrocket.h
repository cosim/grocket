/**
 * @file include/libgrocket.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/09/24
 * @version $Revision$ 
 * @brief 服务框架的静态库版本，调用方可以自己提供可执行程序。
 *        给服务框架编写扩展模块的方法完全相同。
 *        该接口适用于必须使用定制的可执行程序的场景。
 *
 * Revision History 大事件记
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-09-24    Created.
 **/

#ifndef _GHOST_ROCKET_INCLUDE_LIBGROCKET_H_
#define _GHOST_ROCKET_INCLUDE_LIBGROCKET_H_

#include "gr_stdinc.h"
#include "grocket.h"

#ifdef __cplusplus
extern "C" {
#endif

int
gr_main(
    int             argc,
    char **         argv,
    const char *    ini_content,
    size_t          ini_content_len,
    gr_init_t       init,
    gr_term_t       term,
    gr_tcp_accept_t tcp_accept,
    gr_tcp_close_t  tcp_close,
    gr_check_t      chk_binary,
    gr_proc_t       proc_binary,
    gr_proc_http_t  proc_http
);

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GHOST_ROCKET_INCLUDE_LIBGROCKET_H_
