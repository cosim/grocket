/**
 * @file libgrocket/gr_socket.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/09/29
 * @version $Revision$ 
 * @brief   SOCKET
 * Revision History
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-09-29    Created.
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

#include "gr_socket.h"
#include "gr_errno.h"
#include "gr_log.h"
#include "gr_global.h"
#include "gr_tools.h"

void
gr_socket_close(
	int sock
)
{
    if ( INVALID_SOCKET != sock ) {
#if defined( WIN32 ) || defined( WIN64 )
        closesocket( sock );
#else
        close( sock );
#endif
    } else {
        gr_error( "invalid params" );
    }
}

bool
gr_socket_get_tcp_no_delay(
	int sock,
	bool * isNoDelay
)
{
    byte_t flag = 0;
    socklen_t len = (socklen_t)sizeof( flag );
    if (getsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, & len) != SOCKET_ERROR) {
        if ( (int)sizeof( flag ) != len ) {
            gr_error( "len invalid %d", len );
            return false;
        }
        * isNoDelay = (1 == flag) ? true : false;
        return true;
    }

    gr_error( "failed %d", get_errno() );
    return false;
}

bool
gr_socket_set_tcp_no_delay(
	int sock,
	bool isNoDelay
)
{
    byte_t flag = isNoDelay ? 1 : 0;
    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag)) != SOCKET_ERROR) {
        return true;
    }

    gr_error( "failed %d", get_errno() );
    return false;
}

bool
gr_socket_set_keep_alive(
	int sock,
	bool isKeepAlive
)
{
    byte_t flag = isKeepAlive ? 1 : 0;
    if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char*)&flag, sizeof(flag)) != SOCKET_ERROR) {
        return true;
    }

    gr_error( "%s failed %d", get_errno() );
    return false;
}

bool
gr_socket_get_send_buf(
	int sock,
	int * bytes
)
{
    socklen_t len = (socklen_t)sizeof( int );
    if (getsockopt( sock, SOL_SOCKET, SO_SNDBUF, (char*) bytes, & len ) != SOCKET_ERROR) {
        if ( len != (int)sizeof(int) ) {
            gr_error( "len invalid %d", len );
            return false;
        }
        return true;
    }

    gr_error( "failed %d", get_errno() );
    return false;
}

bool
gr_socket_set_send_buf(
	int sock,
	int bytes
)
{
    if (setsockopt( sock, SOL_SOCKET, SO_SNDBUF, (char*)&bytes, sizeof(int) ) != SOCKET_ERROR) {
        return true;
    }

    gr_error( "setsockopt with SO_SNDBUF with %d failed %d,%s", bytes, get_errno(), strerror(get_errno()) );
    return false;
}

bool
gr_socket_get_recv_buf(
	int sock,
	int * bytes
)
{
    socklen_t len = (socklen_t)sizeof( int );
    if (getsockopt( sock, SOL_SOCKET, SO_RCVBUF, (char*)bytes, & len ) != SOCKET_ERROR) {
        if ( len != (int)sizeof( int ) ) {
            gr_error( "len invalid %d", len );
            return false;
        }
        return true;
    }

    gr_error( "failed %d", get_errno() );
    return false;
}

bool
gr_socket_set_recv_buf(
	int sock,
	int bytes
)
{
    if (setsockopt( sock, SOL_SOCKET, SO_RCVBUF, (char*)&bytes, sizeof(int) ) != SOCKET_ERROR) {
        return true;
    }

    gr_error( "failed %d", get_errno() );
    return false;
}

bool
gr_socket_set_ttl(
	int sock,
	int ttl
)
{
	// on Windows, IP_TTL in winsock.h, but we include winsock2.h

    if (setsockopt( sock, IPPROTO_IP, IP_TTL, (char*)&ttl, sizeof(int) ) != SOCKET_ERROR) {
        return true;
    }

    gr_error( "failed %d", get_errno() );
    return false;
}

bool
gr_socket_set_loopback(
    int sock,
    bool enable
)
{
    int loop = enable ? 1:0; 
    if( setsockopt( sock, IPPROTO_IP, IP_MULTICAST_LOOP, (const char*)& loop, sizeof( loop ) ) != SOCKET_ERROR ) {
        return true;
    }

    gr_error( "failed %d", get_errno() );
    return false;
}

bool
gr_socket_set_block(
	int sock,
	bool is_block
)
{
	if( is_block ) {
#if defined( WIN32 ) || defined( WIN64 )
		unsigned long arg = 0;
		ioctlsocket(sock, FIONBIO, &arg);
		return true;
#else
		int flags = fcntl(sock, F_GETFL);
		flags &= ~O_NONBLOCK;
        if ( -1 == fcntl(sock, F_SETFL, flags) ) {
            gr_error( "fcntl failed %d", get_errno() );
        	return false;
        }
        return true;
#endif
	} else {
#if defined( WIN32 ) || defined( WIN64 )
		unsigned long arg = 1;
		ioctlsocket(sock, FIONBIO, &arg);
		return true;
#else
		int flags = fcntl(sock, F_GETFL);
		flags |= O_NONBLOCK;
        if ( -1 == fcntl(sock, F_SETFL, flags) ) {
            gr_error( "fcntl failed %d", get_errno() );
			return false;
        }
        return true;
#endif
    }
}

bool
gr_socket_get_linger(
    int sock,
    uint16_t * lv
)
{
    struct linger lingerValue;
	socklen_t len = (socklen_t)sizeof( struct linger );

    if ( 0 == getsockopt(
        sock,
        SOL_SOCKET,
        SO_LINGER,
        (char *)&lingerValue, & len
    ) ) {
        if ( len != (int)sizeof( struct linger ) ) {
            gr_error( "invalid len %d", len );
            return false;
        }

        if ( ! lingerValue.l_onoff ) {
            * lv = 0;
            return true;
        }

        * lv = lingerValue.l_linger;
        return true;
    }

    gr_error( "failed %d", get_errno() );
    return false;
}

bool
gr_socket_set_linger(
    int sock,
    uint16_t lv
)
{
    struct linger lingerValue;

    // 不允许 l_onoff == 1 && l_linger > 0 的情况
    if ( 0 == lv ) {
        lingerValue.l_onoff = 0;
        // 这个值被忽略
        lingerValue.l_linger = 0;
    } else {
        lingerValue.l_onoff = 1;
        lingerValue.l_linger = lv;
    }

    if ( 0 == setsockopt(
        sock,
        SOL_SOCKET,
        SO_LINGER,
        (const char *)&lingerValue,
        sizeof(lingerValue)
    ) ) {
        return true;
    }

    gr_error( "failed %d", get_errno() );
    return false;
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
gr_socket_recv(
    int sock,
    void * buf,
    int bytes
)
{
    int r = recv( sock, (char*)buf, bytes, MSG_NOSIGNAL );
    if ( r <= 0 ) {
        if ( 0 == r ) {
            gr_error( "recv 0 bytes" );
        } else if ( ! gr_socket_in_progress() && ! gr_socket_would_block() ) {
            gr_error( "recv return %d: %d", r, get_errno() );
        }
    }

    return r;
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
            gr_error( "send 0 bytes" );
        } else if ( ! gr_socket_in_progress() && ! gr_socket_would_block() ) {
            gr_error( "send return %d: %d", r, get_errno() );
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
        gr_error( "invalid params" );
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
        gr_error( "select timeout %d", get_errno() );

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
        gr_error( "recv return %d, %d", * readed, get_errno() );
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
            gr_error( "socket_recv_inner failed" );
            return false;
        }

        if ( r < 0 ) {
            gr_error( "socket_recv_inner return %d bytes", r );
            return false;
        }
        if ( 0 == r ) {
            gr_error( "socket_recv_inner return %d bytes", r );
            break;
        }

        p += r;
        len -= r;
    }

    if ( 0 == len )
        return true;

    gr_error( "failed" );
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
                        gr_error( "select %d ms return %d. failed, %d", (int)timeout_ms, r, e );
                        return false;
                    }

                    continue;
                }
            }

            gr_error( "select failed, %d", get_errno() );
            return false;
        }

        p += r;
        bytes -= r;
    }

    return true;
}

bool
gr_socket_addr_v4(
    const char * host,
    int port,
    struct sockaddr_in * addr
)
{
    if ( NULL == host || '\0' == * host ) {
        gr_error( "invalid params" );
        return false;
    }
    if ( NULL == addr ) {
        gr_error( "invalid params" );
        return false;
    }

    if ( 0 == stricmp( "localhost", host ) )
        host = "127.0.0.1";

    memset( addr, 0, sizeof(struct sockaddr_in) );
    addr->sin_family = AF_INET;
    addr->sin_port = htons( (u_short)port );
    addr->sin_addr.s_addr = inet_addr( host );

    if(addr->sin_addr.s_addr == INADDR_NONE) {
        // Windows XP has getaddrinfo(), but we don't want to require XP to run Ss.
        // gethostbyname() is thread safe on Windows, with a separate hostent per thread
        struct hostent* entry;
        int retry = 5;
        do {
            entry = gethostbyname(host);
#ifdef _WIN32_WCE
        } while( 0 );
#elif defined( WIN32 )
        } while(entry == 0 && WSAGetLastError() == WSATRY_AGAIN && --retry >= 0);
#else
        } while(entry == 0 && errno == EAI_AGAIN && --retry >= 0);
#endif

        if(entry == 0) {
            // DNS 错
            gr_error( "DNS错 %d", get_errno() );
            return false;
        }
        memcpy( & addr->sin_addr, entry->h_addr, entry->h_length);
    }

    return true;
}
