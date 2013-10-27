/**
 * @file include/gr_stdinc.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/09/24
 * @version $Revision$ 
 * @brief standard include header
 *
 * Revision History
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-09-24    Created.
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
#ifndef _GHOST_ROCKET_INCLUDE_GRSTDINC_H_
#define _GHOST_ROCKET_INCLUDE_GRSTDINC_H_

//#define static_inline   static inline
#define static_inline

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#if defined( _WIN32 )
    #ifndef WIN32
        #define WIN32
    #endif
#endif
#if defined( _WIN64 )
    #ifndef WIN64
        #define WIN64
    #endif
#endif

#if defined( WIN32 ) || defined( WIN64 )

    #define _CRT_SECURE_NO_WARNINGS

    // Consider using _snprintf_s instead. To disable deprecation, use _CRT_SECURE_NO_DEPRECATE.
    #pragma warning(disable:4996)

    // The file contains a character that cannot be represented in the current code page (936). Save the file in Unicode format to prevent data loss
    #pragma warning(disable:4819)

    #ifndef _WIN32_WINNT
        #ifdef WINVER
            #define _WIN32_WINNT WINVER
        #else
        #endif
    #else
        #if _WIN32_WINNT < 0x0400
            #error requires _WIN32_WINNT to be #defined to 0x0400 or greater
        #endif
    #endif

	#include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #include <wsipx.h>
    #include <process.h>    // _beginthreadex
    #include <io.h>

    #pragma comment( lib, "ws2_32.lib" )

    #define __attribute__(X)

#else

    // Mac must define __USE_GNU before pthread.h
    #ifndef __USE_GNU
        #define __USE_GNU
    #endif

    #if defined( __linux )
        #include <linux/unistd.h>   // __NR_gettid
    #endif

    #include <dlfcn.h>
    #include <pthread.h>
    #include <ctype.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <sys/types.h>
    #include <sys/time.h>
    #include <stdint.h>
    #include <stdarg.h>
    #include <sys/stat.h>
    #include <strings.h>
#endif

#if defined( __APPLE__ )
    #include <TargetConditionals.h>
    #define CHAR_MAX    INT8_MAX
#endif


#include <errno.h>
#include <string.h>
#include <memory.h>

// _LIB
#if ( defined(__APPLE__) && ( TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR ) )
    #define _LIB
#endif

// OFFSET_RECORD
#define OFFSET_RECORD(address, type, field) ((type *)(  \
                                  (char*)(address) -    \
                                  (char*)(&((type *)0)->field)))

// don't use max & min macros, it's doesn't cross iOS & Mac OS X
// std::max
// std::min
#ifdef __cplusplus
    // std::max disabled on windows, and max disabled on Mac OS X & iOS..... faint!
    // we only add std::max & std::min on windows, because we can not add max & min macros on iOS...
    #include <algorithm>

    #if defined( WIN32 ) || defined( WIN64 )
        // This is my beauty work!!!!!!!!!!!!! so std::max & std::min can used on windows
        #undef max
        #undef min
    #endif // #if defined( WIN32 ) || defined( WIN64 )
#endif // #ifdef __cplusplus

///////////////////////////////////////////////////////////////////////
//
// _DEBUG       ( 1 or not define )
// DEBUG        ( 1 or not define )
// NDEBUG       ( 1 or not define )
//

#if defined( _DEBUG )
    #if ! defined( DEBUG )
        #define DEBUG   1
    #endif
    #if defined( NDEBUG )
        #undef NDEBUG
    #endif
#elif defined( DEBUG )
    #if ! defined( _DEBUG )
        #define _DEBUG  1
    #endif
    #if defined( NDEBUG )
        #undef NDEBUG
    #endif
#else
    // release
    #if ! defined( NDEBUG )
        #define NDEBUG  1
    #endif
    #if defined( _DEBUG )
        #undef _DEBUG
    #endif
    #if defined( DEBUG )
        #undef DEBUG
    #endif
#endif

//
// S_BIG_ENDIAN            1 or 0
// S_LITTLE_ENDIAN         1 or 0
//
#if defined( WIN64 ) || defined(__i386) || defined(_M_IX86) || defined (__x86_64)
    #define S_LITTLE_ENDIAN     1
    #define S_BIG_ENDIAN        0
#elif defined(__sparc) || defined(__sparc__) || defined(__hppa) || defined(__ppc__) || defined(_ARCH_COM)
    #define S_BIG_ENDIAN        1
    #define S_LITTLE_ENDIAN     0
#elif defined(_WIN32_WCE)
    #define S_LITTLE_ENDIAN     1
    #define S_BIG_ENDIAN        0
#elif defined( __APPLE__ )
    #if ( TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR )
        // IOS
        #define S_LITTLE_ENDIAN     1
        #define S_BIG_ENDIAN        0
    #else
        // Mac OS X
        #define S_LITTLE_ENDIAN     1
        #define S_BIG_ENDIAN        0
    #endif

#else
    #error Unknown architecture
#endif

// atoi64
// stricmp
// strnicmp
// snprintf
// S_CRLF
// S_PATH_SEP_C
// S_PATH_SEP
// 
#if defined( WIN32 ) || defined( WIN64 )
    #define atoi64(x)       _atoi64((x))
    #ifndef stricmp
        #define stricmp     _stricmp
    #endif
    #ifndef strnicmp
        #define strnicmp    _strnicmp
    #endif
    #ifndef snprintf
        #define snprintf    _snprintf
    #endif
    #define lstrnicmp       _strnicmp
    #define S_CRLF          "\r\n"
    #define S_PATH_SEP_C    '\\'
    #define S_PATH_SEP      "\\"
    #define localtime_r( NOW, RET )     ((0 == localtime_s( (RET), (NOW) )) ? (RET) : NULL)
#else
    #define atoi64(x)       atoll((x))
    #ifndef stricmp
        #define stricmp     strcasecmp
    #endif
    #define strnicmp        strncasecmp
    #define MAX_PATH        260
    #define S_CRLF          "\n"
    #define S_PATH_SEP_C    '/'
    #define S_PATH_SEP      "/"
#endif

// COUNT_OF
#define COUNT_OF(a) (sizeof(a)/sizeof((a)[0]))

// I64D
// I64U
#ifdef _MSC_VER
    #ifndef I64D
        #define I64D "%I64d"
    #endif
    #ifndef I64U
        #define I64U "%I64u"
    #endif
#else
    #ifndef I64D
        #define I64D "%lld"
    #endif
    #ifndef I64U
        #define I64U "%llu"
    #endif
#endif

// S_INLINE
#ifndef S_INLINE
    #ifdef __cplusplus
        #define S_INLINE        inline
    #else
        #if defined(_MSC_VER)
            #define S_INLINE    __inline
        #elif defined(DIAB_COMPILER)
            // only pragmas supported, don't bother
            #define S_INLINE
        #elif ( S_GNUC )
            // GNU c style
            #define S_INLINE    __inline__
        #else
            #define S_INLINE
        #endif
        #define inline  S_INLINE
    #endif
#endif

// RET ALIGN_UP( N, ALIGN_BYTES )
#define ALIGN_UP( number, align_size )   \
    (((number) + (align_size) - 1) & ~((align_size) - 1))

// S_EXP
// S_IMP
#if ( defined(_MSC_VER) || defined(__CYGWIN__) || (defined(__HP_aCC) && defined(__HP_WINDLL) ) )
    #define S_EXP               __declspec(dllexport)
    #define S_IMP               __declspec(dllimport)
#elif defined(__SUNPRO_CC) && (__SUNPRO_CC >= 0x550)
    #define S_EXP               __global
    #define S_IMP
#else
    #define S_EXP
    #define S_IMP
#endif

// MSG_NO_SIGNAL
#ifndef MSG_NOSIGNAL
    #define	MSG_NOSIGNAL		0
#endif

#ifdef __cplusplus
extern "C" {
#endif
    
#if ( TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR )
    struct objc_object;
    typedef struct objc_object * id;
#endif
    
    // bool
    // true
    // false
#ifndef __cplusplus
    // Mac OS & iOS defined bool, so we add #ifndef bool #define bool ... #endif
    #ifndef bool
        #define	bool    char
    #endif
    #define	true        1
    #define	false       0
#endif
    
// getpid, gettid
// SOCKET
// SOCKET_ERROR
// INVALID_SOCKET
#if defined( WIN32 ) || defined( WIN64 )
    static inline int getpid()
    {
        return (int)GetProcessId( GetCurrentProcess() );
    }

    static inline int gettid()
    {
        return (int)GetCurrentThreadId();
    }
#else
	typedef int	SOCKET;

	#define SOCKET_ERROR    (int)-1
	#define INVALID_SOCKET  (int)-1

    #if defined( __linux )

    static inline int gettid()
    {
        return syscall(__NR_gettid);
    }

    #elif defined( __FreeBSD__ ) || defined( __APPLE__ )

    static inline int gettid()
    {
        return pthread_mach_thread_np( pthread_self() );
    }

    #endif

#endif

    // socklen_t
#if defined( WIN32 ) || defined(WIN64) || defined(__osf__) 
    typedef int socklen_t;
#endif
    
    // TCHAR
#if ! defined( WIN32 ) && ! defined( WIN64 )
	typedef char				TCHAR;
#endif
    
    // byte_t
    typedef unsigned char           byte_t;
    
    // int16_t
    // uint16_t
    typedef short                   int16_t;
    typedef unsigned short          uint16_t;
    
    // S_64
    // int32_t
    // uint32_t
    // uint64_t
    // int64_t
#ifdef __APPLE__
    
    #define S_64    1

#elif ( defined( WIN64 ) )

    // 64 bit
    typedef int                 int32_t;
    typedef unsigned int        uint32_t;

    typedef unsigned long long  uint64_t;
    typedef long long           int64_t;

    #define S_64    1

#elif ( (defined(__sun) && defined(__sparcv9)) || (defined(__linux) && defined(__x86_64)) || (defined(__hppa) && defined(__LP64__)) || (defined(_ARCH_COM) && defined(__64BIT__)) )

    // 64 bit
    typedef int                     int32_t;
    typedef unsigned int            uint32_t;

    //typedef unsigned long long int  uint64_t;
    //typedef long long int           int64_t;

    #define S_64    1
    
#else
    
    // 32 bit
    
#ifndef __linux__
    typedef long                int32_t;
    typedef unsigned long       uint32_t;
#endif
    
    typedef unsigned long long  uint64_t;
    typedef long long           int64_t;
    
    #define S_64    0
    
#endif
    
// __stdcall, __cdecl
#if ! defined( WIN32 ) && ! defined( WIN64 )
    #if ( 0 == S_64 )
        #define __stdcall   __attribute__((__stdcall__))
        #define __cdecl     __attribute__((__cdecl__))
    #else
        #define __stdcall
        #define __cdecl
    #endif
#endif

    // INT64( N )
#if defined( _MSC_VER )
    #define INT64( n ) n##i64
#elif defined(__HP_aCC)
    #define INT64(n)    n
#elif S_64
    #define INT64(n)    n##L
#else
    #define INT64(n)    n##LL
#endif
    
#if ! defined( WIN32 ) && ! defined( WIN64 )
    #define UINT32  uint32_t
    #define UINT16  uint16_t
    #define LONG    long
#endif

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GHOST_ROCKET_INCLUDE_GRSTDINC_H_
