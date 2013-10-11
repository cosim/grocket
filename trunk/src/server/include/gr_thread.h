/**
 * @file include/gr_thread.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/09/28
 * @version $Revision$ 
 * @brief   �߳���ع���
 * Revision History ���¼���
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
 * @brief ����һ���߳�
 * @param[out] thread �̴߳����ɹ��󷵻��߳�id���ɵ������ṩ�ڴ�
 * @param[in] start_routine �̺߳���
 * @param[in] arg �̲߳���
 * @param[in] cpu_id cpu id, -1 �ǲ���CPU
 * @param[in] priority ���ȼ���0��һ�㼶��-1�����ȼ���һ������1�����ȼ���һ������
 * @return int �ɹ�����0��ʧ�ܷ��ش�����
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
    // �߳���
    const char *    name;

    // �̱߳�ʶ
    pthread_t       h;

    // �߳������ɹ��¼�
    gr_event_t      event;

    // �û��̳߳�ʼ������
    gr_thread_func  init_routine;

    // �û��߳����к���
    gr_thread_func  routine;

    // �û�����Ĳ���
    void *          param;

    // �̱߳��
    int             id;

    // ���̰߳󶨵��ڴ棬�����÷�ʹ��
    char *          cookie;
    unsigned long   cookie_max;
    unsigned long   cookie_len;

    // �߳̿���״̬
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
