/**
 * @file include/gr_dll.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   dynamic library operation
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

#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_DLL_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_DLL_H_

#include "gr_stdinc.h"
#include "gr_compiler_switch.h"

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////
//
// HINSTANCE LoadLibraryA( const char * path );
// void * GetProcAddress( HINSTANCE h, const char * name );
// void FreeLibrary( HINSTANCE h );
//
//   for Non Windows OS
//

#if defined(WIN32) || defined(WIN64)
	typedef	HINSTANCE	    gr_dll_t;
#else
	typedef void *		    gr_dll_t;

    #define HINSTANCE       gr_dll_t
    #define LoadLibraryA    gr_dll_open
    #define GetProcAddress  gr_dll_symbol
    #define FreeLibrary     gr_dll_close

#endif

/**
 * @function gr_dll_open
 * @brief open a dynamic library
 * @param[in] const TCHAR * path: path for dynamic library
 * @return dll_t: not NULL if successed, NULL if failed
 * @code
 
   dll_t h = gr_dll_open( "./MyLib" );
   .\MyLib.dll on windows, ./libMyLib.dylib on iOS, ./libMyLib.so on Android
 
 * @endcode
 */
gr_dll_t
gr_dll_open(
    const char * path
);

gr_dll_t
gr_dll_open_absolute(
    const char * path
);

/**
 * @brief close a dynamic library
 * @param[in] dll_t: dynamic library handle
 */
void
gr_dll_close(
	gr_dll_t h
);

/**
 * @brief query a function, that export function
 * @param[in] dll_t: dynamic library handle
 * @param[in] const char * func_name: function name 
 */
void *
gr_dll_symbol(
	gr_dll_t h,
	const char * func_name
);

#ifdef __cplusplus
}
#endif

#endif // ! #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_DLL_H_
