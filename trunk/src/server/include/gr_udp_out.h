/**
 * @file include/gr_udp_out.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/03
 * @version $Revision$ 
 * @brief   UDP 输出线程
 * Revision History 大事件记
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-03    Created.
 **/
#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_UDP_OUT_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_UDP_OUT_H_

#include "gr_conn.h"

#ifdef __cplusplus
extern "C" {
#endif

int gr_udp_out_init();

void gr_udp_out_term();

int gr_udp_out_add(
    gr_udp_rsp_t * rsp
);

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_UDP_OUT_H_
