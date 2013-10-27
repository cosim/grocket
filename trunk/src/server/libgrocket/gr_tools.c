/**
 * @file libgrocket/gr_tools.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   generic tool
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

#include "gr_tools.h"
#include "gr_errno.h"
#include "gr_log.h"
#include "gr_global.h"
#if defined( __APPLE__ )
    #include <mach/mach_time.h>
#endif

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

uint32_t get_tick_count()
{
#if defined( WIN32 ) || defined( WIN64 )

    return GetTickCount();

#elif defined( __linux )

    struct timespec ts;
    if ( 0 != clock_gettime( CLOCK_MONOTONIC, & ts ) ) {
        gr_error( "clock_gettime failed %d", get_errno() );
        return (uint32_t)0;
    }

    // 这个折返的机率和GetTickCount是一样的
    return (uint32_t)( ts.tv_sec * 1000L + ts.tv_nsec / 1000000L ); 

#elif defined( __APPLE__ )

    /*
    mach_absolute_time函数返回的值是启动后系统CPU/Bus的clock一个tick数
    因为这个 GetTickCount 是系统启动后的毫秒数，所以要获得系统启动后的时间需要
    进行一次转换，还好Apple给出了一个官方的方法
    Technical Q&A QA1398
    Mach Absolute Time Units
    https://developer.apple.com/library/mac/#qa/qa1398/_index.html
    另外一些关于这个函数的说明文档如下：
    http://www.macresearch.org/tutorial_performance_and_time
        mach_absolute_time is a CPU/Bus dependent function that returns a value
    based on the number of "ticks" since the system started up.
    uint64_t mach_absolute_time(void);
        Declared In: <mach/mach_time.h>
        Dependency: com.apple.kernel.mach
        This function returns a Mach absolute time value for the current wall
    clock time in units of uint64_t.
    https://developer.apple.com/library/mac/#documentation/Performance/Conceptual/LaunchTime/Articles/MeasuringLaunch.html
        mach_absolute_time reads the CPU time base register and is the basis
    for other time measurement functions.
    */
    static mach_timebase_info_data_t g_info;

    if ( 0 == g_info.denom ) {
        if ( KERN_SUCCESS != mach_timebase_info( & g_info ) ) {
            gr_error( "mach_timebase_info failed %d", get_errno() );
            return (uint32_t)0;
        }
    }

    // 这个折返的机率和GetTickCount是一样的，只要乘的时候别溢出就行
    return (uint32_t)( mach_absolute_time() * g_info.numer / g_info.denom / 1000000L );

#else

    // 这个应该是速度最慢的
    struct timeval tv;
    if ( 0 != gettimeofday( & tv, NULL ) ) {
        gr_error( "gettimeofday failed %d", get_errno() );
        return (uint32_t)0;
    }

    //TODO: 折返了怎么办？
    return (uint32_t)( tv.tv_sec * 1000L + tv.tv_usec / 1000L );    
#endif
}
