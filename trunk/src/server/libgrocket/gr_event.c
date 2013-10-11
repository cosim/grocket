/**
 * @file libgrocket/gr_event.c
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
#include "gr_event.h"
#include "gr_log.h"
#include "gr_global.h"
#include "gr_tools.h"
#if ! defined( WIN32 ) && ! defined( WIN64 )
    #include <time.h>       // clock_gettime
    #include <sys/time.h>   // gettimeofday
    #include <errno.h>
#endif

int
gr_event_create(
    gr_event_t * o
)
{
#if defined( WIN32 ) || defined( WIN64 )
    assert( o );
    o->is_inited = false;
    o->cond = CreateEvent(
         NULL,    // no security attributes
         FALSE,   // Auto reset event
         FALSE,   // initially set to non signaled state
         NULL);   // un named event
    if ( NULL == o->cond ) {
        gr_fatal( "CreateEvent failed %d", get_errno() );
        return -1;
    }
    o->is_inited = true;
    return 0;
#elif defined( __APPLE__ )
	int r;
    assert( o );
    o->is_inited = false;
	r = pthread_mutex_init( & o->lock, NULL );
    if ( 0 != r ) {
        gr_fatal( "pthread_mutex_init failed %d", get_errno() );
		return -1;
    }

	r = pthread_cond_init( & o->cond, NULL );
    if ( 0 != r ) {
        gr_fatal( "pthread_cond_init failed %d", get_errno() );
        pthread_mutex_destroy( & o->lock );
		return -2;
    }
    o->is_inited = true;
    return 0;
#elif defined( __linux )
    int r;

    #if defined( USE_EVENT_FD )

    int flags;
    assert( o );
    o->is_inited = false;

    // EFD_NONBLOCK (since Linux 2.6.27)
    o->cond = eventfd( 0, 0 /*EFD_NONBLOCK*/ );
    if ( -1 == o->cond ) {
        gr_fatal( "eventfd failed %d", get_errno() );
        return -1;
    }

    flags = fcntl( o->cond, F_GETFL );
	flags &= ~O_NONBLOCK;
    if ( -1 == fcntl( o->cond, F_SETFL, flags ) ) {
        gr_error( "fcntl failed %d", get_errno() );
        close( o->cond );
        return false;
    }

    #else

    assert( o );
    o->is_inited = false;

    r = sem_init(
       & o->cond,   // handle to the event semaphore
       0,           // not shared
       0);          // initially set to non signaled state
    if ( -1 == r ) {
        gr_fatal( "sem_init failed %d", get_errno() );
        return -1;
    }
    #endif

    o->is_inited = true;
    return 0;
#endif
}

void
gr_event_destroy(
    gr_event_t * o
)
{
    assert( o );
    if ( o->is_inited ) {
#if defined( WIN32 ) || defined( WIN64 )
        assert( o->cond );
        CloseHandle( o->cond );
        o->cond = NULL;
#elif defined( __APPLE__ )
	    pthread_cond_destroy( & o->cond );
	    pthread_mutex_destroy( & o->lock );
#elif defined( __linux )
    #if defined( USE_EVENT_FD )
        close( o->cond );
        o->cond = -1;
    #else
        sem_destroy( & o->cond );
    #endif
#endif
        o->is_inited = false;
    } else {
        gr_error( "invalid params" );
    }
}

bool
gr_event_alarm(
    gr_event_t * o
)
{
    assert( o );
    if ( o->is_inited ) {
#if defined( WIN32 ) || defined( WIN64 )
        return SetEvent( o->cond ) ? true : false;
#elif defined( __APPLE__ )
        return 0 == pthread_cond_signal( & o->cond ) ? true : false;
#elif defined( __linux )
    #if defined( USE_EVENT_FD )
        //TODO: 异步，返回值要和文档确认一下
        uint64_t n = 0;
        write( o->cond, & n, sizeof( n ) );
        return true;
    #else
        return 0 == sem_post( & o->cond ) ? true : false;
    #endif
#endif
    } else {
        gr_error( "invalid params" );
        return false;
    }
}

int
gr_event_wait(
    gr_event_t * o,
    unsigned int ms
)
{
#if defined( WIN32 ) || defined( WIN64 )
    DWORD f;
    assert( o );

    if ( ! o->is_inited ) {
        gr_fatal( "invalid params" );
        return -1;
    }

    if ( GR_EVENT_WAIT_INFINITE == ms ) {
        f = WaitForSingleObject( o->cond, INFINITE );
    } else {
        f = WaitForSingleObject( o->cond, ms );
    }
    switch( f ) { 
    case WAIT_OBJECT_0:
        return 1;
    case WAIT_TIMEOUT:
        return 0;
    }

    gr_error( "WaitForSingleObject return %d, %d", (int)f, get_errno() );
    return -1;
#elif defined( __APPLE__ )
	int r;
    struct timeval tv;
    struct timespec to;
    
    assert( o );

    if ( ! o->is_inited ) {
        gr_error( "invalid params" );
        return -1;
    }
    
    if ( GR_EVENT_WAIT_INFINITE == ms ) {
        pthread_mutex_lock( & o->lock );
        r = pthread_cond_wait( & o->cond, & o->lock );
        pthread_mutex_unlock( & o->lock );
    } else {    
        if ( 0 != gettimeofday( & tv, NULL ) ) {
            gr_fatal( "gettimeofday failed: %d", get_errno() );
            return -1;
        }

        to.tv_sec = tv.tv_sec + ms / 1000;
        to.tv_nsec = tv.tv_usec * 1000 + (ms % 1000) * 1000000;
        if ( to.tv_nsec > 1000000000 ) {
            ++ to.tv_sec;
            to.tv_nsec -= 1000000000;
        }
        
        pthread_mutex_lock( & o->lock );
        r = pthread_cond_timedwait( & o->cond, & o->lock, & to );
        pthread_mutex_unlock( & o->lock );
    }

    switch( r ) {
    case 0:
        return 1;
    case ETIMEDOUT:
        return 0;
    case EINTR:
        return -2;
    }

    gr_error( "pthread_cond_timedwait(%d) return %d,%s",
        ms, r, strerror(r) );
    return -1;
#elif defined( __linux ) && defined( USE_EVENT_FD )
    int             r;
    struct timeval  timeout;
    fd_set          rdset;
    assert( o );
    if ( ! o->is_inited ) {
        gr_fatal( "invalid params" );
        return -1;
    }

    // 构造timeout对象，如果等的时间为0，也支持
    if ( GR_EVENT_WAIT_INFINITE == ms ) {
        //TODO: 这地方等一年
        ms = 1000 * 60 * 60 * 24 * 365;
    }
    timeout.tv_sec = ms / 1000;
    timeout.tv_usec = ( ms % 1000 ) * 1000;

    FDSET_ZERO( & rdset );
    FDSET_SET( o->cond, & rdset );

    r = select( o->cond + 1, & rdset, NULL, NULL, & timeout );
    if ( r < 0 ) {
        // 出错了
        gr_error( "select return %d, %d", r, get_errno() );
        return -1;
    }
    if ( 0 == r ) {
        // 没有数据事件
        return 0;
    }

    return 1;
#elif defined( __linux ) && ! defined( USE_EVENT_FD )
    int             r;
    int             e;
    assert( o );
    if ( ! o->is_inited ) {
        gr_fatal( "invalid params" );
        return -1;
    }

    if ( 0 == ms ) {
        r = sem_wait( & o->cond );
    } else {
        struct timeval tv;
        struct timespec to;
    
        if ( 0 != gettimeofday( & tv, NULL ) ) {
            gr_fatal( "gettimeofday failed: %d", get_errno() );
            return -1;
        }

        to.tv_sec = tv.tv_sec + ms / 1000;
        to.tv_nsec = tv.tv_usec * 1000 + (ms % 1000) * 1000000;
        if ( to.tv_nsec > 1000000000 ) {
            ++ to.tv_sec;
            to.tv_nsec -= 1000000000;
        }

        r = sem_timedwait( & o->cond, & to );
    }
    if ( 0 == r ) {
        return 1;
    }

    switch( errno ) {
    case 0:
        return 1;
    case ETIMEDOUT:
        return 0;
    case EINTR:
        return -2;
    }

//TODO: error   sem_wait / sem_timedwait return -1, 22,Invalid argument (gr_event.c:312:gr_event_wait)
    e = get_errno();
    gr_error( "sem_wait / sem_timedwait %d ms return %d, %d,%s", ms, r, e, strerror(e) );
    return -1;
#else
    #error  unknown platform
#endif
}
