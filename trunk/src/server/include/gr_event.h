/**
 * @file include/gr_event.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   event
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

#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_EVENT_H_
#define	_GHOST_ROCKET_SERVER_LIBGROCKET_GR_EVENT_H_

#include "gr_stdinc.h"
#include "gr_compiler_switch.h"

#if defined( __APPLE__ )
	#include <pthread.h>
#elif defined( __linux )
	#include <semaphore.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
#if defined( WIN32 ) || defined( WIN64 )
    HANDLE              cond;
#elif defined( __APPLE__ )
	pthread_mutex_t		lock;
	pthread_cond_t		cond;
#elif defined( __linux )
    sem_t               cond;
#endif
} gr_event_t;

int
gr_event_create(
    gr_event_t * o
);

/**
 * @brief 销毁事件对象
 * @param[in] event_t * o 事件对象实例
 */
void
gr_event_destroy(
    gr_event_t * o
);

/**
 * @brief 触发一个事件
 * @param[in] event_t * o 事件对象实例
 * @return bool 成功与否
 */
bool
gr_event_alarm(
    gr_event_t * o
);

/**
 * @brief 线程阻塞，等待一个事件的发生
 * @param[in] event_t * o 事件对象实例
 * @param[in] unsigned long ms 等待的超时时间，单位是毫秒(ms)。
 *        如果ms不为GR_EVENT_WAIT_INFINITE,那么等待ms毫秒后线程返回，
 *        如果ms == GR_EVENT_WAIT_INFINITE，则线程将永远等待。
 * @return int 有事件返回1，超时返回0，错误返回-1，如果因中断退出，则返回-2
 */
#define GR_EVENT_WAIT_INFINITE  0xFFFFFFFF

int
gr_event_wait(
    gr_event_t * o,
    unsigned int ms
);

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_EVENT_H_
