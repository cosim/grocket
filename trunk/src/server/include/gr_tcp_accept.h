/**
 * @file include/gr_tcp_accept.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/03
 * @version $Revision$ 
 * @brief   TCP Accept线程
 * Revision History 大事件记
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-03    Created.
 **/
#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_TCP_ACCEPT_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_TCP_ACCEPT_H_

#include "gr_conn.h"

#ifdef __cplusplus
extern "C" {
#endif

int gr_tcp_accept_init();

void gr_tcp_accept_term();

int gr_tcp_accept_add_listen_ports();

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_TCP_ACCEPT_H_
