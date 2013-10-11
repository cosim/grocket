/**
 * @file include/gr_event.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   事件相关操作
 * Revision History 大事件记
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-05    Created.
 **/
#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_EVENT_H_
#define	_GHOST_ROCKET_SERVER_LIBGROCKET_GR_EVENT_H_

#include "gr_stdinc.h"

#if defined( WIN32 ) || defined( WIN64 )
	// nothing
#elif defined( __APPLE__ )
	#include <pthread.h>
#elif defined( __linux )
    //#define USE_EVENT_FD

    #if defined( USE_EVENT_FD )
        #include <sys/eventfd.h>
    #else
	    #include <semaphore.h>
    #endif
#else
    #error unknown platform
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
    #if defined( USE_EVENT_FD )
    // eventfd 在内核版本，2.6.22以后有效
    int                 cond;
    #else
    sem_t               cond;
    #endif
#endif

    /// 该事件对象上是否已经调用过event_create
    bool                is_inited;

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
 * @param[in] unsigned long ms 等待的超时时间，单位是毫秒(ms)。如果ms>0,那么等待ms毫秒后线程返回，
 *        如果ms == (unsigned long)0，则线程将永远等待。
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
