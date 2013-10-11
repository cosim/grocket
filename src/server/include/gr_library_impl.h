/**
 * @file include/gr_library_impl.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/08
 * @version $Revision$ 
 * @brief   服务器功能函数库
 * Revision History 大事件记
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-08    Created.
 **/
#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_LIBRARY_IMPL_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_LIBRARY_IMPL_H_

#include "gr_stdinc.h"
#include "grocket.h"

#ifdef __cplusplus
extern "C" {
#endif

int gr_library_impl_init();

void gr_library_impl_term();

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_LIBRARY_IMPL_H_
