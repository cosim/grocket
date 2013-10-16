/**
 * @file librocket/gr_queue.c
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

#include "gr_queue.h"
#include "gr_log.h"
#include "gr_global.h"
#include "gr_event.h"
#include "gr_mem.h"
#include "gr_tools.h"
#include <assert.h>

// 一个真正的队列的队列链表，是个单链表
typedef struct queue_inner
{
    // 列表头，调用线程维护，工作线程不能访问
    // 一但调用线程删除了已被标记为已处理的数据项时，
    // 该指针会被修改
    volatile gr_queue_item_t *  head;

    // 当前处理指针，调用线程负责初始化，工作线程负责改变它最终到NULL。
    // 该成员用于处理线程记录自己当前正在处理的数据项，
    // 因为处理过的数据项被打了已处理标记，但并未删除，
    // 处理线程不能每次处理都临时查找未处理项的开始位置
    volatile gr_queue_item_t *  curr;

    // 列表尾，调用线程维护，工作线程不能访问
    // 该成员用于向队列尾加入数据项
    volatile gr_queue_item_t *  tail;

} queue_inner;

struct gr_queue_t
{
    // gr_queue_top 但没数据时使用的等待事件
    gr_event_t                  event;

    // 退出通知
    gr_event_t                  exited_event;

    // 紧急队列
    volatile queue_inner        emerge_queue;
    // 常规队列
    volatile queue_inner        normal_queue;

    // 调用过gr_queue_top后，记录取出的对象所在的队列，gr_queue_pop_top函数使用
    volatile queue_inner *      curr_queue;

    // 回调函数的一个参数，用户传进来的，我给你丫返回去
    void *                      callback_param;

    // 用户传进来的回调函数
    void ( * free_item )( void * param, gr_queue_item_t * p );

    // 当前是否在Top函数使用event_wait函数等待的过程中。如果是，则压包时会使用event_alaram唤醒
    volatile bool               in_event;

    // 当前是否需要线程退出，这个是给Top的信号
    volatile bool               need_exit;

    // 当前是否处理线程已经退出了
    volatile bool               is_exited;
    
#if defined( WIN32 ) || defined( WIN64 )
	// 记录调Top和PopTop的线程ID
	volatile DWORD              pop_thread_id;
#endif
};

gr_queue_t *
gr_queue_create(
    void ( * free_item )( void * param, gr_queue_item_t * p ),
    void * free_item_param
)
{
    gr_queue_t *    self;
    int             r;
    
    if ( NULL == free_item ) {
        return NULL;
    }

    self = (gr_queue_t *)gr_calloc( 1, sizeof( gr_queue_t ) );
    if ( NULL == self ) {
        gr_fatal( "gr_calloc %d failed: %d", (int)sizeof( gr_queue_t ), get_errno() );
        return NULL;
    }

    r = gr_event_create( & self->event );
    if ( 0 != r ) {
        gr_fatal( "gr_event_create return error %d", r );
        gr_free( self );
        return NULL;
    }

    r = gr_event_create( & self->exited_event );
    if ( 0 != r ) {
        gr_fatal( "gr_event_create return error %d", r );
        gr_event_destroy( & self->event );
        gr_free( self );
        return NULL;
    }

    self->callback_param = free_item_param;
    self->free_item = free_item;

    return self;
}

static inline
void
gr_queue_inner_destroy(
    gr_queue_t * self,
    volatile queue_inner * inner
)
{
    if ( inner->head ) {

        volatile gr_queue_item_t * p;

        do {

            p = inner->head;
            if ( NULL == p ) {
                inner->tail = NULL;
                inner->curr = NULL;
                break;
            }

            // remove item from link table
            inner->head = inner->head->next;

            // call user function to delete the item
            self->free_item( self->callback_param, (gr_queue_item_t *)p );

        } while( 1 );
    }
}

void
gr_queue_exit(
    gr_queue_t * self
)
{
    if ( self ) {

        // we will exit
        self->need_exit = true;

        if ( self->in_event ) {
            self->in_event = false;
            gr_event_alarm( & self->event );

            // wait for process thread exit
            gr_event_wait( & self->exited_event, GR_EVENT_WAIT_INFINITE );
    }

        // 关完了恢复一下状态
        self->need_exit = false;

#if defined( WIN32 ) || defined( WIN64 )
    	self->pop_thread_id = 0;
#endif
    }
}

void
gr_queue_destroy(
    gr_queue_t * self
)
{
    if ( NULL != self ) {

        gr_queue_exit( self );

        // delete all pending items

        // If another thread call gr_queue_tPush at same time, then crash, but that's OK. 
        gr_queue_inner_destroy( self, & self->emerge_queue );
        gr_queue_inner_destroy( self, & self->normal_queue );

        // destroy event
        gr_event_destroy( & self->event );
        gr_event_destroy( & self->exited_event );

        gr_free( self );
    }
}

static inline
void
gr_queue_inner_clear(
    gr_queue_t * self,
    volatile queue_inner * inner
)
{
    volatile gr_queue_item_t * p;

    // 工作线程干工作线程的，我干我的。

    p = inner->head;
    while( p ) {

        // 将所有项打上已经处理的标记
        if ( ! p->is_processed ) {
            p->is_processed = true;
        }

        if ( NULL == p->next )
            break;

        p = p->next;
    };
}

void
gr_queue_clear(
    gr_queue_t * self
)
{
    gr_queue_inner_clear( self, & self->emerge_queue );
    gr_queue_inner_clear( self, & self->normal_queue );
}

static inline
void
gr_queue_inner_del_processed(
    gr_queue_t * self,
    volatile queue_inner * inner
)
{
    // scan processed package

    if ( NULL == inner->curr && inner->head ) {

        // 工作线程没任务处理时我再删除

        volatile gr_queue_item_t * p;

        do {

            p = inner->head;
            if ( NULL == p ) {
                inner->tail = NULL;
                inner->curr = NULL;
                break;
            }

            if ( ! p->is_processed ) {
                // first processed item, done the scan process
                break;
            }

            // remove item from link table
            inner->head = inner->head->next;

            // call user function to delete the item
            self->free_item( self->callback_param, (gr_queue_item_t *)p );

        } while( 1 );
    }
}

static inline
int
gr_queue_inner_push(
    gr_queue_t * self,
    volatile queue_inner * inner,
    gr_queue_item_t * data
)
{
    // push package

    // initialize the item
    data->is_processed = false;
    data->next = NULL;

    // add to link table
    if ( NULL == inner->head ) {
        // empty link table
        assert( NULL == inner->tail );
        assert( NULL == inner->curr );
        inner->head = inner->tail = inner->curr = data;
    } else {
        // non empty link table
        //assert( inner->curr );
        assert( inner->tail );
        inner->tail->next = data;
        inner->tail = data;

        if ( NULL == inner->curr ) {
            if ( ! inner->head->is_processed ) {
                inner->curr = inner->head;
            }
        }
    }

    // if in waitting, wake up
    if ( self->in_event ) {
        self->in_event = false;
        gr_event_alarm( & self->event );
    }

    return 0;
}

int
gr_queue_push(
    gr_queue_t * self,
    gr_queue_item_t * data,
    bool is_emerge
)
{
    int r;

    gr_queue_inner_del_processed( self, & self->emerge_queue );
    gr_queue_inner_del_processed( self, & self->normal_queue );

    r = gr_queue_inner_push(
        self,
        is_emerge ? & self->emerge_queue : & self->normal_queue,
        data );

    return r;
}

static inline
bool
queue_inner_is_empty(
    gr_queue_t * self,
    volatile queue_inner * inner
)
{
    // 只要curr指针有效，肯定有未处理的数据
    return inner->curr ? false : true;
}

static inline
volatile gr_queue_item_t *
gr_queue_inner_top(
    gr_queue_t * self,
    volatile queue_inner * inner
)
{
    if ( NULL == inner->curr ) {
        return NULL;
    }

    // 将已经处理完的任务越过去
    while ( inner->curr ) {
        if ( inner->curr->is_processed ) {
            inner->curr = inner->curr->next;
            continue;
        } else {
            break;
        }
    }
    if ( NULL == inner->curr || inner->curr->is_processed ) {
        return NULL;
    }

    assert( NULL == self->curr_queue );

    self->curr_queue = inner;
    return inner->curr;
}

static inline
bool
gr_queue_inner_rebuild(
    gr_queue_t * self
)
{
    if ( self->emerge_queue.head ) {
        self->emerge_queue.curr = self->emerge_queue.head;
        assert( ! self->emerge_queue.curr->is_processed );
    }
    if ( self->normal_queue.head ) {
        self->normal_queue.curr = self->normal_queue.head;
        assert( ! self->normal_queue.curr->is_processed );
    }

    assert( self->emerge_queue.head || self->normal_queue.head );
    return (self->emerge_queue.head || self->normal_queue.head) ? true : false;
}

gr_queue_item_t *
gr_queue_top(
    gr_queue_t * self,
    uint32_t     wait_ms
)
{
    assert( self );

#if defined( WIN32 ) || defined( WIN64 )
    if ( 0 == self->pop_thread_id ) {
        self->pop_thread_id = GetCurrentThreadId();
    }
#endif

    while ( 1 ) {

        if ( self->need_exit ) {
            // caller need exit, so we exited
            self->is_exited = true;
            gr_event_alarm( & self->exited_event );
            return NULL;
        }

        if (   queue_inner_is_empty( self, & self->emerge_queue )
            && queue_inner_is_empty( self, & self->normal_queue ) )
        {
            if ( wait_ms > 0 ) {
                // wait forever
                self->in_event = true;
                gr_event_wait( & self->event, wait_ms );

                if ( self->need_exit ) {
                    // caller need exit, so we exited
                    self->is_exited = true;
                    gr_event_alarm( & self->exited_event );
                    return NULL;
                }

                if (   queue_inner_is_empty( self, & self->emerge_queue )
                    && queue_inner_is_empty( self, & self->normal_queue ) )
                {
                    return NULL;
                }

            } else {
                // link table empty
                return NULL;
            }
        } else {
            if ( ! self->need_exit ) {
                // do nothing. mostly we got this way.
            } else {
                // caller need exit, so we exited
                self->is_exited = true;;
                return NULL;
            }
        }

        if ( ! queue_inner_is_empty( self, & self->emerge_queue ) ) {

            // emerge queue 有数据
            gr_queue_item_t * r = (gr_queue_item_t *)gr_queue_inner_top(
                self, & self->emerge_queue );
            if ( r && ! r->is_processed ) {
                return r;
            }
            sleep_ms( 0 );
            continue;
        } else if ( ! queue_inner_is_empty( self, & self->normal_queue ) ) {
        
            // normal queue 有数据
            gr_queue_item_t * r = (gr_queue_item_t *)gr_queue_inner_top(
                self, & self->normal_queue );
            if ( r && ! r->is_processed ) {
                return r;
            }
            sleep_ms( 0 );
            continue;
        } else {
            // 没可能到这个分支
            assert( false );
            return NULL;
        }
    }
}

bool
gr_queue_pop_top(
    gr_queue_t * self,
    gr_queue_item_t *   confirm_item
)
{
    volatile gr_queue_item_t * p;
    volatile queue_inner * inner;

    assert( self );

    if ( NULL == self->curr_queue ) {
        gr_error( "item not found" );
        assert( self->curr_queue );
        return false;
    }

    inner = self->curr_queue;
    if ( NULL == inner->curr ) {
        return false;
    }

    self->curr_queue = NULL;

    p = inner->curr;
    if ( p != confirm_item ) {
        return false;
    }
    inner->curr = inner->curr->next;
    p->is_processed = true;
    return true;
}

bool
gr_queue_is_empty(
    gr_queue_t * self
)
{
    if ( NULL != self->emerge_queue.head || NULL != self->emerge_queue.tail ) {
        return false;
    }

    if ( NULL != self->normal_queue.head || NULL != self->normal_queue.tail ) {
        return false;
    }

    return true;
}
