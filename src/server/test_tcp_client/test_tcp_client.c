/**
 * @file test_tcp_client/test_tcp_client.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/19
 * @version $Revision$ 
 * @brief   
 * Revision History
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-19    Created.
 *       2     zouyueming   2013-10-21    fixed kill package count BUG.
 *       3     zouyueming   2013-10-22    add signature to each package.
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
#include "gr_stdinc.h"
#if defined( __APPLE__ )
    #include <mach/mach_time.h>
#endif

#if defined( WIN32 ) || defined( WIN64 )
    #pragma comment( lib, "ws2_32.lib" )
#endif

int
get_errno()
{
#if defined( WIN32 ) || defined( WIN64 )
    return (int)GetLastError();
#else
    return errno;
#endif
}

void
gr_socket_close(
	int sock
)
{
    printf( "close socket %d\n", sock );
    if ( INVALID_SOCKET != sock ) {
#if defined( WIN32 ) || defined( WIN64 )
        closesocket( sock );
#else
        close( sock );
#endif
    } else {
        printf( "invalid params\n" );
    }
}

bool
gr_socket_in_progress()
{
#if defined( WIN32 ) || defined( WIN64 )
    return WSAGetLastError() == WSAEINPROGRESS;
#else
    return EINPROGRESS == errno;
#endif
}

bool
gr_socket_would_block()
{
#if defined( WIN32 ) || defined( WIN64 )
    return WSAGetLastError() == WSAEWOULDBLOCK;
#else
    return EWOULDBLOCK == errno;
#endif
}

int
gr_socket_send(
    int sock,
    const void * buf,
    int bytes
)
{
	int r = send( sock, (const char *)buf, bytes, MSG_NOSIGNAL );
    if ( r <= 0 ) {
        if ( 0 == r ) {
            printf( "send 0 bytes\n" );
        } else if ( ! gr_socket_in_progress() && ! gr_socket_would_block() ) {
            printf( "send return %d: %d\n", r, get_errno() );
        }
    }

    return r;
}

static inline
bool
socket_recv_inner(
    int fd,
    char * buffer,
    int count,
    int * readed,
    size_t timeout,
    bool * is_timeout
)
{
    int rs;
    int _readed = 0;

    if ( NULL == readed )
        readed = & _readed;

    if ( is_timeout )
        * is_timeout = false;

    if ( 0 == count ) {
        * readed = 0;
        return true;
    }

    if ( NULL == buffer ) {
        * readed = 0;
        printf( "invalid params\n" );
        return false;
    }

    * readed = 0;

    if( timeout > 0 ) {

        fd_set _rFdSet;
        struct timeval tv;

#if ! defined( WIN32 ) && ! defined( WIN64 )
        assert( fd < FD_SETSIZE );
#endif

        FD_ZERO( & _rFdSet );
        FD_SET(fd, &_rFdSet);
        tv.tv_sec = (long)timeout / 1000;
        tv.tv_usec = ((long)timeout - tv.tv_sec * 1000) * 1000;
        rs = select( (int)fd + 1, &_rFdSet, 0, 0, &tv );
    } else {
        rs = 1;
    }

    if ( 1 != rs ) {
        printf( "select return %d\n", rs );

        if ( is_timeout ) {
            * is_timeout = true;
        }
        return false;
    }

#if defined( WIN32 ) || defined( WIN64 )
    * readed = recv( fd, buffer, count, 0 );
#else
    * readed = recv( fd, buffer, count, MSG_NOSIGNAL );
#endif

    if ( * readed <= 0 ) {
        printf( "recv return %d, %d\n", * readed, get_errno() );
        * readed = 0;
        return false;
    }

    return true;
}

bool
gr_socket_recv_fill(
    int sock,
    void * buf,
    int bytes,
    size_t timeout_ms,
    bool * is_timeout
)
{
    bool _is_timeout;
    char * p = (char*)buf;
    int len = bytes;
    int r = 0;

    if ( NULL == is_timeout )
        is_timeout = & _is_timeout;

    while( len > 0 ) {

        if ( ! socket_recv_inner( sock, p, len, & r, timeout_ms, is_timeout ) ) {
            printf( "socket_recv_inner failed\n" );
            return false;
        }

        if ( r < 0 ) {
            printf( "socket_recv_inner return %d bytes\n", r );
            return false;
        }
        if ( 0 == r ) {
            printf( "socket_recv_inner return %d bytes\n", r );
            break;
        }

        p += r;
        len -= r;
    }

    if ( 0 == len )
        return true;

    printf( "failed\n" );
    return false;
}

bool
gr_socket_send_all(
    int sock,
    const void * buf,
    int bytes,
    bool is_async_socket,
    size_t timeout_ms
)
{
    int r;
    const char * p = (const char *)buf;

    if ( NULL == p || 0 == bytes )
        return true;

    while( bytes > 0 ) {

        r = gr_socket_send( sock, p, bytes );
        if( 0 == r ) {
            return false;
        } else if ( r < 0 ) {
#if ! defined( WIN32 ) && ! defined( WIN64 )
            if ( EINTR == errno )
                continue;
#endif

            if ( is_async_socket ) {

                if ( gr_socket_in_progress() || gr_socket_would_block() ) {

                    fd_set w;
                    struct timeval tv;

                    FD_ZERO( & w );
                    FD_SET( sock, & w );
                    tv.tv_sec = (long)timeout_ms / 1000;
                    tv.tv_usec = ((long)timeout_ms - tv.tv_sec * 1000) * 1000;
                    r = select( (int)sock + 1, 0, & w, 0, &tv );
                    if ( 1 != r ) {
                        int e = get_errno();
                        printf( "select %d ms return %d. failed, %d\n", (int)timeout_ms, r, e );
                        return false;
                    }

                    continue;
                }
            }

            printf( "select failed, %d\n", get_errno() );
            return false;
        }

        p += r;
        bytes -= r;
    }

    return true;
}

uint32_t get_tick_count()
{
#if defined( WIN32 ) || defined( WIN64 )

    return GetTickCount();

#elif defined( __linux )

    struct timespec ts;
    if ( 0 != clock_gettime( CLOCK_MONOTONIC, & ts ) ) {
        printf( "clock_gettime failed %d\n", get_errno() );
        return (uint32_t)0;
    }

    // 这个折返的机率和GetTickCount是一样的
    return (uint32_t)( ts.tv_sec * 1000L + ts.tv_nsec / 1000000L ); 

#elif defined( __APPLE__ )

    /*
    mach_absolute_time函数返回的值是启动后系统CPU/Bus的clock一个tick数
    因为这个 GetTickCount 是系统启动后的毫秒数，所以要获得系统启动后的时间需要
    进行一次转换，还好Apple给出了一个官方的方法
    Technical Q&A QA1398
    Mach Absolute Time Units
    https://developer.apple.com/library/mac/#qa/qa1398/_index.html
    另外一些关于这个函数的说明文档如下：
    http://www.macresearch.org/tutorial_performance_and_time
        mach_absolute_time is a CPU/Bus dependent function that returns a value
    based on the number of "ticks" since the system started up.
    uint64_t mach_absolute_time(void);
        Declared In: <mach/mach_time.h>
        Dependency: com.apple.kernel.mach
        This function returns a Mach absolute time value for the current wall
    clock time in units of uint64_t.
    https://developer.apple.com/library/mac/#documentation/Performance/Conceptual/LaunchTime/Articles/MeasuringLaunch.html
        mach_absolute_time reads the CPU time base register and is the basis
    for other time measurement functions.
    */
    static mach_timebase_info_data_t g_info;

    if ( 0 == g_info.denom ) {
        if ( KERN_SUCCESS != mach_timebase_info( & g_info ) ) {
            printf( "mach_timebase_info failed %d\n", get_errno() );
            return (uint32_t)0;
        }
    }

    // 这个折返的机率和GetTickCount是一样的，只要乘的时候别溢出就行
    return (uint32_t)( mach_absolute_time() * g_info.numer / g_info.denom / 1000000L );

#else

    // 这个应该是速度最慢的
    struct timeval tv;
    if ( 0 != gettimeofday( & tv, NULL ) ) {
        printf( "gettimeofday failed %d\n", get_errno() );
        return (uint32_t)0;
    }

    //TODO: 折返了怎么办？
    return (uint32_t)( tv.tv_sec * 1000L + tv.tv_usec / 1000L );    
#endif
}

#if defined( WIN32 ) || defined( WIN64 )
    typedef HANDLE      pthread_t;
#endif

int
gr_thread_create(
    pthread_t * thread,
    void *(*start_routine)(void*),
    void * arg
)
{
#if defined( WIN32 ) || defined( WIN64 )
    DWORD tid;

    * thread = CreateThread(
	    0,
        (unsigned int)0,
        (LPTHREAD_START_ROUTINE)start_routine,
        (LPVOID)arg,
        0 /*CREATE_SUSPENDED*/,
        & tid
    );
    if ( NULL == * thread ) {
        printf( "CreateThread failed: %d\n", get_errno() );
        return -2;
    }

    return 0;
#else
    int r;
    int e;
    r = pthread_create( thread, NULL, start_routine, arg );
    e = errno;
    if ( 0 != r ) {
        printf( "pthread_create failed: %d,%s\n", e, strerror(e) );
        return -2;
    }
    return 0;
#endif
}

void
gr_thread_join(
    pthread_t * thread
)
{
#if defined( WIN32 ) || defined( WIN64 )
    if ( thread && * thread ) {
        HANDLE h = * thread;
        WaitForSingleObject( h, INFINITE );
        CloseHandle( h );
        * thread = NULL;
    }
#else
    pthread_join( * thread, NULL );
#endif
}

int tcp_main( int argc, char ** argv, int thread_id )
{
#if defined( WIN32 ) || defined( WIN64 )
	WSADATA wsaData;
	DWORD ret;
#endif
    int port;
    const char * host;
    int fd;
    struct sockaddr_in addr;
    int r;
    char * req;
    size_t req_len;
    size_t rsp_max;
    char * rsp;
    size_t rsp_len;
    uint32_t i;
    uint64_t sent = 0;
    uint64_t recved = 0;
    uint32_t start = 0;
    uint32_t stop = 0;

    uint32_t exam_unit = 100000;
    uint32_t exam_count = 0;
    uint32_t exam_time_sum = 0;

#if defined( WIN32 ) || defined( WIN64 )
    // 加载 Winsock 2.2
    if ((ret = WSAStartup(0x0202, &wsaData)) != 0) {
        printf( "thread %d: WSAStartup failed\n", thread_id );
        return -1;
    }
#endif

	port = 8000;
	host = "127.0.0.1";
	if ( argc >= 2 )
		host = argv[ 1 ];
	if ( argc >= 3 )
		port = atoi( argv[ 2 ] );

    fd = (int)socket( PF_INET, SOCK_STREAM, IPPROTO_TCP );
    if ( -1 == fd ) {
        printf( "thread %d: create socket failed\n", thread_id );
        return -2;
    }

    memset( & addr, 0 , sizeof( addr ) );
    addr.sin_family = AF_INET;
    addr.sin_port = htons( port );
    addr.sin_addr.s_addr = inet_addr( host );

    r = connect( fd, (const struct sockaddr *)& addr, (int)sizeof( addr ) );
    if ( 0 != r ) {
        printf( "thread %d: connect %s:%d failed\n",
            thread_id, host, port );
        return -2;
    }

    rsp_max = 1024 * 1024;
    rsp = (char *)malloc( rsp_max );
    if ( NULL == rsp ) {
        printf( "thread %d: malloc %d failed\n", thread_id, (int)rsp_max );
        return -3;
    }
    rsp_len = 0;

    req = (char *)malloc( rsp_max );
    if ( NULL == req ) {
        printf( "thread %d: malloc %d failed\n", thread_id, (int)rsp_max );
        return -4;
    }

    req_len = 44;
    memset( req, 'A', req_len );
    req[ req_len ] = 0;

    start = get_tick_count();

    for ( i = 0; ; ++ i ) {

        char id[ 32 ];
        int len = sprintf( id, "%d ", i + 1 );

        const char * rreq = req;
        size_t rreq_len = req_len;

        strcpy( req, id );

        if ( ! gr_socket_send_all( fd, rreq, (int)rreq_len, false, 1000 ) ) {
            printf( "thread %d: send %d bytes failed\n", thread_id, (int)rreq_len );
            break;
        }
        sent += rreq_len;

        //printf( "\r wait for #%d kiss package ...", i ); fflush( stdout );

        rsp_len = req_len;
        if ( ! gr_socket_recv_fill( fd, & rsp[ 0 ], (int)rsp_len, 1000, NULL ) ) {
            printf( "thread %d: recv %d bytes failed\n", thread_id, (int)rsp_len );
            break;
        }
        recved += rsp_len;

        if ( 0 != memcmp( rsp, req, rsp_len ) ) {
            printf( "thread %d: compare failed: %s != %s\n", thread_id, req, rsp );
            break;
        }

        if ( 0 != i && 0 == (i % exam_unit) ) {
            stop = get_tick_count();

            ++ exam_count;
            exam_time_sum += stop - start;

            printf( "thread %d: %d kiss package, send %llu bytes, recv %llu bytes. "
                    "%u package use %d ms, avg: %u/%u=%u ms, %d REQ/MS\t\n",
                    thread_id, i + 1, sent, recved,
                    exam_unit, stop - start,
                    exam_time_sum, exam_count, exam_time_sum / exam_count,
                    exam_unit / (exam_time_sum / exam_count) );
            fflush( stdout );

            start = get_tick_count();
        }
    }

    gr_socket_close( fd );

    free( rsp );

    return 0;
}

static int      g_argc  = 0;
static char **  g_argv  = NULL;

void * tcp_thread( void * params )
{
    int thread_id = (int)params;

    tcp_main( g_argc, g_argv, thread_id );

    return NULL;
}

int main( int argc, char ** argv )
{
    pthread_t * threads;
    int         thread_count;
    int         i;
    int         r = 0;

    g_argc = argc;
    g_argv = argv;

    thread_count = 10;

    threads = (pthread_t *)calloc( 1, sizeof( pthread_t ) * thread_count );
    if ( NULL == threads ) {
        printf( "calloc failed\n" );
        return -1;
    }

    do {
        for ( i = 0; i < thread_count; ++ i ) {
            r = gr_thread_create( & threads[ i ], tcp_thread, (void*)i );
            if ( 0 != r ) {
                printf( "start #%d thread failed %d\n", i, r );
                break;
            }
        }
        if ( 0 != r ) {
            break;
        }

        for ( i = 0; i < thread_count; ++ i ) {
            gr_thread_join( & threads[ i ] );
        }

    } while ( false );

    free( threads );

    printf( "will exit, press any key to exit\n" );
    getchar();
    return r;
}
