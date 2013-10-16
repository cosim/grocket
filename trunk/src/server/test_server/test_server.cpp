/**
 * @file test_server/test_server.cpp
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/15
 * @version $Revision$ 
 * @brief   test server
 * Revision History
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-15    Created.
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

#include "libgrocket.h"

#if defined(WIN32)

    #if defined(_DEBUG)
        #pragma comment( lib, "../bin/Win32/Debug/libgrocket.lib" )
    #else
        #pragma comment( lib, "../bin/Win32/Release/libgrocket.lib" )
    #endif
#elif defined(WIN64)
    #if defined(_DEBUG)
        #pragma comment( lib, "../bin/x64/Debug/libgrocket.lib" )
    #else
        #pragma comment( lib, "../bin/x64/Release/libgrocket.lib" )
    #endif
#endif

extern "C"
{

extern int gr_init(
    gr_process_type_t   proc_type,
    gr_server_t *       server
);

extern void gr_term(
    gr_process_type_t   proc_type,
    void *              must_be_zero
);

extern void gr_tcp_accept(
    int                 port,
    int                 sock,
    bool *              need_disconnect
);

extern void gr_tcp_close(
	int                 port,
    int                 sock
);

extern void gr_check(
    void *              data,
    int                 len,
    gr_port_item_t *    port_info,
    int                 sock,
    gr_check_ctxt_t *   ctxt,
    bool *              is_error,
    bool *              is_full
);

extern void gr_proc(
    const void *        data,
    int                 len,
    gr_proc_ctxt_t *    ctxt,
    int *               processed_len
);

extern void gr_proc_http(
    gr_http_ctxt_t *    http
);

} // extern "C"

int main( int argc, char ** argv )
{
    char config[ 1024 ] = "";
    int config_len;
    strcpy( config,
        "[server]\n"
        "daemon                      = false\n"
        "manual_tcp                  = false\n"
        "network_in_concurrent       = 10000\n"
        "tcp_accept_send_buf         = 8388608\n"
        "tcp_accept_recv_buf         = 8388608\n"
        "udp_send_buf                = 8388608\n"
        "udp_recv_buf                = 8388608\n"
        "listen_backlog              = 511\n"
        "\n"
        "network_in_thread_count     = 1\n"
        "network_out_thread_count    = 1\n"
        "worker_thread_count         = 10\n"
        "backend_thread_count        = 1\n"
        "\n"
        "module                      = ./demo_module\n"
        "\n"
        "[listen]\n"
        "0 = tcp://0.0.0.0:65535\n"
        //"1 = udp://0.0.0.0:65535\n"
    );
    config_len = strlen( config );

    return gr_main(
        argc, argv,
        config, config_len,
        gr_init, gr_term, gr_tcp_accept, gr_tcp_close, gr_check, gr_proc, gr_proc_http
    );
}
