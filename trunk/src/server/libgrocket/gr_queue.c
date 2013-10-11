/**
 * @file librocket/gr_queue.c
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
#include "gr_queue.h"
#include "gr_log.h"
#include "gr_global.h"
#include "gr_event.h"
#include "gr_mem.h"
#include "gr_tools.h"
#include <assert.h>

// һ�������Ķ��еĶ��������Ǹ�������
typedef struct queue_inner
{
    // �б�ͷ�������߳�ά���������̲߳��ܷ���
    // һ�������߳�ɾ�����ѱ����Ϊ�Ѵ����������ʱ��
    // ��ָ��ᱻ�޸�
    volatile gr_queue_item_t *  head;

    // ��ǰ����ָ�룬�����̸߳����ʼ���������̸߳���ı������յ�NULL��
    // �ó�Ա���ڴ����̼߳�¼�Լ���ǰ���ڴ���������
    // ��Ϊ�����������������Ѵ����ǣ�����δɾ����
    // �����̲߳���ÿ�δ�����ʱ����δ������Ŀ�ʼλ��
    volatile gr_queue_item_t *  curr;

    // �б�β�������߳�ά���������̲߳��ܷ���
    // �ó�Ա���������β����������
    volatile gr_queue_item_t *  tail;

} queue_inner;

struct gr_queue_t
{
    // gr_queue_top ��û����ʱʹ�õĵȴ��¼�
    gr_event_t                  event;

    // �˳�֪ͨ
    gr_event_t                  exited_event;

    // ��������
    volatile queue_inner        emerge_queue;
    // �������
    volatile queue_inner        normal_queue;

    // ���ù�gr_queue_top�󣬼�¼ȡ���Ķ������ڵĶ��У�gr_queue_pop_top����ʹ��
    volatile queue_inner *      curr_queue;

    // �ص�������һ���������û��������ģ��Ҹ���Ѿ����ȥ
    void *                      callback_param;

    // �û��������Ļص�����
    void ( * free_item )( void * param, gr_queue_item_t * p );

    // ��ǰ�Ƿ���Top����ʹ��event_wait�����ȴ��Ĺ����С�����ǣ���ѹ��ʱ��ʹ��event_alaram����
    volatile bool               in_event;

    // ��ǰ�Ƿ���Ҫ�߳��˳�������Ǹ�Top���ź�
    volatile bool               need_exit;

    // ��ǰ�Ƿ����߳��Ѿ��˳���
    volatile bool               is_exited;
    
#if defined( WIN32 ) || defined( WIN64 )
	// ��¼��Top��PopTop���߳�ID
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

        // �����˻ָ�һ��״̬
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

    // �����̸߳ɹ����̵߳ģ��Ҹ��ҵġ�

    p = inner->head;
    while( p ) {

        // ������������Ѿ�����ı��
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

        // �����߳�û������ʱ����ɾ��

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
    // ֻҪcurrָ����Ч���϶���δ���������
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

    // ���Ѿ������������Խ��ȥ
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

            // emerge queue ������
            gr_queue_item_t * r = (gr_queue_item_t *)gr_queue_inner_top(
                self, & self->emerge_queue );
            if ( r && ! r->is_processed ) {
                return r;
            }
            sleep_ms( 0 );
            continue;
        } else if ( ! queue_inner_is_empty( self, & self->normal_queue ) ) {
        
            // normal queue ������
            gr_queue_item_t * r = (gr_queue_item_t *)gr_queue_inner_top(
                self, & self->normal_queue );
            if ( r && ! r->is_processed ) {
                return r;
            }
            sleep_ms( 0 );
            continue;
        } else {
            // û���ܵ������֧
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
