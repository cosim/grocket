#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_CONFIG_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_CONFIG_H_

#include "gr_stdinc.h"

#ifdef __cplusplus
extern "C" {
#endif

int gr_config_init(
    const char *        ini_content,
    size_t              ini_content_len
);

void gr_config_term();

bool gr_config_is_daemon();

int gr_config_log_level( int def );

bool gr_config_is_tcp_disabled();

int gr_config_tcp_accept_concurrent();
int gr_config_tcp_accept_thread_count();

int gr_config_tcp_in_concurrent();
int gr_config_tcp_in_thread_count();
int gr_config_udp_in_concurrent();

int gr_config_udp_in_thread_count();

int gr_config_tcp_out_thread_count();
int gr_config_tcp_out_concurrent();

int gr_config_udp_out_concurrent();
int gr_config_udp_out_thread_count();

int gr_config_worker_thread_count();

int gr_config_backend_thread_count();

void gr_config_get_module_path( char * path, size_t path_max, bool * is_absolute );

int gr_config_tcp_accept_send_buf();
int gr_config_tcp_accept_recv_buf();
int gr_config_udp_send_buf();
int gr_config_udp_recv_buf();

int gr_config_get_listen_backlog();

int gr_config_get_tcp_recv_buf_init();
int gr_config_get_tcp_recv_buf_max();

int gr_config_library_class_max();
const char * gr_config_library_core_path();

int gr_config_rsp_buff_align();

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_CONFIG_H_
