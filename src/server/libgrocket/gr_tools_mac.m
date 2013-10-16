/**
 * @file libgrocket/gr_tools_mac.m
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/07
 * @version $Revision$ 
 * @brief   generic tool. for Mac OS X
 * Revision History
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-07    Created.
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

#import <Foundation/Foundation.h>

size_t
get_exe_path(
    char * path,
    size_t  path_len
)
{
    static char g_path[ MAX_PATH ] = "";
    static size_t g_path_len = 0;
    NSAutoreleasePool * pool;
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

    pool = [[NSAutoreleasePool alloc] init];
    s = [[NSBundle mainBundle] executablePath];

    len = 0;

    do {

        if ( nil == pool || nil == s ) {
            break;
        }

        p = (const char *)[s cStringUsingEncoding: NSUTF8StringEncoding];
        if ( NULL == p || '\0' == * p ) {
            break;
        }
        
        len = strlen( p );
        if ( len >= path_len ) {
            len = 0;
            break;
        }
        
        strcpy( path, p );

        strcpy( g_path, path );
        g_path_len = len;

    } while( 0 );

    if ( nil != pool ) {
        [pool release];
    }
        
    return len;
}

#endif // #if defined( __APPLE__ )
