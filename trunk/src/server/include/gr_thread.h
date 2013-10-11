/**
 * @file include/gr_thread.h
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
#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_THREAD_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_THREAD_H_

#include "gr_event.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined( WIN32 ) || defined( WIN64 )
    typedef HANDLE      pthread_t;
#endif

int gr_processor_count();

/**
 * @brief 创建一个线程
 * @param[out] thread 线程创建成功后返回线程id，由调用者提供内存
 * @param[in] start_routine 线程函数
 * @param[in] arg 线程参数
 * @param[in] cpu_id cpu id, -1 是不绑定CPU
 * @param[in] priority 优先级，0是一般级别，-1是优先级低一个级别，1是优先级高一个级别
 * @return int 成功返回0，失败返回错误码
 */
int
gr_thread_create(
    pthread_t * thread,
    void *(*start_routine)(void*),
    void * arg,
    int cpu_id,
    int priority
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

    // 线程标识
    pthread_t       h;

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
    //

};

typedef struct
{
    gr_thread_t *   threads;
    int             thread_count;

} gr_threads_t;

int gr_threads_start(
    gr_threads_t *  threads,
    int             thread_count,
    gr_thread_func  init_routine,
    gr_thread_func  start_routine,
    void *          param,
    int             cookie_max_bytes,
    bool            wait_for_all_thread_ready,
    const char *    name
);

void gr_threads_close( gr_threads_t * threads );

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_THREAD_H_
