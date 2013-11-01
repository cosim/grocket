/**
 * @file server/include/gr_compiler_switch.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/29
 * @version $Revision$ 
 * @brief compiler switch
 *
 * Revision History
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-29    Created.
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
#ifndef _GHOST_ROCKET_SERVER_INCLUDE_GR_COMPILER_SWITCH_H_
#define _GHOST_ROCKET_SERVER_INCLUDE_GR_COMPILER_SWITCH_H_

// 要达到最佳性能，还要在 Makefile 里去掉 -g 加 -O3

// 定义它调试连接对象，性能会降低
//#define GR_DEBUG_CONN

// 定义它启动DEBUG日志，不定义它禁用DEBUG日志。为了减少日志对性能的影响
//#define ENABLE_DEBUG_LOG

// 是否允许 inline，如果 inline 了，跑 valgrind 就不清楚谁的开销最大了
#define ENABLE_INLINE_FUNCTION

///////////////////////////////////////////////////////////////////////

#ifdef ENABLE_INLINE_FUNCTION
    #define static_inline   static inline
#else
    #define static_inline   static
#endif

#endif // #ifndef _GHOST_ROCKET_SERVER_INCLUDE_GR_COMPILER_SWITCH_H_
