/**
 * @file include/gr_global.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   全局唯一对象
 * Revision History 大事件记
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-05    Created.
 **/
#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_GLOBAL_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_GLOBAL_H_

#include "gr_log.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    gr_server_t         server_interface;

    // 开始打日志的级别。级别比该值小的日志不会打
    gr_log_level_t      log_start_level;

    // 返回数据包对齐字节数
    int                 rsp_buf_align;

    // gr_log模块
    void *              log;
    // gr_config模块
    void *              config;
    // gr_server模块
    void *              server;
    // gr_module模块
    void *              module;
    // gr_tcp_accept模块
    void *              tcp_accept;
    // gr_tcp_in模块
    void *              tcp_in;
    // gr_udp_in模块
    void *              udp_in;
    // gr_tcp_out模块
    void *              tcp_out;
    // gr_udp_out模块
    void *              udp_out;
    // gr_worker模块
    void *              worker;
    // gr_backend模块
    void *              backend;
    // gr_conn模块
    void *              conn;
    // gr_library模块
    void *              library;

} gr_global_t;

// 直接用extern全局变量,目的是防止通过函数调用取该结构导致的函数调用开销
extern gr_global_t g_ghost_rocket_global;

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_GLOBAL_H_
