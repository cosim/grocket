/**
 * @file include/gr_config.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   config file
 * Revision History
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-05    Created.
 **/
/* 
 *
 * Copyright (C) 2013-now da_ming at hotmail.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
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
bool gr_config_is_debug();

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

bool gr_config_log_enable_tid();

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_CONFIG_H_
