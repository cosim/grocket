/**
 * @file include/gr_queue.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/01
 * @version $Revision$ 
 * @brief   A thread write, B thread read, a no lock queue
 * Revision History
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-01    Created.
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

// 本结构的所有字段调用方都必须假装看不到。
// 那为什么还让你看到了呢？那是因为我不想像STL的list那样，
// 在压包前还要再在外面分配一个管理对象，这开销不划算。
struct gr_queue_item_t
{
    // single link table
    gr_queue_item_t *   next;

    // process thread set true to indicate this item is processed,
    // processed item will be delete on next gr_queue_push or gr_queue_destroy called
    volatile bool       is_processed;

    // 至少三个字节可用。这三字节已经在外部被其它类使用，具体参见 gr_queue_item_compact_t
    char                reserved[ 3 ];
};

#pragma pack( pop )

/**
 * @brief 创建一个单线程写、单线程读队列实例
 *   @param[in] void * cb_inst: 回调函数将以参数的方式将该参数传回
 *   @param[in] free_item: 删除函数
 * @return gr_queue_t *: 返回单线程写、单线程读队列实例
 * @warning 本函数为压数据线程调用
 */
gr_queue_t *
gr_queue_create(
    void ( * free_item )( void * param, gr_queue_item_t * p ),
    void * free_item_param
);

/**
 * @brief 删除一个单线程写、单线程读队列实例
 *   @param[in] gr_queue_t * self: 待删除对象实例
 * @warning 本函数为压数据线程调用
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
 * @brief 压入数据
 *   @param[in] gr_queue_t * self: 单线程写、单线程读队列
 *   @param[in] gr_queue_item_t * data: 压入的数据
 *   @param[in] bool is_emerge: 是否紧急数据包
 * @return int: 成功，则返回0
 * @warning 本函数为压数据线程调用
 */
int
gr_queue_push(
    gr_queue_t *        self,
    gr_queue_item_t *   data,
    bool                is_emerge
);

/**
 * @brief 在队列中取顶端数据，但并不弹出数据
 *   @param[in] gr_queue_t * self: 单线程写、单线程读队列
 *   @param[in] uint32_t wait_ms: 等待时间
 * @return gr_queue_item_t *: 取得的数据指针。如果没有任何数据，则返回NULL
 * @warning 本函数为弹数据线程调用; if is_wait is true and return NULL, then caller thread must exit
 */
gr_queue_item_t *
gr_queue_top(
    gr_queue_t *        self,
    uint32_t            wait_ms
);

/**
 * @brief 在队列中弹出顶端数据，如果队列为空，则什么都不做
 *   @param[in] gr_queue_t * self: 单线程写、单线程读队列
 *   @param[in] gr_queue_item_t * confirm_item: 确认要弹出的对象
 * @warning 本函数为弹数据线程调用
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
