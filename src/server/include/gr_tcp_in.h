/**
 * @file include/gr_tcp_in.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/03
 * @version $Revision$ 
 * @brief   TCP数据接收线程
 * Revision History 大事件记
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-03    Created.
 **/
#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_TCP_IN_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_TCP_IN_H_

#include "gr_conn.h"

#ifdef __cplusplus
extern "C" {
#endif

int gr_tcp_in_init();

void gr_tcp_in_term();

int gr_tcp_in_add_conn(
    gr_tcp_conn_item_t *    conn
);

#if defined( WIN32 ) || defined( WIN64 )

void * gr_tcp_in_get_poll();

#endif // #if defined( WIN32 ) || defined( WIN64 )

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_TCP_IN_H_
