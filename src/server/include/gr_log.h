/**
 * @file include/gr_log.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   log
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

#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_LOG_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_LOG_H_

#include "gr_stdinc.h"
#include "gr_compiler_switch.h"
#include "grocket.h"

#ifdef __cplusplus
extern "C" {
#endif

int gr_log_open( const char * log_name );

void gr_log_close();

void gr_log_on_config_ready();

void gr_log_write(
    const char *    file,
    int             line,
    const char *    func,
    gr_log_level_t  level,
    bool            is_user_log,
    const char * fmt,
    ...
);

const char *
gr_log_level_2_str(
    gr_log_level_t  level
);


#define gr_fatal(str, ...)      if ( g_ghost_rocket_global.server_interface.log_level <= GR_LOG_FATAL )   { gr_log_write( __FILE__, __LINE__, __FUNCTION__, GR_LOG_FATAL,   false, str, ##__VA_ARGS__); }
#define gr_error(str, ...)      if ( g_ghost_rocket_global.server_interface.log_level <= GR_LOG_ERROR )   { gr_log_write( __FILE__, __LINE__, __FUNCTION__, GR_LOG_ERROR,   false, str, ##__VA_ARGS__); }
#define gr_warning(str, ...)    if ( g_ghost_rocket_global.server_interface.log_level <= GR_LOG_WARNING ) { gr_log_write( __FILE__, __LINE__, __FUNCTION__, GR_LOG_WARNING, false, str, ##__VA_ARGS__); }
#define gr_info(str, ...)       if ( g_ghost_rocket_global.server_interface.log_level <= GR_LOG_INFO )    { gr_log_write( __FILE__, __LINE__, __FUNCTION__, GR_LOG_INFO,    false, str, ##__VA_ARGS__); }
#ifdef ENABLE_DEBUG_LOG
#define gr_debug(str, ...)      if ( g_ghost_rocket_global.server_interface.log_level <= GR_LOG_DEBUG )   { gr_log_write( __FILE__, __LINE__, __FUNCTION__, GR_LOG_DEBUG,   false, str, ##__VA_ARGS__); }
#else
#define gr_debug(str, ...)
#endif

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_LOG_H_
