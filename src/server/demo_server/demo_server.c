/**
 * @file demo_server/demo_server.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/09/29
 * @version $Revision$ 
 * @brief   server framework demo alone server
 * Revision History
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-09-29    Created.
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

// 非常详细的服务器框架接口使用的细节详见 demo_module.c
// 本程序示范如何写一个服务器进程。

// 要自己提供一个服务器可执行程序，这种场景适用于一些比较复杂的应用场景，比如迁移等。
// 使用者可以初始化完自己的事儿之后再初始化服务器，甚至启服务器的动作在一个单独的线
// 程或单独的进程里去做。这里只是演示一下如何达成该目标。
// 这次必须包含的是 libgrocket.h 头文件。
#include "libgrocket.h"

// 这次必须要连接静态库了
// Windows下的静态库依赖
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

// 注意：本工程直接使用了demo_module.c 源文件，所以给服务器写扩展的方法还是原封不动的,
// 唯一区别就是扩展的处理函数就在可执行文件里，不用再装载动态库了。
// 我只需要在这里将 demo_module.c 文件中的函数声明一下。

extern void gr_version(
    int *               gr_server_version
);

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
    gr_conn_buddy_t *   conn_buddy,
    bool *              need_disconnect
);

extern void gr_tcp_close(
	int                 port,
    int                 sock,
    gr_conn_buddy_t *   conn_buddy
);

extern void gr_check(
    void *              data,
    int                 len,
    gr_check_ctxt_t *   ctxt,
    gr_conn_buddy_t *   conn_buddy,
    bool *              is_error,
    bool *              is_full
);

extern void gr_proc(
    const void *        data,
    int                 len,
    gr_proc_ctxt_t *    ctxt,
    gr_conn_buddy_t *   conn_buddy,
    int *               processed_len
);

extern void gr_proc_http(
    gr_http_ctxt_t *    http,
    gr_conn_buddy_t *   conn_buddy,
    int *               processed_len
);

// 接下来写 main 函数
int main( int argc, char ** argv )
{
    // 到这儿已经完成 89% 了，先问你一个问题：服务器框架需要的配置grocket.ini也需要
    // 拷贝到可执行程序目录下，可以么？
    // 如果你说不行，你说必须从其它的配置文件中读取配置？或者你需要一个没有配置文件的可执行程序？
    // 那好，我们手工加一个配置：
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

    // 到这儿已经完成 99% 了，继续完成剩余的 1%

    return gr_main(
        // 可执行文件参数
        argc, argv,
        // 配置文件
        config, config_len,
        // 服务器模块处理函数
        gr_version, gr_init, gr_term, gr_tcp_accept, gr_tcp_close, gr_check, gr_proc, gr_proc_http
    );
}

// 写完了？
// 写完了。
// 写完了？!
// 写完了。
