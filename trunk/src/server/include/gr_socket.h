/**
 * @file include/gr_socket.h
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

#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_SOCKET_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_SOCKET_H_

#include "gr_stdinc.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined( WIN32 ) || defined( WIN64 )
    #ifdef _WIN32_WCE
        #include <Ws2tcpip.h>
        #pragma comment( lib, "ws2.lib" )
    #else
        #include <Iphlpapi.h>   // GetAdapterInfo
        #include <Sensapi.h>
        #pragma comment( lib, "Iphlpapi.lib" )
        #pragma comment( lib, "Sensapi.lib" )
        #pragma comment( lib, "ws2_32.lib" )
    #endif
#else
	#include <unistd.h>
	#include <fcntl.h>
	#include <sys/socket.h>
	#include <sys/poll.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <arpa/inet.h>
	#include <netdb.h>

	#if defined(__linux) || defined(__APPLE__) || defined(__FreeBSD__)
		#include <ifaddrs.h>
	#else
		#include <sys/ioctl.h>
		#include <net/if.h>
		#ifdef __sun
			#include <sys/sockio.h>
		#endif
	#endif
#endif

#ifdef __sun
	#define INADDR_NONE (in_addr_t)0xffffffff
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief close socket
 * @param[in] SOCKET sock: socket fd that will be close
 */
void
gr_socket_close(
	int sock
);

/**
 * @brief Is TCP use delay algorithem? 
 * @param[in] SOCKET sock: SOCKET fd
 * @param[in] bool isNoDelay: is it no delay? if true,
 *                            single send call will be fire a real send.
 * @return bool: is OK?
 */
bool
gr_socket_get_tcp_no_delay(
	int sock,
	bool * isNoDelay
);

bool
gr_socket_set_tcp_no_delay(
	int sock,
	bool isNoDelay
);

/**
 * @brief Is TCP use KeepAlive?
 * @param[in] SOCKET sock: SOCKET fd
 * @param[in] bool isKeepAlive: is it KeepAlive
 * @return bool: is OK?
 */
bool
gr_socket_set_keep_alive(
	int sock,
	bool isKeepAlive
);

/**
 * @brief set send buffer
 * @param[in] SOCKET sock: SOCKET fd
 * @param[in] int bytes: send buffer bytes
 * @return bool: is OK?
 */
bool
gr_socket_get_send_buf(
	int sock,
	int * bytes
);

bool
gr_socket_set_send_buf(
	int sock,
	int bytes
);

/**
 * @brief set recv buffer
 * @param[in] SOCKET sock: SOCKET fd
 * @param[in] int bytes: recv buffer bytes
 * @return bool: is OK?
 */
bool
gr_socket_get_recv_buf(
	int sock,
	int * bytes
);

bool
gr_socket_set_recv_buf(
	int sock,
	int bytes
);

/**
 * @brief set TTL
 * @param[in] SOCKET sock: SOCKET fd
 * @param[in] int ttl: TTL
 * @return bool: is OK?
 */
bool
gr_socket_set_ttl(
	int sock,
	int ttl
);

bool
gr_socket_set_loopback(
    int sock,
    bool enable
);

/**
 * @brief set sync or async SOCKET
 * @param[in] SOCKET sock: SOCKET fd
 * @param[in] bool is_block: is_block
 * @return bool: is OK?
 */
bool
gr_socket_set_block(
	int sock,
	bool is_block
);

bool
gr_socket_get_linger(
    int sock,
    uint16_t * lv
);

bool
gr_socket_set_linger(
    int sock,
    uint16_t linger
);

/**
 * @brief if last socket call failed, is it because E_INPROGRESS?
 * @param[in] SOCKET sock: SOCKET fd
 * @return bool: yes or no
 */
bool
gr_socket_in_progress();

/**
 * @brief if last socket call failed, is it because E_WOULDBLOCK
 * @param[in] SOCKET sock: SOCKET fd
 * @return bool: yes or no
 */
bool
gr_socket_would_block();

/**
 * @brief same as socket recv function
 * @param[in] SOCKET sock: SOCKET fd
 * @param[in] void * buf: recv buffer
 * @param[in] int bytes: recv buffer bytes
 * @return int: readed bytes, < 0 if failed
 */
int
gr_socket_recv(
	int sock,
	void * buf,
	int bytes
);

/**
 * @brief same as socket send function
 * @param[in] SOCKET sock: SOCKET fd
 * @param[in] void * buf: data pointer that will be send
 * @param[in] int bytes: data bytes
 * @return int: sent bytes
 */
int
gr_socket_send(
	int sock,
	const void * buf,
	int bytes
);

bool
gr_socket_recv_fill(
    int sock,
    void * buf,
    int bytes,
    size_t timeout_ms,
    bool * is_timeout
);

/**
 * @brief send all data
 * @param[in] SOCKET sock: SOCKET fd
 * @param[in] void * buf: data pointer that will be send
 * @param[in] int bytes: data bytes
 * @return bool: is all sent
 */
bool
gr_socket_send_all(
	int sock,
	const void * buf,
	int bytes,
    bool is_async_socket,
    size_t timeout_ms
);

/**
 * @brief construct a IPV4 address
 * @param[in] const char * host: host
 * @param[in] int port: port
 * @param[out] sockaddr_in * addr: result
 * @return bool: is it OK
 */
bool
gr_socket_addr_v4(
    const char * host,
    int port,
    struct sockaddr_in * addr
);

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_SOCKET_H_
