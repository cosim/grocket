/**
 * @file include/gr_atomic.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/14
 * @version $Revision$ 
 * @brief   atomic operation
 * Revision History
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-14    Created.
 *       2     zouyueming   2013-10-16    add port to linux 32/64 and gcc 4.1.2+
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

#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_ATOMIC_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_ATOMIC_H_

#include "gr_stdinc.h"
#if defined( __APPLE__ )
#include <libkern/OSAtomic.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// void gr_atomic_add( int v, volatile gr_atomic_t * dst )
#if defined( __linux )

    // gr_atomic_t 在 linux 下是 32 位
    typedef volatile int                gr_atomic_t;

    #if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ > 1) || (__GNUC__ == 4 && __GNUC_MINOR__ == 1 && __GNUC_PATCHLEVEL__ >= 2)
        // GCC 从 4.1.2 开始内置 __sync_fetch_and_add 函数
        static inline void gr_atomic_add( int v, int * dst )
        {
            // 苹果上的gcc返回的值是加之后的值,不对
            __sync_fetch_and_add( dst, v );
        }
    #elif defined( __x86_64 )
        // 64 位 X86 CPU
        static inline void gr_atomic_add( int v, int * dst )
        {
            asm volatile("addl %1,%0"
                            : "+m" (*dst)
                            : "ir" (v));
        }
    #else
        // 32 位 X86 CPU
        static int gr_atomic_add( int v, int * dst)
        {
            asm volatile(
                "movl        %0,%%eax;"
                "movl        %1,%%ecx;"
                "lock xadd   %%eax,(%%ecx);"
                ::"m"(v),"m"(dst)
            );
        }
    #endif

#elif defined( __APPLE__ )

    // gr_atomic_t 在 apple 下是 32 位
    typedef volatile int32_t                gr_atomic_t;
    #define gr_atomic_add( v, dst )         OSAtomicAdd32( (int32_t)(v), (volatile int32_t *)(dst) )

#elif defined( WIN32 ) || defined( WIN64 )

    // gr_atomic_t 在 windows 下随机器字长不同而大小不同
    typedef LONG volatile                   gr_atomic_t;
    #define gr_atomic_add( v, dst )         InterlockedExchangeAdd( (LONG volatile *)(dst), (LONG)(v) )

#else
    #error unknown platform
#endif

#ifdef __cplusplus
}
#endif

#endif // ! #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_ATOMIC_H_
