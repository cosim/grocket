/**
 * @file include/gr_thread.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/09/28
 * @version $Revision$ 
 * @brief   thread operation
 * Revision History
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-09-28    Created.
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

#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_THREAD_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_THREAD_H_

#include "gr_event.h"
#include "gr_compiler_switch.h"
//#include "gr_conn.h"

#ifdef __cplusplus
extern "C" {
#endif

// forward declare
struct gr_tcp_req_t;

#if defined( WIN32 ) || defined( WIN64 )
    typedef HANDLE      pthread_t;
#endif

void gr_thread_init();
void gr_thread_term();

int gr_processor_count();

/**
 * @brief 创建一个线程
 * @param[out] thread 线程创建成功后返回线程id，由调用者提供内存
 * @param[in] start_routine 线程函数
 * @param[in] arg 线程参数
 * @return int 成功返回0，失败返回错误码
 */
int
gr_thread_create(
    pthread_t * thread,
    void *(*start_routine)(void*),
    void * arg
);

/**
 * @function thread_join
 * @brief wait for thread stop
 */
void
gr_thread_join(
    pthread_t * thread
);

struct gr_thread_t;
typedef struct gr_thread_t gr_thread_t;

typedef void ( * gr_thread_func )( gr_thread_t * );

struct gr_thread_t
{
    // 线程名
    const char *    name;

    // 已经释放的请求对象,将来重用。本功能只在禁用worker线程时使用
    struct gr_tcp_req_t *  free_tcp_req_list;

    // 线程标识
    pthread_t       h;

    // 线程的 pid 和 tid
    int             pid;
    int             tid;

    // 线程启动成功事件
    gr_event_t      event;

    // 用户线程初始化函数
    gr_thread_func  init_routine;

    // 用户线程运行函数
    gr_thread_func  routine;

    // 用户传入的参数
    void *          param;

    // 线程编号
    int             id;

    // 与线程绑定的内存，供调用方使用
    char *          cookie;
    unsigned long   cookie_max;
    unsigned long   cookie_len;

    // 线程控制状态
    volatile bool   is_started;
    volatile bool   is_running;
    volatile bool   is_need_exit;

    bool            enable_thread;
    //

    // CPU亲缘性设置
    unsigned long   affinity_mask;

    // 优先级，0是一般级别，-1是优先级低一个级别，1是优先级高一个级别
    int             priority;
};

typedef struct
{
    gr_thread_t *   threads;
    int             thread_count;

} gr_threads_t;

#define ENABLE_THREAD   true
#define DISABLE_THREAD  false

int gr_threads_start(
    gr_threads_t *  threads,
    int             thread_count,
    gr_thread_func  init_routine,
    gr_thread_func  start_routine,
    void *          param,
    int             cookie_max_bytes,
    bool            wait_for_all_thread_ready,
    bool            enable_thread,
    const char *    name
);

void gr_threads_close( gr_threads_t * threads );

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_THREAD_H_
