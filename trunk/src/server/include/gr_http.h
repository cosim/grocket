/**
 * @file include/gr_http.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   HTTP协议处理
 * Revision History 大事件记
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-05    Created.
 **/
#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_HTTP_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_HTTP_H_

#include "gr_stdinc.h"

#ifdef __cplusplus
extern "C" {
#endif

int gr_http_init();

void gr_http_term();

#ifdef __cplusplus
}
#endif

#endif // ! #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_HTTP_H_
