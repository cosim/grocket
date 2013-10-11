/**
 * @file include/gr_event.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   �¼���ز���
 * Revision History ���¼���
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
    // eventfd ���ں˰汾��2.6.22�Ժ���Ч
    int                 cond;
    #else
    sem_t               cond;
    #endif
#endif

    /// ���¼��������Ƿ��Ѿ����ù�event_create
    bool                is_inited;

} gr_event_t;

int
gr_event_create(
    gr_event_t * o
);

/**
 * @brief �����¼�����
 * @param[in] event_t * o �¼�����ʵ��
 */
void
gr_event_destroy(
    gr_event_t * o
);

/**
 * @brief ����һ���¼�
 * @param[in] event_t * o �¼�����ʵ��
 * @return bool �ɹ����
 */
bool
gr_event_alarm(
    gr_event_t * o
);

/**
 * @brief �߳��������ȴ�һ���¼��ķ���
 * @param[in] event_t * o �¼�����ʵ��
 * @param[in] unsigned long ms �ȴ��ĳ�ʱʱ�䣬��λ�Ǻ���(ms)�����ms>0,��ô�ȴ�ms������̷߳��أ�
 *        ���ms == (unsigned long)0�����߳̽���Զ�ȴ���
 * @return int ���¼�����1����ʱ����0�����󷵻�-1��������ж��˳����򷵻�-2
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
