/**
 * @file libgrocket/gr_server_impl_posix.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   服务器外围工作
 * Revision History 大事件记
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-05    Created.
 **/
#include "gr_server_impl.h"
#include "gr_log.h"
#include "gr_global.h"
#include "gr_tools.h"
#include "gr_errno.h"
#include "gr_module.h"
#include <signal.h>

#if ! defined(WIN32) && ! defined(WIN64)

static inline
bool run_back()
{
    int flag;
    flag = fork();
    if( flag < 0 ) {
        //output_log( "error", "%s:%d fork failed: %d", __FILE__, __LINE__, errno );
        return false;
    } else if( flag > 0 ) {
        exit( 0 );
        return true;
    }
    return true;
}

static
void
process_parent_signal(
    int sig
)
{
    gr_server_impl_t *  server;

    signal( sig, process_parent_signal );

    printf( "!!!!!! receive monitor stopping signal %d !!!!!!\n", sig );
    gr_info( "receive monitor stopping signal %d", sig );

    if ( NULL != g_ghost_rocket_global.server ) {
        server = g_ghost_rocket_global.server;
        server->is_server_stopping = true;
    }
}

int
gr_server_daemon_main()
{
    pid_t exit_pid = 0;
    pid_t pid = -1;
    int   exit_status = 0;
    int   r = 0;
    gr_server_impl_t *  server;

    if ( NULL == g_ghost_rocket_global.server ) {
        gr_error( "global.server is NULL" );
        return GR_ERR_INVALID_PARAMS;
    }
    server  = g_ghost_rocket_global.server;

    if ( server->argc <= 1 
        || ( 0 != strcmp( server->argv[ 1 ], "-debug" ) && 0 != strcmp( server->argv[ 1 ], "debug" ) ) )
    {
        // 不是调试模式才把进程扔到后台。
        run_back();
    }

    // 不允许随便断，因为master_process_init初始化的可能是数据操作，
    // 回头给所有子进程共享的，如果这儿断了可能会破坏数据。
    signal( SIGHUP, SIG_IGN );
    signal( SIGINT, SIG_IGN );
    signal( SIGQUIT, SIG_IGN );
    signal( SIGTERM, SIG_IGN );
    signal( SIGIOT, SIG_IGN );
    signal( SIGPIPE, SIG_IGN );
    signal( SIGTSTP, SIG_IGN );
    signal( SIGCHLD, SIG_IGN );
    signal( SIGXCPU, SIG_IGN );
    signal( SIGXFSZ, SIG_IGN );

    r =  gr_module_master_process_init();
    if ( 0 == r ) {

        server->is_server_stopping = 0;

        while( ! server->is_server_stopping ) {

            pid = fork();
            if ( 0 == pid ) {

                // 子进程
                //prctl(PR_SET_NAME, "Server" );

                r = gr_server_console_main();
                break;

            } else {

                // 父进程
                //prctl( PR_SET_NAME, "Dog" );

                signal( SIGHUP, SIG_IGN );

                signal( SIGINT, process_parent_signal );
                signal( SIGQUIT, process_parent_signal );
                signal( SIGTERM, process_parent_signal );

                signal( SIGIOT, SIG_IGN );
                signal( SIGPIPE, SIG_IGN );
                signal( SIGTSTP, SIG_IGN );
                signal( SIGCHLD, SIG_IGN );
                signal( SIGXCPU, SIG_IGN );
                signal( SIGXFSZ, SIG_IGN );

                r = 0;

                exit_pid = wait( & exit_status );

                if ( server->is_server_stopping )
                    break;

                //sleep( 2 ); // 等待两秒，防止程序bug导致频繁故障, 如果是core，可能会瞬间写满硬盘
            }
        }

        if ( 0 != pid ) {
            gr_module_master_process_term();
            //printf( "Monitor exit\n" );
        } else {
            //printf( "Server exit\n" );
        }
    } else {
        gr_fatal( "gr_module_master_process_init return %d", r );
    }

    return r;
}

int gr_server_console_main()
{
    gr_server_impl_t *  server;
    int                 r;

    if ( NULL == g_ghost_rocket_global.server ) {
        gr_error( "global.server is NULL" );
        return GR_ERR_INVALID_PARAMS;
    }
    server  = g_ghost_rocket_global.server;

    r = gr_server_main( server );
}

#endif // #if defined(__linux)
