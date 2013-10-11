/**
 * @file libgrocket/gr_dll.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   dll相关操作
 * Revision History 大事件记
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-05    Created.
 **/
#include "gr_dll.h"
#include "gr_log.h"
#include "gr_global.h"
#include "gr_tools.h"

gr_dll_t
gr_dll_open_absolute(
    const char * path
)
{
#if defined( WIN32 ) || defined( WIN64 )
    HINSTANCE h;
    assert( path && * path );
    h = LoadLibraryA( path );
    if ( NULL != h )
        return h;

    gr_error( "LoadLibraryA %s failed: %d", path, get_errno() );
    return NULL;
#else
    int flags = RTLD_NOW;
    gr_dll_t h;
    assert( path && * path );
#ifdef _AIX
    flags |= RTLD_MEMBER;
#endif
    h = dlopen( path, flags );
    if ( NULL != h )
        return h;

    gr_error( "dlopen %s failed: %d,%s", path, get_errno(), dlerror() );
    return NULL;
#endif
}

gr_dll_t
gr_dll_open(
    const char * s
)
{
    char path[ MAX_PATH ];
    char ofile[ MAX_PATH ];
    char * ver = NULL;
    char file[ MAX_PATH ];
    char * p;
    size_t plen;
    size_t slen;
    size_t ofile_len;
    size_t ver_len;
    size_t file_len;
    gr_dll_t h;

    if ( NULL == s || '\0' == * s ) {
        gr_error( "invalid params" );
        return NULL;
    }

    slen = strlen( s );
    if ( slen >= COUNT_OF(path) ) {
        gr_error( "invalid params" );
        return NULL;
    }
        
    strncpy( path, s, slen );
    path[ slen ] = '\0';
    path_to_os( path );

    p = strrchr( path, S_PATH_SEP_C );
    if ( NULL == p ) {
        ofile_len = slen;
        strncpy( ofile, path, ofile_len );
        ofile[ ofile_len ] = '\0';
        
        path[ 0 ] = '\0';
        
    } else {
        ofile_len = strlen( p + 1 );
        strncpy( ofile, p + 1, ofile_len );
        ofile[ ofile_len ] = '\0';

#if defined( WIN32 ) || defined( WIN64 )
        if ( p == path || ':' == * (p - 1) )
#else
        if ( p == path )
#endif
            * (p + 1) = '\0';
        else
            * p = '\0';

        * p = S_PATH_SEP_C;
        * (p + 1) = '\0';
    }

    // , follow version
    ver = strrchr( ofile, ',' );
    if ( ver ) {
        * ver ++ = '\0';
        ofile_len = strlen( ofile );
        
        ver_len = strlen( ver );
    } else {
        ver_len = 0;
    }

    p = NULL;

#if defined( WIN32 ) || defined( WIN64 )
    strncpy( file, ofile, ofile_len );
    file[ ofile_len ] = '\0';
    if ( ver ) {
        strcat( file, "." );
        strcat( file, ver );
    }
    p = ".dll";
#elif defined(__APPLE__)
    strcpy( file, "lib" );
    strcat( file, ofile );
    if ( ver ) {
        strcat( file, "." );
        strcat( file, ver );
    }
    p = (char*)".dylib";
#elif defined(__hpux)
    strcpy( file, "lib" );
    strcat( file, ofile );
    if ( ver ) {
        strcat( file, "." );
        strcat( file, ver );
    }
    p = ".sl";
#elif defined(_AIX)
    strcpy( file, "lib" );
    strcat( file, ofile );
    strcat( file, ".a(lib" );
    strcat( file, ofile );
    strcat( file, ".so" );
    if ( ver ) {
        strcat( file, "." );
        strcat( file, ver );
    }
    strcat( file, ")" );
    p = NULL;
#else
    strcpy( file, "lib" );
    strcat( file, ofile );
    strcat( file, ".so" );
    if ( ver ) {
        strcat( file, "." );
        strcat( file, ver );
    }
    p = NULL;
#endif

    file_len = strlen( file );
    if ( p ) {
        
        bool need_cat = true;
        
        plen = strlen( p );
        
        // append p, if needed
        
        if ( file_len >= plen ) {
#if defined( WIN32 ) || defined( WIN64 )
            if ( 0 == strnicmp( p, file + file_len - plen, plen ) ) {
#else
            if ( 0 == strncmp( p, file + file_len - plen, plen ) ) {
#endif
                need_cat = false;
            }
        }
            
        if ( need_cat ) {
            strcat( file, p );
            file_len += plen;
        }

    } else {
        plen = 0;
    }
    strcat( path, file );

    h = gr_dll_open_absolute( path );
    if ( h )
        return h;

    gr_error( "dll_open_inner %s failed", path );
    return NULL;
}

void
gr_dll_close(
    gr_dll_t h
)
{    
    if (NULL == h) {
        gr_error( "invalid params" );
        return;
    }

#if defined( WIN32 ) || defined( WIN64 )
    FreeLibrary( h );
#else
    dlclose( h );
#endif
}

void *
gr_dll_symbol(
    gr_dll_t h,
    const char * func_name
)
{
    void * p;

    if ( NULL == h ) {
        gr_error( "invalid params" );
        return NULL;
    }
    if ( NULL == func_name || '\0' == * func_name ) {
        gr_error( "invalid params" );
        return NULL;
    }

#if defined( WIN32 ) || defined( WIN64 )
    p = GetProcAddress( h, func_name );
#else
    p = dlsym( h, func_name );
#endif

    if ( p )
        return p;

    gr_error( "dlsymbol %s failed", func_name );
    return NULL;
}
