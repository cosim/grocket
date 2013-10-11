/**
 * @file libgrocket/gr_tools_mac.m
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/07
 * @version $Revision$ 
 * @brief   Mac平台通用工具函数
 * Revision History 大事件记
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-07    Created.
 **/
#include "gr_tools.h"
#include "gr_errno.h"
#include "gr_log.h"
#include "gr_global.h"

#if defined( __APPLE__ )

#import <Foundation/Foundation.h>

size_t
get_exe_path(
    char * path,
    size_t  path_len
)
{
    static char g_path[ MAX_PATH ] = "";
    static size_t g_path_len = 0;
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    const char * p;
    NSString * s = nil;
    size_t len;
        
    if ( '\0' != g_path[ 0 ] ) {
        if ( path_len <= g_path_len ) {
            return 0;
        }
        memcpy( path, g_path, g_path_len );
        path[ g_path_len ] = '\0';
        return g_path_len;
    }

    s = [[NSBundle mainBundle] executablePath];

    len = 0;

    do {

        if ( nil == s )
            break;

        p = (const char *)[s cStringUsingEncoding: NSUTF8StringEncoding];
        if ( NULL == p || '\0' == * p )
            break;
        
        len = strlen( p );
        if ( len >= path_len ) {
            len = 0;
            break;
        }
        
        strcpy( path, p );

        strcpy( g_path, path );
        g_path_len = len;

    } while( 0 );

    [pool release];
        
    return len;
}

#endif // #if defined( __APPLE__ )
