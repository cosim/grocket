/**
 * @file include/gr_server_impl.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   server framework main function
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

#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_SERVER_IMPL_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_SERVER_IMPL_H_

#include "gr_stdinc.h"
#include "gr_compiler_switch.h"
#include "grocket.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
#if defined( WIN32 ) || defined( WIN64 )
    char                    service_name[ 64 ];
	SERVICE_TABLE_ENTRYA    service_table[ 2 ];
	SERVICE_STATUS          service_status;
	SERVICE_STATUS_HANDLE   status_handle;
	DWORD                   service_err;

    // 如果有父进程，则这里记录父进程句柄，否则为NULL
    HANDLE                  parent_process;

#endif

    //volatile bool           is_tcp_disabled;

} gr_server_impl_t;

int gr_server_init();

int gr_server_daemon_main();

int gr_server_console_main();

int gr_server_main();

void gr_server_term();

void gr_server_need_exit( gr_server_impl_t * server );

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_SERVER_IMPL_H_
