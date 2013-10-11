/**
 * @file include/gr_queue.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/01
 * @version $Revision$ 
 * @brief   ������д�������̶��Ķ���
 * Revision History ���¼���
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-01    Created.
 **/
#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_QUEUE_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_QUEUE_H_

#include "gr_stdinc.h"

#ifdef __cplusplus
extern "C" {
#endif

struct gr_queue_t;
typedef struct gr_queue_t       gr_queue_t;
struct gr_queue_item_t;
typedef struct gr_queue_item_t  gr_queue_item_t;

#pragma pack( push, 4 )

// ���ṹ�������ֶε��÷��������װ��������
// ��Ϊʲô�����㿴�����أ�������Ϊ�Ҳ�����STL��list������
// ��ѹ��ǰ��Ҫ�����������һ����������⿪�������㡣
struct gr_queue_item_t
{
    // single link table
    gr_queue_item_t *   next;

    // process thread set true to indicate this item is processed,
    // processed item will be delete on next gr_queue_push or gr_queue_destroy called
    volatile bool       is_processed;

    // ���������ֽڿ��á������ֽ��Ѿ����ⲿ��������ʹ�ã�����μ� gr_queue_item_compact_t
    char                reserved[ 3 ];
};

#pragma pack( pop )

/**
 * @brief ����һ�����߳�д�����̶߳�����ʵ��
 *   @param[in] void * cb_inst: �ص��������Բ����ķ�ʽ���ò�������
 *   @param[in] free_item: ɾ������
 * @return gr_queue_t *: ���ص��߳�д�����̶߳�����ʵ��
 * @warning ������Ϊѹ�����̵߳���
 */
gr_queue_t *
gr_queue_create(
    void ( * free_item )( void * param, gr_queue_item_t * p ),
    void * free_item_param
);

/**
 * @brief ɾ��һ�����߳�д�����̶߳�����ʵ��
 *   @param[in] gr_queue_t * self: ��ɾ������ʵ��
 * @warning ������Ϊѹ�����̵߳���
 */
void
gr_queue_destroy(
    gr_queue_t * self
);

void
gr_queue_clear(
    gr_queue_t * self
);

void
gr_queue_exit(
    gr_queue_t * self
);

bool
gr_queue_is_empty(
    gr_queue_t * self
);

/**
 * @brief ѹ������
 *   @param[in] gr_queue_t * self: ���߳�д�����̶߳�����
 *   @param[in] gr_queue_item_t * data: ѹ�������
 *   @param[in] bool is_emerge: �Ƿ�������ݰ�
 * @return int: �ɹ����򷵻�0
 * @warning ������Ϊѹ�����̵߳���
 */
int
gr_queue_push(
    gr_queue_t *        self,
    gr_queue_item_t *   data,
    bool                is_emerge
);

/**
 * @brief �ڶ�����ȡ�������ݣ���������������
 *   @param[in] gr_queue_t * self: ���߳�д�����̶߳�����
 *   @param[in] uint32_t wait_ms: �ȴ�ʱ��
 * @return gr_queue_item_t *: ȡ�õ�����ָ�롣���û���κ����ݣ��򷵻�NULL
 * @warning ������Ϊ�������̵߳���; if is_wait is true and return NULL, then caller thread must exit
 */
gr_queue_item_t *
gr_queue_top(
    gr_queue_t *        self,
    uint32_t            wait_ms
);

/**
 * @brief �ڶ����е����������ݣ��������Ϊ�գ���ʲô������
 *   @param[in] gr_queue_t * self: ���߳�д�����̶߳�����
 *   @param[in] gr_queue_item_t * confirm_item: ȷ��Ҫ�����Ķ���
 * @warning ������Ϊ�������̵߳���
 */
bool
gr_queue_pop_top(
    gr_queue_t *        self,
    gr_queue_item_t *   confirm_item
);

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_QUEUE_H_
