/**
 * @file libgrocket/gr_event.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   event
 * Revision History
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-05    Created.
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
    o->cond = CreateEvent(
         NULL,    // no security attributes
         FALSE,   // Auto reset event
         FALSE,   // initially set to non signaled state
         NULL);   // un named event
    if ( NULL == o->cond ) {
        gr_fatal( "CreateEvent failed %d", get_errno() );
        return -1;
    }
    return 0;
#elif defined( __APPLE__ )
	int r;
    assert( o );
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
    return 0;
#elif defined( __linux )
    int r;
    assert( o );

    r = sem_init(
       & o->cond,   // handle to the event semaphore
       0,           // not shared
       0);          // initially set to non signaled state
    if ( -1 == r ) {
        gr_fatal( "sem_init failed %d", get_errno() );
        return -1;
    }
    return 0;
#endif
}

void
gr_event_destroy(
    gr_event_t * o
)
{
    assert( o );
#if defined( WIN32 ) || defined( WIN64 )
    assert( o->cond );
    CloseHandle( o->cond );
    o->cond = NULL;
#elif defined( __APPLE__ )
	pthread_cond_destroy( & o->cond );
	pthread_mutex_destroy( & o->lock );
#elif defined( __linux )
    sem_destroy( & o->cond );
#endif
}

bool
gr_event_alarm(
    gr_event_t * o
)
{
    assert( o );
#if defined( WIN32 ) || defined( WIN64 )
    return SetEvent( o->cond ) ? true : false;
#elif defined( __APPLE__ )
    return 0 == pthread_cond_signal( & o->cond ) ? true : false;
#elif defined( __linux )
    return 0 == sem_post( & o->cond ) ? true : false;
#endif
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
#elif defined( __linux )
    int             r;
    int             e;
    assert( o );

    if ( GR_EVENT_WAIT_INFINITE == ms ) {
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
    e = errno;
    gr_error( "sem_wait / sem_timedwait %d ms return %d, %d,%s", ms, r, e, strerror(e) );
    return -1;
#else
    #error  unknown platform
#endif
}
