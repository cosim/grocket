/**
 * @file include/gr_udp_in.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/03
 * @version $Revision$ 
 * @brief   UDP数据接收线程
 * Revision History 大事件记
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-03    Created.
 **/
#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_UDP_IN_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_UDP_IN_H_

#include "gr_stdinc.h"

#ifdef __cplusplus
extern "C" {
#endif

int gr_udp_in_init();

void gr_udp_in_term();

int gr_udp_in_add_listen_ports();

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_UDP_IN_H_
