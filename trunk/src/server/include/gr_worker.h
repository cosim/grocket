/**
 * @file include/gr_worker.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   工作线程或工作进程
 * Revision History 大事件记
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-05    Created.
 **/
#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_WORK_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_WORK_H_

#include "gr_conn.h"

#ifdef __cplusplus
extern "C" {
#endif

int gr_worker_init();

void gr_worker_term();

int gr_worker_add_tcp(
    gr_tcp_req_t *  req,
    bool            is_emerge
);

int gr_worker_add_udp(
    gr_tcp_req_t *  req,
    bool            is_emerge
);

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_WORK_H_
