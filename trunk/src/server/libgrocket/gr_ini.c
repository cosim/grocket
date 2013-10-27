/**
 * @file libgrocket/gr_ini.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   INI operation
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

#include "gr_ini.h"
#include "gr_errno.h"
#include "gr_log.h"
#include "gr_global.h"
#include "gr_tools.h"
#include "gr_mem.h"

static_inline
int
gr_ini_open2(
    gr_ini * ini
)
{
    char *              p = ini->buf;
    char *              p2;
    char *              p3;
    char *              line;
    size_t              len;
    gr_ini_section *    section = NULL;
    bool                is_err = false;

    while( 1 ) {

        p2 = strchr( p, '\n' );

        if ( p2 ) {
            * p2 = '\0';
            if ( p2 != ini->buf ) {
                if ( '\r' == * ( p2 - 1 ) ) {
                    * ( p2 - 1 ) = '\0';
                }
            }
        }

        // 
        line = p;

        do {

            if ( '\0' == * line )
                break;
            if ( '#' == line[ 0 ] || ';' == line[ 0 ] )
                break;

            if ( '[' == line[ 0 ] ) {

                // section

                ++ line;

                len = strlen( line );
                if ( len <= 1 )
                    break;

                if ( ']' == line[ len - 1 ] )
                    line[ len - 1 ] = '\0';

                if ( ini->sections_count >= COUNT_OF( ini->sections ) ) {
                    gr_fatal( "section %d out of bounds!!!!", (int)ini->sections_count );
                    is_err = true;
                    break;
                }

                section = & ini->sections[ ini->sections_count ];

                section->name = line;
                ++ ini->sections_count;

            } else {

                if ( NULL == section ) {
                    gr_error( "section NULL" );
                    is_err = true;
                    break;
                }

                len = strlen( line );
                if ( len <= 1 ) {
                    gr_error( "invalid line length" );
                    is_err = true;
                    break;
                }

                p3 = strchr( line, '=' );
                if ( NULL == p3 ) {
                    gr_error( "'=' not found in line" );
                    is_err = true;
                    break;
                }

                * p3 = '\0';
                ++ p3;

                // left = line
                // right = p3;

                if ( '\0' == * line )
                    break;

                line = str_trim( line, NULL );
                if ( NULL == line || '\0' == * line )
                    break;

                p3 = str_trim( p3, NULL );
                if ( NULL == p3 )
                    p3 = (char*)"";

                if ( section->items_count >= COUNT_OF( section->items ) ) {
                    gr_error( "item %d out of bounds!!!!", (int)section->items_count );
                    is_err = true;
                    break;
                }

                section->items[ section->items_count ].key = line;
                section->items[ section->items_count ].val = p3;

                ++ section->items_count;
            }

        } while( 0 );

        //

        if ( NULL == p2 ) {
            break;
        } else {
            p = p2 + 1;
        }
    }

    if ( is_err ) {
        gr_error( "failed" );
        return GR_ERR_INVALID_PARAMS;
    }

    return 0;
}

int
gr_ini_open(
    gr_ini * ini,
    const char * path
)
{
    FILE * fp;
    long len;
    int r;
    char ph[ MAX_PATH ];

    strncpy( ph, path, sizeof( ph ) );
    ph[ sizeof( ph ) - 1 ] = '\0';
    path_to_os( ph );

    memset( ini, 0, sizeof( gr_ini ) );

    fp = fopen( ph, "rb" );
    if ( NULL == fp ) {
        gr_error( "fopen %s failed, %d", path, get_errno() );
        return -1;
    }

    fseek( fp, 0, SEEK_END );

    len = ftell( fp );
    if ( (long)-1 == len || 0 == len ) {
        gr_error( "ftell end %s failed, %d", path, get_errno() );
        fclose( fp );
        return -2;
    }

    fseek( fp, 0, SEEK_SET );

    ini->len = (size_t)len;
    ini->buf = (char *)gr_calloc( 1, len + 1 );
    if ( NULL == ini->buf ) {
        gr_error( "calloc %d failed", (int)len + 1 );
        fclose( fp );
        return -3;
    }

    if ( ini->len != fread( ini->buf, 1, ini->len, fp ) ) {
        gr_error( "fread failed: %d", get_errno() );
        gr_free( ini->buf );
        ini->buf = NULL;
        fclose( fp );
        return -4;
    }

    fclose( fp );

    r = gr_ini_open2( ini );
    if ( 0 != r ) {
        gr_error( "gr_ini_open2 return %d", r );
        gr_free( ini->buf );
        ini->buf = NULL;
        return r;
    }

    return 0;
}

int
gr_ini_open_memory(
    gr_ini * ini,
    const char * content,
    size_t content_len
)
{
    int r;

    memset( ini, 0, sizeof( gr_ini ) );

    ini->len = content_len;
    ini->buf = (char *)gr_calloc( 1, content_len + 1 );
    if ( NULL == ini->buf ) {
        gr_error( "calloc failed" );
        return -3;
    }

    memcpy( ini->buf, content, content_len );
    ini->buf[ content_len ] = '\0';

    r = gr_ini_open2( ini );
    if ( 0 != r ) {
        gr_error( "gr_ini_open2 return %d", r );
        gr_free( ini->buf );
        ini->buf = NULL;
        return r;
    }

    return 0;
}

void
gr_ini_close(
    gr_ini * ini
)
{
    if ( NULL != ini ) {
        return;
    }

    if ( ini->buf ) {
        gr_free( ini->buf );
        ini->buf = NULL;
    }

    ini->len = 0;
}


static_inline
gr_ini_section *
ini_find_section(
    gr_ini * ini,
    const char * section
)
{
    size_t i;

    if ( NULL == section || '\0' == * section ) {
        gr_error( "invalid params" );
        return NULL;
    }

    for ( i = 0; i < ini->sections_count; ++ i ) {

        gr_ini_section * p = & ini->sections[ i ];

        if ( 0 == stricmp( p->name, section ) ) {
            return p;
        }
    }

    gr_error( "section %s not found", section );
    return NULL;
}

static_inline
const char *
ini_find_in_section(
    gr_ini_section * ini,
    const char * name
)
{
    size_t i;

    if ( 0 == name || '\0' == * name ) {
        gr_error( "invalid params" );
        return NULL;
    }

    for ( i = 0; i < ini->items_count; ++ i ) {

        if ( 0 == stricmp( ini->items[ i ].key, name ) )
            return ini->items[ i ].val;
    }

    gr_debug( "section %s not found", name );
    return NULL;
}

size_t
gr_ini_get_sections_count(
    gr_ini * ini
)
{
    if ( ini ) {
        return ini->sections_count;
    } else {
        return 0;
    }
}

bool
gr_ini_get_sections(
   gr_ini * ini,
   const char ** sections,
   size_t * sections_count
)
{
    size_t i;
    size_t count = 0;

    if ( NULL == ini || NULL == sections || NULL == sections_count || 0 == * sections_count ) {
        return false;
    }

    for ( i = 0; i != ini->sections_count && i < * sections_count; ++ i ) {
        sections[ i ] = ini->sections[ i ].name;
    }

    * sections_count = ini->sections_count;
    return true;
}

bool
gr_ini_get_bool(
    gr_ini * ini,
    const char * section,
    const char * name,
    bool def
)
{
    gr_ini_section * p;
    const char * v;

    if ( NULL == ini || NULL == section || '\0' == * section || NULL == name || '\0' == * name ) {
        gr_error( "invalid params" );
        return def;
    }

    p = ini_find_section( ini, section );
    if ( NULL == p ) {
        gr_error( "ini_find_section failed" );
        return def;
    }

    v = ini_find_in_section( p, name );
    if ( NULL == v ) {
        gr_error( "ini_find_in_section failed" );
        return def;
    }

    if ( 0 == stricmp( "false", v ) || 0 == stricmp( "no", v ) )
        return false;
    else if ( 0 == stricmp( "true", v ) || 0 == stricmp( "yes", v ) )
        return true;
    return def;
}

int
gr_ini_get_int(
    gr_ini * ini,
    const char * section,
    const char * name,
    int def
)
{
    gr_ini_section * p;
    const char * v;

    if ( NULL == ini || NULL == section || '\0' == * section || NULL == name || '\0' == * name ) {
        gr_error( "invalid params" );
        return def;
    }

    p = ini_find_section( ini, section );
    if ( NULL == p ) {
        gr_error( "ini_find_section failed" );
        return def;
    }

    v = ini_find_in_section( p, name );
    if ( NULL == v ) {
        gr_warning( "ini_find_in_section failed" );
        return def;
    }

    if ( '\0' == * v || v[ 0 ] <= 0 || ( '-' != v[ 0 ] && ! isdigit( v[ 0 ] ) ) )
        return def;

    return atoi( v );
}

const char *
gr_ini_get_string(
    gr_ini * ini,
    const char * section,
    const char * name,
    const char * def
)
{
    gr_ini_section * p;
    const char * v;

    if ( NULL == ini || NULL == section || '\0' == * section || NULL == name || '\0' == * name ) {
        gr_error( "invalid params" );
        return def;
    }

    p = ini_find_section( ini, section );
    if ( NULL == p ) {
        gr_error( "ini_find_section failed" );
        return def;
    }

    v = ini_find_in_section( p, name );
    if ( NULL == v ) {
        gr_debug( "ini_find_in_section failed, name=%s", name );
        return def;
    }

    return v;
}
