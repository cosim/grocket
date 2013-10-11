/**
 * @file libgrocket/gr_log.c
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
#include "gr_log.h"
#include "gr_global.h"

int gr_log_open()
{
    return 0;
}

void gr_log_close()
{
}

void gr_log_write(
    const char *    file,
    int             line,
    const char *    func,
    gr_log_level_t  level,
    const char * fmt,
    ...
)
{
    char buf[ 1024 ];
    const char * p;
    const char * level_s;
    va_list ap;

    if ( GR_LOG_FATAL == level )
        level_s = "fatal  ";
    else if ( GR_LOG_ERROR == level )
        level_s = "error  ";
    else if ( GR_LOG_WARNING == level )
        level_s = "warning";
    else if ( GR_LOG_INFO == level )
        level_s = "info   ";
    else if ( GR_LOG_DEBUG == level )
        level_s = "debug  ";
    else
        level_s = "unknown";

    p = strrchr( file, S_PATH_SEP_C );
    if ( p )
        file = p + 1;

    va_start( ap, fmt );

    vsnprintf( buf, sizeof( buf ), fmt, ap );

    va_end ( ap );

    buf[ sizeof( buf ) - 1 ] = '\0';

    printf( "%s %s (%s:%d:%s)\n", level_s, buf, file, (int)line, func );
}
