/**
 * @file libgrocket/gr_thread.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/09/28
 * @version $Revision$ 
 * @brief   thread
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

#include "gr_thread.h"
#include "gr_log.h"
#include "gr_global.h"
#include "gr_tools.h"
#include "gr_config.h"
#include "gr_errno.h"
#include "gr_conn.h"
#include "gr_mem.h"
#if defined( __linux )
    #include <sched.h>
#endif

void gr_thread_init()
{
    g_ghost_rocket_global.affinity_masks[ 0 ] = 0x00000001;
    g_ghost_rocket_global.affinity_masks[ 1 ] = 0x00000002;
    g_ghost_rocket_global.affinity_masks[ 2 ] = 0x00000004;
    g_ghost_rocket_global.affinity_masks[ 3 ] = 0x00000008;
    g_ghost_rocket_global.affinity_masks[ 4 ] = 0x00000010;
    g_ghost_rocket_global.affinity_masks[ 5 ] = 0x00000020;
    g_ghost_rocket_global.affinity_masks[ 6 ] = 0x00000040;
    g_ghost_rocket_global.affinity_masks[ 7 ] = 0x00000080;
    g_ghost_rocket_global.affinity_masks[ 8 ] = 0x00000100;
    g_ghost_rocket_global.affinity_masks[ 9 ] = 0x00000200;
    g_ghost_rocket_global.affinity_masks[10 ] = 0x00000400;
    g_ghost_rocket_global.affinity_masks[11 ] = 0x00000800;
    g_ghost_rocket_global.affinity_masks[12 ] = 0x00001000;
    g_ghost_rocket_global.affinity_masks[13 ] = 0x00002000;
    g_ghost_rocket_global.affinity_masks[14 ] = 0x00004000;
    g_ghost_rocket_global.affinity_masks[15 ] = 0x00008000;
    g_ghost_rocket_global.affinity_masks[16 ] = 0x00010000;
    g_ghost_rocket_global.affinity_masks[17 ] = 0x00020000;
    g_ghost_rocket_global.affinity_masks[18 ] = 0x00040000;
    g_ghost_rocket_global.affinity_masks[19 ] = 0x00080000;
    g_ghost_rocket_global.affinity_masks[20 ] = 0x00100000;
    g_ghost_rocket_global.affinity_masks[21 ] = 0x00200000;
    g_ghost_rocket_global.affinity_masks[22 ] = 0x00400000;
    g_ghost_rocket_global.affinity_masks[23 ] = 0x00800000;
    g_ghost_rocket_global.affinity_masks[24 ] = 0x01000000;
    g_ghost_rocket_global.affinity_masks[25 ] = 0x02000000;
    g_ghost_rocket_global.affinity_masks[26 ] = 0x04000000;
    g_ghost_rocket_global.affinity_masks[27 ] = 0x08000000;
    g_ghost_rocket_global.affinity_masks[28 ] = 0x10000000;
    g_ghost_rocket_global.affinity_masks[29 ] = 0x20000000;
    g_ghost_rocket_global.affinity_masks[30 ] = 0x40000000;
    g_ghost_rocket_global.affinity_masks[31 ] = 0x80000000;
}

void gr_thread_term()
{
}

static_inline
unsigned long get_cpu_affinity( const char * role, int thread_id, int * priority )
{
    // tcp.listen
    // tcp.input
    // tcp.output
    // udp.in
    // udp.out
    // svr.worker


    unsigned long *  masks = g_ghost_rocket_global.affinity_masks;
    int              ncpu = gr_processor_count();
    static const int ncpu_max = 32;

    if ( ncpu > ncpu_max ) {
        ncpu = ncpu_max;
    }

    do {

        if ( thread_id >= 0 ) {
            if ( strstr( role, "tcp.input" ) ) {
                const char *    affinity;
                size_t          i;
                unsigned long   masks;
                char            cpu_bind[ 1024 ] = "";
                char            t[ 32 ];
                char            c;
                
                affinity = gr_config_tcp_in_thread_affinity( thread_id );
                if ( NULL == affinity || '\0' == * affinity ) {
                    gr_info( "[init][tcp.input %d][affinity=NONE]", thread_id );
                    break;
                }

                masks = 0;
                for (   i = 0;
                        i < COUNT_OF( g_ghost_rocket_global.affinity_masks ) && i < ncpu;
                        ++ i )
                {
                    c = affinity[ i ];
                    if ( '1' != c && '0' != c ) {
                        break;
                    }
                    if ( '0' == c ) {
                        continue;
                    }
                    masks |= g_ghost_rocket_global.affinity_masks[ i ];

                    //TODO: 我知道这儿有缓冲区溢出漏洞, 你丫自己能害你自己?  
                    sprintf( t, "%d", i );
                    if ( '\0' != cpu_bind[ 0 ] ) {
                        strcat( cpu_bind, "," );
                    }
                    strcat( cpu_bind, t );
                }
                gr_info( "[init][tcp.input %d][affinity=%s]",
                    thread_id, cpu_bind );
            }
        }
    } while ( false );

    // 返回0，爱用哪个核用哪个核
    * priority = 0;
    return 0;
}

int gr_processor_count()
{
    // 我不相信你丫可以热插拔CPU
    static int r = 0;
    if ( 0 == r ) {
    
#if defined( WIN32 ) || defined( WIN64 )
        SYSTEM_INFO si;
        GetSystemInfo( & si );
        r = (int)si.dwNumberOfProcessors;
#else
        r = (int)sysconf( _SC_NPROCESSORS_CONF );
#endif
    }
    return r;
}

int
gr_thread_create(
    pthread_t * thread,
    void *(*start_routine)(void*),
    void * arg
)
{
#if defined( WIN32 ) || defined( WIN64 )
    DWORD tid;

    * thread = CreateThread(
	    0,
        (unsigned int)0,
        (LPTHREAD_START_ROUTINE)start_routine,
        (LPVOID)arg,
        0 /*CREATE_SUSPENDED*/,
        & tid
    );
    if ( NULL == * thread ) {
        gr_fatal( "[init]CreateThread failed: %d", get_errno() );
        return -2;
    }

    return 0;
#else
    int r;
    int e;
    r = pthread_create( thread, NULL, start_routine, arg );
    e = errno;
    if ( 0 != r ) {
        gr_fatal( "[init]pthread_create failed: %d,%s", e, strerror(e) );
        return -2;
    }
    return 0;
#endif
}

void
gr_thread_join(
    pthread_t * thread
)
{
#if defined( WIN32 ) || defined( WIN64 )
    if ( thread && * thread ) {
        HANDLE h = * thread;
        WaitForSingleObject( h, INFINITE );
        CloseHandle( h );
        * thread = NULL;
    }
#else
    pthread_join( * thread, NULL );
#endif
}

void * gr_thread_runtine( gr_thread_t * thread )
{
    thread->free_tcp_req_list = NULL;
    thread->pid = getpid();
    thread->tid = gettid();

    do {

#if defined( WIN32 ) || defined( WIN64 )
        {
            int priority = thread->priority;
            // CPU亲缘性
            if ( 0 != thread->affinity_mask ) {
                if ( 0 == SetThreadAffinityMask( thread->h, (DWORD_PTR)thread->affinity_mask ) ) {
                    gr_fatal( "[init]SetThreadAffinityMask failed: %d", get_errno() );
                    break;
                }
            }
            // 优先级
            switch( priority ) {
            case -3:
                priority = THREAD_PRIORITY_IDLE;
                break;
            case -2:
                priority = THREAD_PRIORITY_LOWEST;
                break;
            case -1:
                priority = THREAD_PRIORITY_BELOW_NORMAL;
                break;
            case 0:
                priority = THREAD_PRIORITY_NORMAL;
                break;
            case 1:
                priority = THREAD_PRIORITY_ABOVE_NORMAL;
                break;
            case 2:
                priority = THREAD_PRIORITY_HIGHEST;
                break;
            case 3:
                priority = THREAD_PRIORITY_TIME_CRITICAL;
                break;
            default:
                gr_fatal( "[init]invalid priority: %d", priority );
                return NULL;
                break;
            }
            if ( THREAD_PRIORITY_NORMAL != priority ) {
                if ( ! SetThreadPriority( thread->h, priority ) ) {
                    gr_fatal( "[init]SetThreadPriority failed: %d", (int)GetLastError() );
                    break;
                }
            }
        }

#elif defined( __linux )
        if ( 0 != thread->affinity_mask ) {
            // CPU亲缘性
            unsigned long   affinity = thread->affinity_mask;
            cpu_set_t       mask;
            int             i;
            int             r;

            CPU_ZERO( & mask );
            i = 0;
            do {
                if ( affinity & 1 ) {
                    CPU_SET( i, & mask );
                }
                ++ i;
                affinity >>= 1;
            } while ( affinity );

            r = sched_setaffinity( thread->tid, sizeof( cpu_set_t ), & mask );
            if ( 0 != r ) {
                gr_fatal( "[init]sched_setaffinity( %d ) failed, errno=%d:%s",
                    thread->tid, errno, strerror( errno ) );
                break;
            }
        }
#endif

        // 线程启动标记
        thread->is_running = true;
        // 初始化
        if ( NULL != thread->init_routine ) {
            gr_info( "[init][%s%d][tid=%d]init enter",
                thread->name, thread->id, thread->tid );
            thread->init_routine( thread );
            gr_info( "[init][%s%d][tid=%d]init leave",
                thread->name, thread->id, thread->tid );
        }
        // 线程启动事件
        gr_event_alarm( & thread->event );
        // 线程用户函数
        gr_info( "[init][%s%d][tid=%d]runtine enter",
            thread->name, thread->id, thread->tid );
        thread->routine( thread );
        gr_info( "[term][%s%d][tid=%d]runtine leave",
            thread->name, thread->id, thread->tid );

    } while ( false );

    // 释放 tcp_req pool
    while ( thread->free_tcp_req_list ) {
        gr_tcp_req_t * next = (gr_tcp_req_t *)thread->free_tcp_req_list->entry.next;
        gr_tcp_req_free( thread->free_tcp_req_list, false );
        thread->free_tcp_req_list = next;
    }

    gr_info( "[term][%s%d][tid=%d] exit",
        thread->name, thread->id, thread->tid );

    // 线程退出标记
    thread->is_running = false;
    // 线程关闭事件
    gr_event_alarm( & thread->event );
    return NULL;
}

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
)
{
    int             r   = GR_ERR_UNKNOWN;
    int             i;
    gr_threads_t *  p   = threads;

    typedef void *(*std_thread_routine)(void*);

    do {
        p->thread_count = 0;

        p->threads = (gr_thread_t *)gr_calloc( 1, sizeof( gr_thread_t ) * thread_count );
        if ( NULL == p->threads ) {
            gr_fatal( "[init]malloc %d bytes failed, errno=%d,%s",
                (int)sizeof( gr_thread_t ) * thread_count, errno, strerror( errno ) );
            r = GR_ERR_BAD_ALLOC;
            break;
        }

        r = GR_OK;

        for ( i = 0; i < thread_count; ++ i ) {

            int ret;
            gr_thread_t * t = & p->threads[ i ];

            t->cookie       = NULL;
            t->cookie_max   = (unsigned long)cookie_max_bytes;
            t->cookie_len   = 0;

            if ( t->cookie_max > 0 ) {
                t->cookie = (char *)gr_calloc( 1, t->cookie_max );
                if ( NULL == t->cookie ) {
                    gr_fatal( "[init]bad_alloc %d", t->cookie_max );
                    r = GR_ERR_BAD_ALLOC;
                    break;
                }
            } else {
                t->cookie = NULL;
            }

            t->init_routine = init_routine;
            t->routine      = start_routine;
            t->param        = param;
            t->id           = i;
            t->is_started   = false;
            t->name         = name;
            t->affinity_mask= get_cpu_affinity( name, i, & t->priority );
            t->enable_thread= enable_thread;

            ret = gr_event_create( & t->event );
            if ( 0 != ret ) {
                gr_fatal( "[init]gr_event_create return %d, errno=%d,%s",
                    ret, errno, strerror( errno ) );
                r = GR_ERR_SYSTEM_CALL_FAILED;
                break;
            }

            if ( enable_thread ) {
                ret = gr_thread_create(
                    & t->h,
                    (std_thread_routine)gr_thread_runtine,
                    t );
                if ( 0 != ret ) {
                    gr_fatal( "[init]gr_thread_create failed" );
                    r = GR_ERR_CREATE_THREAD_FALED;
                    gr_event_destroy( & t->event );
                    break;
                }

                t->is_started = true;
            }

            ++ p->thread_count;
        }
        if ( GR_OK != r ) {
            break;
        }

        // 等所有线程都启动
        if ( enable_thread && wait_for_all_thread_ready ) {
            for ( i = 0; i < p->thread_count; ++ i ) {
                gr_thread_t * t = & p->threads[ i ];
                //TODO: 没判断返回值
                gr_event_wait( & t->event, GR_EVENT_WAIT_INFINITE );

                if ( ! t->is_running ) {
                    gr_fatal( "[init] %s thread %d start failed", name, i );
                    r = GR_ERR_UNKNOWN;
                    break;
                }
            }
        }
    } while ( false );

    if ( GR_OK != r ) {
        gr_threads_close( p );
        return r;
    }

    return GR_OK;
}

void gr_threads_close( gr_threads_t * threads )
{
    gr_thread_t * t;

    if ( NULL == threads ) {
        return;
    }

    if ( NULL != threads->threads ) {
        if ( threads->thread_count > 0 ) {
            int i;

            // 给所有线程设置关闭标记
            for ( i = 0; i < threads->thread_count; ++ i ) {
                t = & threads->threads[ i ];
                if ( t->is_started ) {
                    t->is_need_exit = true;
                }
            }

            // 等所有线程退出
            while ( threads->thread_count > 0 ) {
                t = & threads->threads[ threads->thread_count - 1 ];
                if ( t->is_started ) {
                    gr_thread_join( & t->h );
                    t->is_started = false;
                }

                // 删线程的cookie
                t->cookie_len = 0;
                t->cookie_max = 0;
                if ( NULL != t->cookie ) {
                    gr_free( t->cookie );
                    t->cookie = NULL;
                }

                gr_event_destroy( & t->event );

                -- threads->thread_count;
            }
        }

        gr_free( threads->threads );
        threads->threads = NULL;
    }
}
