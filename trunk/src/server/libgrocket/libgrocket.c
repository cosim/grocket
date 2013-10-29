/**
 * @file libgrocketd/libgrocketd.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/09/24
 * @version $Revision$ 
 * @brief server frame static linkage version
 *
 * Revision History
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-09-24    Created.
 *       2     zouyueming   2013-10-21    add CoUninitialize for windows
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
#include <time.h>       // for time()
#include "gr_errno.h"
#include "gr_log.h"
#include "gr_global.h"
#include "gr_server_impl.h"
#include "gr_config.h"
#include "gr_module.h"
#include "gr_tools.h"
#include "gr_thread.h"
#include "gr_library_impl.h"
#if defined( _MSC_VER ) && defined( _DEBUG )
    #include <crtdbg.h>
#endif
#ifdef _DEBUG
#include "gr_library_invoke.h"
#endif

// 唯一的全局变量,没加static目的是方便 extern 关键字
// 加extern关键字的目的是性能，不必由于使用 static 关键字而付出取全局变量指针的函数调用时间
gr_global_t g_ghost_rocket_global;

static_inline
int system_init()
{
#if defined( WIN32 ) || defined( WIN64 )
	WSADATA wsa_data;
	int r;

    #if defined( _MSC_VER ) && defined( _DEBUG )
    _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );//| _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_CHECK_CRT_DF );
    #endif

    CoInitialize( NULL );

    // 加载 Winsock 2.2
    r = WSAStartup( 0x0202, & wsa_data );
    if ( 0 != r ) {
        gr_fatal( "[init]WSAStartup failed %d, GetLastError = %d", r, (int)GetLastError() );
        return GR_ERR_SYSTEM_CALL_FAILED;
    }
#endif

    gr_thread_init();

    return 0;
}

static_inline
void system_term()
{
    gr_thread_term();

#if defined( WIN32 ) || defined( WIN64 )
    WSACleanup();
    CoUninitialize();
#endif
}

static_inline
int setup_current_directory()
{
#if defined( WIN32 ) || defined( WIN64 )
    BOOL b;
#else
    int r;
#endif
    char path[ 260 ] = "";
    char * p;

    memset( path, 0, sizeof( path ) );
    get_exe_path( path, sizeof( path ) );
    if ( '\0' == path[ 0 ] ) {
        gr_fatal( "[init]get_exe_path failed" );
        return GR_ERR_SYSTEM_CALL_FAILED;
    }

    p = strrchr( path, S_PATH_SEP_C );
    if ( NULL == p ) {
        gr_fatal( "[init]%s not found %s", path, S_PATH_SEP );
        return GR_ERR_SYSTEM_CALL_FAILED;
    }

    * p = '\0';

#if defined( WIN32 ) || defined( WIN64 )
    b = SetCurrentDirectoryA( path );
    if ( ! b ) {
        gr_fatal( "[init]SetCurrentDirectoryA(%s) failed: %d", path, (int)GetLastError() );
        return GR_ERR_SYSTEM_CALL_FAILED;
    }
#else
    r = chdir( path );
    if ( 0 != r ) {
        gr_fatal( "[init]chdir(%s) return failed %d: %d,%s", path, r, errno, strerror(errno) );
        return GR_ERR_SYSTEM_CALL_FAILED;
    }
#endif

    //gr_info( "[init]Set current directory to %s", path );

    return GR_OK;
}

int
gr_main(
    int             argc,
    char **         argv,
    const char *    ini_content,
    size_t          ini_content_len,
    gr_version_t    version,
    gr_init_t       init,
    gr_term_t       term,
    gr_tcp_accept_t tcp_accept,
    gr_tcp_close_t  tcp_close,
    gr_check_t      chk_binary,
    gr_proc_t       proc_binary,
    gr_proc_http_t  proc_http
)
{
    // 注意：这个函数是父子进程都调用的代码
    int r = GR_OK;

    do {

        // 初始化全局唯一变量
        memset( & g_ghost_rocket_global, 0, sizeof(g_ghost_rocket_global) );
        // 默认从info开始打日志
        g_ghost_rocket_global.server_interface.log_level = GR_LOG_DEBUG; // GR_LOG_INFO;
        // argc, argv
        g_ghost_rocket_global.server_interface.argc = argc;
        g_ghost_rocket_global.server_interface.argv = argv;
        // 记录服务器启动时间
        g_ghost_rocket_global.server_interface.start_time = time( NULL );

        // 设置当前目录，方便模块取当前目录
        r = setup_current_directory();
        if ( 0 != r ) {
            gr_fatal( "[init]setup_current_directory return error %d", r );
            r = GR_ERR_SET_CURRENT_DIRECTORY_FAILED;
            break;
        }

        // 初始化系统相关的一些东西，比如Windows下的Socket库啦
        r = system_init();
        if ( 0 != r ) {
            gr_fatal( "[init]system_init return error %d", r );
            r = GR_ERR_SYSTEM_CALL_FAILED;
            break;
        }

        // 打开日志模块
        r = gr_log_open( NULL );
        if ( 0 != r ) {
            gr_fatal( "[init]gr_log_open() return error %d", r );
            r = GR_ERR_OPEN_LOG_FAILED;
            break;
        }

        gr_info( "==== grocket server version 1.%d(low compatible 1.%d) ====",
            GR_SERVER_VERSION, GR_SERVER_LOW_VERSION );

        gr_info( "[init]%d processor", gr_processor_count() );

#ifdef ENABLE_DEBUG_LOG
    #if defined( WIN32 ) || defined( WIN64 )
        #pragma message( "!!!!ENABLE_DEBUG_LOG defined, performance warning!" )
    #else
        #warning "!!!!ENABLE_DEBUG_LOG defined, performance warning!!!!"
    #endif
        gr_warning( "[init]!!!!ENABLE_DEBUG_LOG defined, performance warning!" );
#else
        gr_info( "[init]ENABLE_DEBUG_LOG not defined, "
                 "server debug log will not output" );
#endif

        // 初始化配置模块
        r = gr_config_init( ini_content, ini_content_len );
        if ( 0 != r ) {
            gr_fatal( "[init]gr_config_init return error %d", r );
            r = GR_ERR_INIT_CONFIG_FAILED;
            break;
        }

        // 初始化服务器函数库
        r = gr_library_impl_init();
        if ( 0 != r ) {
            gr_fatal( "[init]gr_library_impl_init return error %d", r );
            r = GR_ERR_INIT_LIBRARY_FAILED;
            break;
        }

        if ( gr_config_is_debug() ) {
#ifdef ENABLE_DEBUG_LOG
            gr_info( "[init]DEBUG model" );
#else
            gr_error( "[init]DEBUG model. but ENABLE_DEBUG_LOG not defined, "
                      "server debug log will not output" );
#endif
            g_ghost_rocket_global.server_interface.log_level = GR_LOG_DEBUG;
        } else {
            // 从配置文件里读取日志级别
            g_ghost_rocket_global.server_interface.log_level = (gr_log_level_t)
                gr_config_log_level( g_ghost_rocket_global.server_interface.log_level );
            if ( g_ghost_rocket_global.server_interface.log_level <= GR_LOG_DEBUG ) {
#ifndef ENABLE_DEBUG_LOG
                gr_error( "[init]log_level=%s. but ENABLE_DEBUG_LOG not defined, "
                          "server debug log will not output",
                          gr_log_level_2_str( g_ghost_rocket_global.server_interface.log_level ) );
#endif
            }
        }
        gr_info( "[init]LOG level = %s",
            gr_log_level_2_str( g_ghost_rocket_global.server_interface.log_level ) );

        // 初始化用户模块
        r = gr_module_init(
            version, init, term, tcp_accept, tcp_close, chk_binary, proc_binary, proc_http );
        if ( 0 != r ) {
            gr_fatal( "[init]gr_module_init return error %d", r );
            r = GR_ERR_INIT_MODULE_FAILED;
            break;
        }

        // 初始化服务器
        r = gr_server_init();
        if ( 0 == r ) {

            if ( gr_config_is_daemon() ) {
                // 以 daemon 模式运行服务器。
                r = gr_server_daemon_main();
            } else {
                // 以命令行模式运行服务器
                r = gr_server_console_main();
            }

        } else {
            gr_fatal( "[init]gr_server_init_config return error %d", r );
            r = GR_ERR_INIT_CONFIG_FAILED;
        }

    } while ( false );

    // 卸载服务器模块
    gr_server_term();
    // 卸载用户模块
    gr_module_term();
    // 卸载化服务器函数库
    gr_library_impl_term();
    // 卸载配置模块
    gr_config_term();
    // 关闭日志模块
    gr_log_close();
    // 卸载系统相关的初始化
    system_term();

    return r;
}
