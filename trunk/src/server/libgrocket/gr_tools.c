/**
 * @file libgrocket/gr_tools.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   通用工具函数
 * Revision History 大事件记
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-05    Created.
 **/
#include "gr_tools.h"
#include "gr_errno.h"
#include "gr_log.h"
#include "gr_global.h"

#if ! defined( __APPLE__ )

size_t
get_exe_path(
    char * path,
    size_t  path_len
)
{
#if defined(WIN32) || defined(WIN64)
    DWORD r;
    assert( path && path_len );
    r = GetModuleFileNameA( NULL, path, (DWORD)path_len );
    if ( ERROR_INSUFFICIENT_BUFFER == GetLastError() || (size_t)r >= path_len ) {
        gr_fatal( "GetModuleFileNameA failed, %d", (int)GetLastError() );
        path[ 0 ] = 0;
        return 0;
    }
    path[ r ] = '\0';
    return (size_t)r;
#elif defined( __linux )
    ssize_t r;
    assert( path && path_len );
    r = readlink( "/proc/self/exe", path, path_len );
    if ( r <= 0 ) {
        gr_fatal( "readlink failed %d,%s", errno, strerror(errno) );
        path[ 0 ] = 0;
        return 0;
    }
    if ( (unsigned long)r >= path_len ) {
        gr_fatal( "invalid params" );
        path[ 0 ] = 0;
        return 0;
    }
    path[ r ] = '\0';
    return r;
#else
    #error unknown platform
#endif
}

#endif // #if ! defined( __APPLE__ )

char *
str_trim(
    char * s,
    size_t * len
)
{
    char * t;
    char   c;
    size_t  n;
        
    assert( s );
    if ( len )
        n = * len;
    else
        n = strlen( s );
        
    // de pre space
    while ( *s && *s > 0 && isspace( *s ) ) {
        ++ s;
        -- n;
    };
        
    // de tail space
    if ( n ) {
        t = s + n;
        while( ( c = * (t-1) ) > 0 && isspace( c ) ) {
            -- t;
            -- n;
        };
        * t = '\0';
    }
        
    if ( len )
        * len = n;
            
    //assert( strlen( s ) == n );
    return s;
}

void
path_to_os(
    char * path
)
{
    if ( path ) {
        while ( * path ) {
            if ( '/' == * path || '\\' == * path )
                * path = S_PATH_SEP_C;
            ++ path;
        }
    }
}

int
get_errno()
{
#if defined( WIN32 ) || defined( WIN64 )
    return (int)GetLastError();
#else
    return errno;
#endif
}

void
sleep_ms(
    uint32_t ms
)
{
#if defined( WIN32 ) || defined( WIN64 )
    Sleep( ms );
#else
    // 以秒为单位等
    if ( ms >= 1000 ) {
        sleep( ms / 1000 );
        ms %= 1000;
    }
    // 以毫秒为单位等
    if ( ms > 0 ) {
        struct timespec interval;
        struct timespec remainder;
        interval.tv_sec = 0;
        interval.tv_nsec = ms * 1000000;
        nanosleep( & interval, & remainder );
    }
#endif
}

bool
is_exists(
    const char * path
)
{
#if defined( WIN32 ) || defined( WIN64 )
    return INVALID_FILE_ATTRIBUTES != GetFileAttributesA( path );
#else
    return 0 == access( path, R_OK );
#endif
}
