#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_MODULE_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_MODULE_H_

/**
 * @file include/gr_module.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   用户模块
 * Revision History 大事件记
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-05    Created.
 **/
#include "gr_stdinc.h"
#include "grocket.h"
#include "gr_conn.h"

#ifdef __cplusplus
extern "C" {
#endif

int gr_module_init(
    gr_init_t       init,
    gr_term_t       term,
    gr_tcp_accept_t tcp_accept,
    gr_tcp_close_t  tcp_close,
    gr_check_t      chk_binary,
    gr_proc_t       proc_binary,
    gr_proc_http_t  proc_http);

void gr_module_term();

int gr_module_master_process_init();
void gr_module_master_process_term();

int gr_module_child_process_init();
void gr_module_child_process_term();

int gr_module_worker_init( int worker_id );
void gr_module_worker_term( int worker_id );

bool gr_module_on_tcp_accept(
    gr_port_item_t *    port_item,
    int                 fd
);

void gr_module_check_tcp(
    gr_tcp_req_t *      req,
    bool *              is_error,
    bool *              is_full
);

void gr_module_proc_tcp(
    gr_tcp_req_t *      req,
    gr_proc_ctxt_t *    ctxt,
    int *               processed_len
);

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_MODULE_H_
