/**
 * @file libgrocket/gr_server_impl_windows.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   server framework main function, for Windows
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

#include "gr_server_impl.h"
#include "gr_log.h"
#include "gr_global.h"
#include "gr_tools.h"
#include "gr_errno.h"
#include "gr_module.h"
#include "gr_config.h"

#if defined(WIN32) || defined(WIN64)

#include <Tlhelp32.h>

void do_close();

static_inline
bool
set_service_name(
    gr_server_impl_t * server,
    const char * name
)
{
    if ( NULL == name || '\0' == * name )
        return false;

    memset( server->service_name, 0, sizeof( server->service_name ) );
    strncpy( server->service_name, name, sizeof( server->service_name ) );
    if ( 0 != server->service_name[ sizeof( server->service_name ) - 1 ] ) {
        return false;
    }
    
    return true;
}

static_inline
bool
calc_service_name(
    gr_server_impl_t * server
)
{
    char ph[ MAX_PATH ] = "";
    char * p;
    char * name;

    get_exe_path( ph, sizeof( ph ) );

    name = strrchr( ph, '\\' );
    if ( NULL == name )
        name = ph;
    else
        ++ name;

    p = strrchr( name, '.' );
    if ( p ) {
        * p = '\0';
    }

    return set_service_name( server, name );
}

static_inline
bool service_report_status(
    DWORD dwCurrentState,
    DWORD dwWin32ExitCode,
    DWORD dwWaitHint
)
{
    static DWORD dwCheckPoint = 1;
    bool fResult = true;
    gr_server_impl_t * server = g_ghost_rocket_global.server;
    if ( NULL == server ) {
        gr_error( "global.server is NULL" );
        return false;
    }

    if (dwCurrentState == SERVICE_START_PENDING)
        server->service_status.dwControlsAccepted = 0;
    else
        server->service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    server->service_status.dwCurrentState = dwCurrentState;
    server->service_status.dwWin32ExitCode = dwWin32ExitCode;
    server->service_status.dwWaitHint = dwWaitHint;

    if (   dwCurrentState == SERVICE_RUNNING
        || dwCurrentState == SERVICE_STOPPED
    ) {
        server->service_status.dwCheckPoint = 0;
    } else {
        server->service_status.dwCheckPoint = ++ dwCheckPoint;
    }

    // Report the status of the service to the service control manager.
    //
    fResult = SetServiceStatus( server->status_handle, & server->service_status);

    return fResult;
}

static_inline
void
call_service_start(
    int argc,
    char ** argv
)
{
    #define START_WAIT_SECONDS  ( 60 * 5 )

    service_report_status( SERVICE_START_PENDING, NO_ERROR, 1000 * START_WAIT_SECONDS );

    service_report_status( SERVICE_RUNNING, NO_ERROR, 3000 );

    gr_server_console_main();

    service_report_status( SERVICE_STOPPED, NO_ERROR, 3000 );
}

static
void WINAPI
service_ctrl(
    DWORD ctrl_code
)
{
    gr_server_impl_t * server = g_ghost_rocket_global.server;
    if ( NULL == server ) {
        gr_error( "global.server is NULL" );
        return;
    }

    // Handle the requested control code.
    //
    switch(ctrl_code)
    {
    // Stop the service.
    //
	case SERVICE_CONTROL_SHUTDOWN: 
    case SERVICE_CONTROL_STOP:

        printf( "!!!!!! receive service stopping signal %d !!!!!!\n", ctrl_code );
        gr_info( "receive service stopping signal %d", (int)ctrl_code );
        do_close();
        service_report_status(SERVICE_STOP_PENDING, NO_ERROR, 0);
        return;

    // Update the service status.
    //
    case SERVICE_CONTROL_INTERROGATE:
        break;

    // invalid control code
    //
    default:
        break;
    }

    service_report_status( server->service_status.dwCurrentState, NO_ERROR, 0);
}

static
void WINAPI
service_main(
    DWORD argc,
    char ** argv
)
{
    gr_server_impl_t * server = g_ghost_rocket_global.server;
    if ( NULL == server ) {
        gr_error( "global.server is NULL" );
        return;
    }

    // register our service control handler:
    //
    server->status_handle = RegisterServiceCtrlHandlerA( server->service_name, service_ctrl );
    if ( ! server->status_handle ) {
        gr_error( "RegisterServiceCtrlHandlerA failed: %d", get_errno() );
        return;
    }

    // SERVICE_STATUS members that don't change in example
    server->service_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    server->service_status.dwServiceSpecificExitCode = 0;

    // report the status to the service control manager.
    //
    service_report_status(
        SERVICE_START_PENDING, // service state
        NO_ERROR,              // exit code
        3000
    );                 // wait hint

    call_service_start( argc, argv );

    service_report_status(
        SERVICE_STOPPED,
        server->service_err,
        0
    );
}

static_inline
int
install_service(
    gr_server_impl_t * server
)
{
    SC_HANDLE   sc_service;
    SC_HANDLE   sc_manager;
    char        path[ MAX_PATH ];

    if ( 0 == GetModuleFileNameA( NULL, path, sizeof( path ) ) ) {
        gr_error( "Call GetModuleFileNameA failed: %d", get_errno() );
        return - 500;
    }

    sc_manager = OpenSCManager(
        NULL,                   // machine (NULL == local)
        NULL,                   // database (NULL == default)
        SC_MANAGER_ALL_ACCESS   // access required
    );
    if ( sc_manager ) {
        DWORD start_type;

        start_type = SERVICE_AUTO_START;

        sc_service = CreateServiceA(
            sc_manager,               // SCManager database
            server->service_name,			    // name of service
            server->service_name,				// name to display
            SERVICE_ALL_ACCESS,         // desired access
            SERVICE_WIN32_OWN_PROCESS/* | SERVICE_INTERACTIVE_PROCESS*/,  // service type
            start_type,         // start type
            SERVICE_ERROR_NORMAL,       // error control type
            path,                     // service's binary
            NULL,                       // no load ordering group
            NULL,                       // no tag identifier
            "RpcSs\0\0\0",       // dependencies
            NULL,                       // LocalSystem account
            NULL);                      // no password

        if ( sc_service ) {
            printf( "%s installed.\n", server->service_name );
            gr_info( "Service %s installed", server->service_name );
            CloseServiceHandle(sc_service);
        } else {
            printf( "CreateService failed - %d\n", (int)GetLastError() );
            gr_error( "CreateService %s failed: %d", server->service_name, get_errno() );
            return -501;
        }

        CloseServiceHandle(sc_manager);
    } else {
        printf( "OpenSCManager failed - %d\n", (int)GetLastError() );
        gr_error( "OpenSCManager for %s failed: %d", server->service_name, get_errno() );
        return -502;
    }

    return 0;
}

static_inline
int
remove_service(
    gr_server_impl_t * server
)
{
    SC_HANDLE   sc_service;
    SC_HANDLE   sc_manager;

    sc_manager = OpenSCManager(
        NULL,                   // machine (NULL == local)
        NULL,                   // database (NULL == default)
        SC_MANAGER_ALL_ACCESS   // access required
    );
    if ( sc_manager ) {
        sc_service = OpenServiceA(sc_manager, server->service_name, SERVICE_ALL_ACCESS);

        if (sc_service) {
            // try to stop the service
            if ( ControlService( sc_service, SERVICE_CONTROL_STOP, & server->service_status ) ) {
                printf( "Stopping %s.", server->service_name );

                Sleep( 1000 );

                while( QueryServiceStatus( sc_service, & server->service_status ) ) {
                    if ( server->service_status.dwCurrentState == SERVICE_STOP_PENDING ) {
                        printf(".");
                        Sleep( 1000 );
                    } else {
                        break;
                    }
                }

                if ( server->service_status.dwCurrentState == SERVICE_STOPPED ) {
                    printf( "\n%s stopped.\n", server->service_name );
                    gr_info( "Service %s stopped", server->service_name );
                } else {
                    printf( "\n%s failed to stop.\n", server->service_name );
                    gr_error( "StopService %s failed: %d", server->service_name, get_errno() );
                }
            }

            // now remove the service
            if( DeleteService(sc_service) ) {
                printf( "%s removed.\n", server->service_name );
                gr_info( "Service %s removed", server->service_name );
            } else {
                printf( "DeleteService failed - %d\n", (int)GetLastError() );
                gr_error( "DeleteService %s failed: %d", server->service_name, get_errno() );
            }

            CloseServiceHandle(sc_service);
        } else {
            printf( "OpenService failed - %d\n", (int)GetLastError() );
            gr_error( "OpenService %s failed: %d", server->service_name, get_errno() );
        }

        CloseServiceHandle(sc_manager);
    } else {
        printf( "OpenSCManager failed - %s\n", (int)GetLastError() );
        gr_error( "OpenSCManager %s failed: %d", server->service_name, get_errno() );
    }

    return 0;
}

// Windows的daemon模式是以服务的方式启
int gr_server_daemon_main()
{
    int                 r = 0;
    gr_server_impl_t *  server;
    int                 argc;
    char **             argv;

    if ( NULL == g_ghost_rocket_global.server ) {
        gr_error( "global.server is NULL" );
        return GR_ERR_INVALID_PARAMS;
    }

    server  = g_ghost_rocket_global.server;
    argc    = g_ghost_rocket_global.server_interface.argc;
    argv    = g_ghost_rocket_global.server_interface.argv;

    if ( ! calc_service_name( server ) ) {
        gr_error( "calc_service_name failed" );
        return GR_ERR_UNKNOWN;
    }

    // fill service table
	server->service_table[ 0 ].lpServiceName = server->service_name;
	server->service_table[ 0 ].lpServiceProc = service_main;
	server->service_table[ 1 ].lpServiceName = NULL;
	server->service_table[ 1 ].lpServiceProc = NULL;

    // 
    if ( argc > 1 && ( 0 == strcmp( argv[ 1 ], "-install" ) || 0 == strcmp( argv[ 1 ], "install" ) ) ) {
		r = install_service( server );
	} else if ( argc > 1 && ( 0 == strcmp( argv[ 1 ], "-remove" ) || 0 == strcmp( argv[ 1 ], "remove" ) ) ) {
		r = remove_service( server );
    } else if ( argc > 1
        && ( 0 == strcmp( argv[ 1 ], "-run" ) || 0 == strcmp( argv[ 1 ], "run" ) )
        && ( 0 == strcmp( argv[ 1 ], "-debug" ) || 0 == strcmp( argv[ 1 ], "debug" ) )
    )
    {
        r = gr_server_console_main();

	} else {

		/*
		char user[ 128 ] = "";
		DWORD user_len = 128;

		// 如果没加参数运行，必须以 SYSTEM 用户运行
		if ( ! GetUserName( user, & user_len ) ) {
			printf( "无法取得当前用户" );
			return - __LINE__;
		}

		if ( 0 != strcmp( "SYSTEM", user ) ) {
			//printf( "必须以服务方式运行本程序" );
			return - __LINE__;
		}
		*/

		/*
		printf( "正试图以服务方式启动，如果要以命令行方式启动，请加 -run 参数...\n" );
		printf( "                      安装服务用 -install 参数\n" );
		printf( "                      删除服务用 -remove 参数\n" );
		*/

		if ( StartServiceCtrlDispatcherA( server->service_table ) ) {
			// 服务模式
			//printf( "Service Started.\n" );
            r = 0;
		} else {
            //printf( "try to Run on console mode...\n" );
            r = gr_server_console_main();
        }
	}

    return r;
}

static
BOOL WINAPI
dog_process_signal(
    DWORD ctrl_type
)
{
    gr_server_impl_t *  server;

    if ( NULL == g_ghost_rocket_global.server ) {
        gr_error( "global.server is NULL" );
        return FALSE;
    }
    server  = g_ghost_rocket_global.server;

    switch( ctrl_type )
    {
    case CTRL_BREAK_EVENT:      // A CTRL+C or CTRL+BREAK signal was
                                // received, either from keyboard input
                                // or from a signal generated by
                                // GenerateConsoleCtrlEvent.

    case CTRL_C_EVENT:          // SERVICE_CONTROL_STOP in debug mode or
                                // A CTRL+c signal was received, either
                                // from keyboard input or from a signal
                                // generated by the GenerateConsoleCtrlEvent
                                // function.

    case CTRL_CLOSE_EVENT:      // A signal that the system sends to all
                                // processes attached to a console when
                                // the user closes the console (either
                                // by choosing the Close command from
                                // the console window's System menu, or
                                // by choosing the End Task command from
                                // the Task List).

    case CTRL_SHUTDOWN_EVENT:   // A signal that the system sends to all
                                // console processes when the system is
                                // shutting down.

        printf( "!!!!!! receive Dog stopping signal %d !!!!!!\n", (int)ctrl_type );
        g_ghost_rocket_global.server_interface.is_server_stopping = true;
        return TRUE;

    default:
        printf( "!!!!!! receive unknown Dog signal %d !!!!!!\n", (int)ctrl_type );
        return FALSE;

    }
}

#define SUBPROC_SIGN    "{9F592725-613E-4fd9-AAD1-C31F533FA4E9}"

static_inline
void kill_sub_proc( gr_server_impl_t *  server )
{
	HANDLE ps = NULL;
    char exe_name[260] = "";
    const char* p;
    DWORD pid;
	PROCESSENTRY32 pe = {0};

    GetModuleFileNameA(NULL, exe_name, sizeof(exe_name));

    p = strrchr(exe_name, S_PATH_SEP_C);
    if (NULL != p)
    {
        strcpy(exe_name, p + 1);
        if ('\0' == exe_name[0])
        {
            return;
        }
    }

	ps = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
	if( INVALID_HANDLE_VALUE == ps )
        return;

    pid = GetCurrentProcessId();

	pe.dwSize = sizeof(PROCESSENTRY32);
	if( Process32First( ps, &pe ) ) {
		do {
			if( 0 == stricmp( exe_name, pe.szExeFile ) ) {
                if (pe.th32ProcessID != pid)
                {
				    HANDLE h = OpenProcess( PROCESS_TERMINATE, FALSE, pe.th32ProcessID );
				    TerminateProcess( h, 0 );
				    CloseHandle( h );
                }
				//break;
			}
		} while( Process32Next( ps, &pe) );
	}
	CloseHandle( ps );
}

static_inline
HINSTANCE create_sub_proc( gr_server_impl_t *  server )
{
    char cmd[MAX_PATH] = "";
    PROCESS_INFORMATION	pi;
    STARTUPINFOA si;

    char spid[ 16 ] = "";
    sprintf( spid, " %d", (int)GetProcessId( GetCurrentProcess() ) );

    GetModuleFileNameA(NULL, cmd, sizeof(cmd));
    strcat(cmd, " run ");
    strcat(cmd, SUBPROC_SIGN);
    strcat(cmd, spid );


    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );
	
    si.wShowWindow = SW_HIDE;
    si.dwFlags |= STARTF_USESHOWWINDOW;

    if ( ! CreateProcessA(
        NULL, // path[ 0 ] ? path : NULL,
        (LPSTR)cmd,
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &si,
        &pi ) )
    {
        return NULL;
    }

    CloseHandle( pi.hThread );

    WaitForInputIdle(pi.hProcess, INFINITE);

    return pi.hProcess;
}

static_inline
int dog_main( gr_server_impl_t *  server )
{
    HANDLE  h;
    int r = 0;
    DWORD sub_proc_id;

    SetConsoleCtrlHandler( dog_process_signal, TRUE );

    kill_sub_proc( server );

    r =  gr_module_master_process_init();
    if ( 0 == r ) {

        while( ! g_ghost_rocket_global.server_interface.is_server_stopping )
        {
            h = create_sub_proc( server );
            if ( NULL == h ) {
                Sleep( 1000 );
                continue;
            }

            sub_proc_id = GetProcessId( h );

            while( ! g_ghost_rocket_global.server_interface.is_server_stopping ) {

                DWORD r = WaitForSingleObject( h, 1000 );
                if ( WAIT_OBJECT_0 == r ) {
                    TerminateProcess( h, 0 );
                    CloseHandle( h );
                    h = NULL;

                    break;
                }
            }
        }

        /*
        if ( h ) {
            // 退出前把子进程退了
            TerminateProcess( h, 0 );
            WaitForSingleObject( h, INFINITE );
            CloseHandle( h );
            h = NULL;
        }
        */

        gr_module_master_process_term();

    } else {
        gr_fatal( "gr_module_master_process_init return %d", r );
    }

    SetConsoleCtrlHandler( dog_process_signal, FALSE );

    return r;
}

// Windows默认就是两个进程,因为不支持fork,所以只好这样
int gr_server_console_main()
{
    bool                is_dog = true;
    gr_server_impl_t *  server;
    int                 argc;
    char **             argv;

    if ( NULL == g_ghost_rocket_global.server ) {
        gr_error( "global.server is NULL" );
        return GR_ERR_INVALID_PARAMS;
    }
    server  = g_ghost_rocket_global.server;

    argc = g_ghost_rocket_global.server_interface.argc;
    argv = g_ghost_rocket_global.server_interface.argv;

    if (   argc > 1
        && ( 0 == strcmp( argv[ 1 ], "-debug" ) || 0 == strcmp( argv[ 1 ], "debug" ) ) )
    {
        // 有debug参数，则不启看门狗进程
        is_dog = false;
    } else if ( gr_config_is_debug() ) {
        // debug模式，则不启看门狗进程
        is_dog = false;
    } else {
        int i;
        char* cmdline;
        char* p;
        for ( i = 1; i < argc; ++ i)
        {
            if (0 == strcmp(SUBPROC_SIGN, argv[i]))
            {
                is_dog = false;
                // 清了防止别人看到
                memset(argv[i], 0, sizeof(SUBPROC_SIGN)-1);
                // 把Widnows的命令行也清了
                cmdline = GetCommandLineA();
                p = strstr(cmdline, SUBPROC_SIGN);
                if (NULL != p)
                {
                    memset(p, 0, sizeof(SUBPROC_SIGN)-1);
                }

                if ( argc > i + 1 ) {
                    // 记录父进程ID
                    int idx = i + 1;
                    const char * sppid = argv[ idx ];
                    if ( * sppid > 0 && isdigit( * sppid ) ) {
                        int ppid = atoi( sppid );
                        server->parent_process = OpenProcess( PROCESS_QUERY_INFORMATION, FALSE, ppid );
                    }
                }

                break;
            }
        }
    }

    if ( is_dog ) {
        return dog_main( server );
    } else {
        return gr_server_main( server );
    }
}

#endif // #if defined(WIN32) || defined(WIN64)
