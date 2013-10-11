/**
 * @file libgrocket/gr_thread.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/09/28
 * @version $Revision$ 
 * @brief   线程相关功能
 * Revision History 大事件记
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-09-28    Created.
 **/
#include "gr_thread.h"
#include "gr_log.h"
#include "gr_global.h"
#include "gr_tools.h"
#include "gr_errno.h"
#include "gr_mem.h"

//#define LINUX_SUPPORT_BIND_CPU
 
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
    void * arg,
    int cpu_id,
    int priority
)
{
#if defined( WIN32 ) || defined( WIN64 )
    DWORD tid;
    static DWORD_PTR mask_map[] = {
        0x00000001, 0x00000002, 0x00000004, 0x00000008,
        0x00000010, 0x00000020, 0x00000040, 0x00000080,
        0x00000100, 0x00000200, 0x00000400, 0x00000800,
        0x00001000, 0x00002000, 0x00004000, 0x00008000,
        0x00010000, 0x00020000, 0x00040000, 0x00080000,
        0x00100000, 0x00200000, 0x00400000, 0x00800000,
        0x01000000, 0x02000000, 0x04000000, 0x08000000,
        0x10000000, 0x20000000, 0x40000000, 0x80000000
    };

    if ( cpu_id >= 0 && cpu_id >= COUNT_OF( mask_map ) ) {
        gr_fatal( "ooh my god! your CPU is pretty cool!!!" );
        return -1;
    }

    * thread = CreateThread(
	    0,
        (unsigned int)0,
        (LPTHREAD_START_ROUTINE)start_routine,
        (LPVOID)arg,
        CREATE_SUSPENDED,
        & tid
    );
    if ( NULL == * thread ) {
        gr_fatal( "CreateThread failed: %d", get_errno() );
        return -2;
    }

    // CPU亲缘性
    if ( cpu_id >= 0 ) {
        if ( 0 == SetThreadAffinityMask( * thread, mask_map[ cpu_id ] ) ) {
            gr_fatal( "SetThreadAffinityMask failed: %d", get_errno() );
            CloseHandle( * thread );
            * thread = NULL;
            return -3;
        }
    }

    // 线程优先级
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
        gr_fatal( "invalid priority: %d", priority );
        CloseHandle( * thread );
        * thread = NULL;
        return -4;
        break;
    }
    if ( ! SetThreadPriority( * thread, priority ) ) {
        gr_fatal( "SetThreadPriority failed: %d", (int)GetLastError() );
        CloseHandle( * thread );
        * thread = NULL;
        return -5;
    }

    // 恢复线程运行
    if ( (DWORD)-1 == ResumeThread( * thread ) ) {
        gr_fatal( "ResumeThread failed: %d", get_errno() );
        CloseHandle( * thread );
        * thread = NULL;
        return -6;
    }
    return 0;
#elif defined( __APPLE__ )
    int r;
    int e;
    if ( cpu_id > 0 ) {
        r = pthread_create( thread, NULL, start_routine, arg );
        e = errno;
    } else {
        r = pthread_create( thread, NULL, start_routine, arg );
        e = errno;
    }
    if ( 0 != r ) {
        gr_fatal( "pthread_create failed: %d,%s", e, strerror(e) );
        return -2;
    }
    return 0;
#elif defined( __linux )
    int r;
    int e;
    #if defined( LINUX_SUPPORT_BIND_CPU )
    if ( cpu_id > 0 ) {
        cpu_set_t cpu_info;
        pthread_attr_t attr;

        __CPU_ZERO( & cpu_info );
        __CPU_SET( cpu_id, & cpu_info );
        pthread_attr_init( & attr );

        r = pthread_attr_setaffinity_np( & attr, sizeof( cpu_set_t ), & cpu_info );
        if ( 0 != r ) {
            return -1;
        }

        r = pthread_create( thread, & attr, start_routine, arg );
        e = errno;

        pthread_attr_destroy( & attr );
    } else {
    #endif // #if defined( LINUX_SUPPORT_BIND_CPU )
        r = pthread_create( thread, NULL, start_routine, arg );
        e = errno;
    #if defined( LINUX_SUPPORT_BIND_CPU )
    }
    #endif // #if defined( LINUX_SUPPORT_BIND_CPU )
    if ( 0 != r ) {
        gr_fatal( "pthread_create failed: %d,%s", e, strerror(e) );
        return -2;
    }
    return 0;
#else
    #error unknonw platform
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
    int pid = getpid();
    int tid = gettid();

    // 线程启动标记
    thread->is_running = true;
    // 初始化
    if ( NULL != thread->init_routine ) {
        gr_info( "[pid=%d][tid=%d][name=%s.%d] init thread starting", pid, tid, thread->name, thread->id );
        thread->init_routine( thread );
        gr_info( "[pid=%d][tid=%d][name=%s.%d] init thread started", pid, tid, thread->name, thread->id );
    }
    // 线程启动事件
    gr_event_alarm( & thread->event );
    // 线程用户函数
    gr_info( "[pid=%d][tid=%d][name=%s.%d] thread routine starting", pid, tid, thread->name, thread->id );
    thread->routine( thread );
    gr_info( "[pid=%d][tid=%d][name=%s.%d] thread routine exit", pid, tid, thread->name, thread->id );
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
                    gr_fatal( "bad_alloc %d", t->cookie_max );
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

            ret = gr_event_create( & t->event );
            if ( 0 != ret ) {
                gr_fatal( "[init]gr_event_create return %d, errno=%d,%s",
                    ret, errno, strerror( errno ) );
                r = GR_ERR_SYSTEM_CALL_FAILED;
                break;
            }

            ret = gr_thread_create(
                & t->h,
                (std_thread_routine)gr_thread_runtine,
                t,
                -1, 0 );
            if ( 0 != ret ) {
                gr_fatal( "[init]gr_thread_create failed" );
                r = GR_ERR_CREATE_THREAD_FALED;
                gr_event_destroy( & t->event );
                break;
            }

            t->is_started = true;

            ++ p->thread_count;
        }
        if ( GR_OK != r ) {
            break;
        }

        // 等所有线程都启动
        if ( wait_for_all_thread_ready ) {
            for ( i = 0; i < p->thread_count; ++ i ) {
                gr_thread_t * t = & p->threads[ i ];
                //TODO: 没判断返回值
                gr_event_wait( & t->event, GR_EVENT_WAIT_INFINITE );
            }
        }

        r = GR_OK;
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
                    gr_event_destroy( & t->event );
                    -- threads->thread_count;
                }
            }

            // 删所有线程的cookie
            for ( i = 0; i < threads->thread_count; ++ i ) {
                t = & threads->threads[ i ];
                t->cookie_len = 0;
                t->cookie_max = 0;
                if ( NULL != t->cookie ) {
                    gr_free( t->cookie );
                    t->cookie = NULL;
                }
            }
        }

        gr_free( threads->threads );
        threads->threads = NULL;
    }
}
