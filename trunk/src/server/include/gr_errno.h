/**
 * @file include/gr_errno.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   error code
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

#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_ERRNO_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_ERRNO_H_

#include "gr_compiler_switch.h"

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    GR_OK                               = 0,

    GR_ERR_UNKNOWN                      = -1,

    GR_ERR_INVALID_PARAMS               = -2,

    GR_ERR_SYSTEM_CALL_FAILED           = -3,

    GR_ERR_WRONG_CALL_ORDER             = -4,

    GR_ERR_NOT_FOUND                    = -5,

    GR_ERR_WRONG_VERSION                = -6,

    GR_ERR_BAD_ALLOC                    = -10,

    GR_ERR_CREATE_THREAD_FALED          = -11,

    GR_ERR_OPEN_LOG_FAILED              = -1000,

    GR_ERR_SET_CURRENT_DIRECTORY_FAILED = -1001,

    GR_ERR_INIT_CONFIG_FAILED           = -1002,

    GR_ERR_INIT_MODULE_FAILED           = -1003,

    GR_ERR_INIT_TCP_ACCEPT_FALED        = -1004,

    GR_ERR_INIT_TCP_IN_FALED            = -1005,

    GR_ERR_INIT_UDP_IN_FALED            = -1006,

    GR_ERR_INIT_TCP_OUT_FALED           = -1007,

    GR_ERR_INIT_UDP_OUT_FALED           = -1008,

    GR_ERR_INIT_WORKER_FALED            = -1009,

    GR_ERR_INIT_HTTP_FALED              = -1010,

    GR_ERR_INIT_CONN_FALED              = -1011,

    GR_ERR_INIT_POLL_FALED              = -1012,

    GR_ERR_INIT_LIBRARY_FAILED          = -1013,

    GR_ERR_INIT_TCP_CLOSE_FALED         = -1014,
};

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_ERRNO_H_
