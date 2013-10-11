/**
 * @file include/gr_log.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   日志
 * Revision History 大事件记
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-05    Created.
 **/
#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_LOG_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_LOG_H_

#include "gr_stdinc.h"
#include "grocket.h"

#ifdef __cplusplus
extern "C" {
#endif

int gr_log_open();

void gr_log_close();

void gr_log_write(
    const char *    file,
    int             line,
    const char *    func,
    gr_log_level_t  level,
    const char * fmt,
    ...
);

#define gr_fatal(str, ...)      if ( g_ghost_rocket_global.log_start_level <= GR_LOG_FATAL )   { gr_log_write( __FILE__, __LINE__, __FUNCTION__, GR_LOG_FATAL,   str, ##__VA_ARGS__); }
#define gr_error(str, ...)      if ( g_ghost_rocket_global.log_start_level <= GR_LOG_ERROR )   { gr_log_write( __FILE__, __LINE__, __FUNCTION__, GR_LOG_ERROR,   str, ##__VA_ARGS__); }
#define gr_warning(str, ...)    if ( g_ghost_rocket_global.log_start_level <= GR_LOG_WARNING ) { gr_log_write( __FILE__, __LINE__, __FUNCTION__, GR_LOG_WARNING, str, ##__VA_ARGS__); }
#define gr_info(str, ...)       if ( g_ghost_rocket_global.log_start_level <= GR_LOG_INFO )    { gr_log_write( __FILE__, __LINE__, __FUNCTION__, GR_LOG_INFO,    str, ##__VA_ARGS__); }
#define gr_debug(str, ...)      if ( g_ghost_rocket_global.log_start_level <= GR_LOG_DEBUG )   { gr_log_write( __FILE__, __LINE__, __FUNCTION__, GR_LOG_DEBUG,   str, ##__VA_ARGS__); }

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_LOG_H_
