/**
 * @file include/gr_server_impl.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   ����������
 * Revision History ���¼���
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-05    Created.
 **/
#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_SERVER_IMPL_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_SERVER_IMPL_H_

#include "gr_stdinc.h"
#include "grocket.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int                     argc;
    char **                 argv;
#if defined( WIN32 ) || defined( WIN64 )
    char                    service_name[ 64 ];
	SERVICE_TABLE_ENTRYA    service_table[ 2 ];
	SERVICE_STATUS          service_status;
	SERVICE_STATUS_HANDLE   status_handle;
	DWORD                   service_err;

    // ����и����̣��������¼�����̾��������ΪNULL
    HANDLE                  parent_process;

    HINSTANCE               funcs_dll;

#else

    void *                  funcs_dll;

#endif

    volatile bool           is_server_stopping;

    //volatile bool           is_tcp_disabled;

    /*

    struct Mantain *        mantain;

    struct Log *            log;

    struct Connections *    connections;

    struct Http *           http;

    struct Io *             io;

    struct MemoryPool *     memory;

    struct Module *         module;

    struct Workers *        workers;
    */

    // ����������ʱ��
    time_t                  start_time;

} gr_server_impl_t;

int
gr_server_init(
    int             argc,
    char **         argv
);

int gr_server_daemon_main();

int gr_server_console_main();

int gr_server_main();

void gr_server_term();

void gr_server_need_exit( gr_server_impl_t * server );

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_SERVER_IMPL_H_
