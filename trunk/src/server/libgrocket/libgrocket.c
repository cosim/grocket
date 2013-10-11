/**
 * @file libgrocketd/libgrocketd.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/09/24
 * @version $Revision$ 
 * @brief 服务框架提供的静态库版本。
 *        该版本适用于必须使用定制的可执行程序的场景。
 *
 * Revision History 大事件记
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-09-24    Created.
 **/
#include "libgrocket.h"
#include "gr_errno.h"
#include "gr_log.h"
#include "gr_global.h"
#include "gr_server_impl.h"
#include "gr_config.h"
#include "gr_module.h"
#include "gr_tools.h"
#include "gr_library_impl.h"
#if defined( _MSC_VER ) && defined( _DEBUG )
    #include <crtdbg.h>
#endif
#ifdef _DEBUG
#include "gr_library_invoke.h"
#endif

// 唯一的全局变量,没加static目的是方便 extern 关键字
// 加extern关键字的目的是性能，不必付出函数调用时间
gr_global_t g_ghost_rocket_global;

static inline
int system_init()
{
#if defined( WIN32 ) || defined( WIN64 )
	WSADATA wsa_data;
	DWORD ret;

    #if defined( _MSC_VER ) && defined( _DEBUG )
    _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );//| _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_CHECK_CRT_DF );
    #endif

    CoInitialize( NULL );

    // 加载 Winsock 2.2
    if ( ( ret = WSAStartup( 0x0202, & wsa_data ) ) != 0 ) {
        gr_fatal( "[init]WSAStartup failed, GetLastError = %d", (int)GetLastError() );
        return GR_ERR_SYSTEM_CALL_FAILED;
    }
#endif

    return 0;
}

static inline
void system_term()
{
#if defined( WIN32 ) || defined( WIN64 )
    WSACleanup();
#endif
}

static inline
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

    gr_info( "[init]Set current directory to %s", path );

    return GR_OK;
}

int
gr_main(
    int             argc,
    char **         argv,
    const char *    ini_content,
    size_t          ini_content_len,
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
        g_ghost_rocket_global.log_start_level = GR_LOG_DEBUG; // GR_LOG_INFO;
        // argc, argv
        g_ghost_rocket_global.server_interface.argc = argc;
        g_ghost_rocket_global.server_interface.argv = argv;

        // 初始化系统相关的一些东西，比如Windows下的Socket库啦
        r = system_init();
        if ( 0 != r ) {
            gr_fatal( "[init]system_init return error %d", r );
            r = GR_ERR_SYSTEM_CALL_FAILED;
            break;
        }

        // 打开日志模块
        r = gr_log_open();
        if ( 0 != r ) {
            gr_fatal( "[init]gr_log_open() return error %d", r );
            r = GR_ERR_OPEN_LOG_FAILED;
            break;
        }

        // 设置当前目录，方便模块取当前目录是正确的
        r = setup_current_directory();
        if ( 0 != r ) {
            gr_fatal( "[init]setup_current_directory return error %d", r );
            r = GR_ERR_SET_CURRENT_DIRECTORY_FAILED;
            break;
        }

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

        // 从配置文件里读取日志级别
        g_ghost_rocket_global.log_start_level = (gr_log_level_t)
            gr_config_log_level( g_ghost_rocket_global.log_start_level );

        // 初始化用户模块
        r = gr_module_init(
            init, term, tcp_accept, tcp_close, chk_binary, proc_binary, proc_http );
        if ( 0 != r ) {
            gr_fatal( "[init]gr_module_init return error %d", r );
            r = GR_ERR_INIT_MODULE_FAILED;
            break;
        }

        // 初始化服务器
        r = gr_server_init( argc, argv );
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
