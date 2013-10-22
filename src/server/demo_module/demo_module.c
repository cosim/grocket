/**
 * @file demo_module/demo_module.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/09/27
 * @version $Revision$ 
 * @brief   server framework demo server module
 * Revision History
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-09-27    Created.
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

///////////////////////////////////////////////////////////////////////
//
// grocket.h要求我在包含它之前保证
// uint16_t, uint32_t, socklen_t, bool, size_t, sockaddr_in, sockaddr_in6这几
// 个数据类型有效。我本应该直接#include "gr_stdinc.h"搞定一切，但为了表示grocket.h的
// “纯洁”，我手工在这里实现。所以这是一段“蛋疼”的代码，如果计算行数的话，它当然不
// 包括在内。
//
// 蛋疼开始
// 
#include <stdio.h>  // 随便包含一个头文件，这样 size_t 类型就肯定有了

#include <string.h> // for strcmp
#include <assert.h> // for assert
#include <stdlib.h> // for malloc. cross windows, linux, OS X(bsd)

#if defined(WIN32) || defined(WIN64)

    // for sockaddr_in
    #include <WinSock2.h>
    // for sockaddr_in6
    #include <WS2tcpip.h>
    #include <Windows.h>
    #pragma comment( lib, "ws2_32.lib" )

    // Windows no socklen_t, 它通常用在网络程序中传递sockaddr地址长度时使用，int是兼容定义。

    typedef int                 socklen_t;
    typedef unsigned short      uint16_t;
    typedef unsigned int        uint32_t;

#elif defined(__linux)

    // 在linux下，sockaddr_in和sockaddr_in6在in.h
    #include <netinet/in.h>

#elif defined( __APPLE__ )

    // 在 Mac 下，uint16_t 什么的定义在stdint.h
    #include <stdint.h>
    // 在 Mac 下，socklen_t 声明在netdb.h
    #include <netdb.h>
#endif

#if ! defined(__cplusplus)
    // 在支持C99以前的C编译器是没有bool类型的，我必须要把它定义成一个字节

    typedef unsigned char   bool;
    #define true    1
    #define false   0
#endif

#if defined(WIN32) || defined(WIN64)
// 我想在这个demo里打出进程ID，在windows里没有getpid函数，自己提供一个

static int getpid()
{
    return (int)GetProcessId( GetCurrentProcess() );
}
#endif

//
// 蛋疼结束
//
///////////////////////////////////////////////////////////////////////

// 写服务器模块唯一需要包含的文件，不需要链接任何库。这行算行数。
#include "grocket.h"

// 服务器框架提供给用户模块的接口指针。这行算行数。
gr_server_t *   g_server = NULL;

// 版本号检查，框架的最低兼容版本号必须小于或等于模块开发者使用的开发框架的版本号。该判断算行数。
// 这个检查，使得可以直接升级甚至降级框架的二进制文件，而模块的兼容性检查只需要看模块是否能被成功装载。
// 服务框架接口兼容性检查失败。直接初始化失败，省得在运行时core增加追查成本
// 本函数算行数。它是要求必须实现的，这个函数无论在任何模块中也完全不需要修改。
void gr_version(
    int *               gr_server_version
)
{
    * gr_server_version = GR_SERVER_VERSION;
}

// 注意这个函数是可选的，你可以不实现它。
// 初始化函数。

int gr_init(
    gr_process_type_t   proc_type,
    gr_server_t *       server
)
{
    // 主进程初始化、子进程初始化、所有线程初始化时都会调用本函数。
    // 那如何区分呢？答案是proc_type参数，它的取值会有如下几种：
    // 1、GR_PROCESS_MASTER 则表示是主进程, 适用于主进程初始化资源, 子进程使用资源的场景
    // 2、GR_PROCESS_CHILD 正常的工作进程
    // 3、GR_PROCESS_THREAD_1 表示工作进程中的第一个工作线程初始化
    // 4、GR_PROCESS_THREAD_1 + n 表示工作进程中的第 n 个工作线程初始化

    // 前面说：“写服务器模块唯一需要包含的文件，不需要链接任何库”，那服务器框架导出函数的实现
    // 在哪里？答案是：在服务器框架进程里，它在gr_init函数中通过gr_server_t *接口以函数指针的
    // 方式暴露给用户模块，所以用户模块当然要把这个指针保存起来。这行算行数。
    g_server = server;

    {
        int n;
        gr_i_server_t * o = g_server->library->buildin;

        // 调用它的config_get_int从服务器配置文件读取[server]段的tcp.accept.concurrent的值
        n = o->config_get_int( o, "server", "tcp.accept.concurrent", 0 );
        printf( "I guess, [server]tcp.accept.concurrent = %d, is it right? haha!!!!\n", n );

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // 这就是这套服务器框架的一个特色，它提供类功能库给你用，还不要求你连接任何库。
        // 还支持你实现自己的功能函数库，以动态库的形式挂在服务器框架中供其它人使用，
        // 当然使用者依然不需要连接任何库！！
        // 当功能实现修改、更新、增加新函数都不会需要你的程序重新连接。
    }

    // 如果你很好奇，可以：
    // 1、通过server->argc和server_argv取得命令行参数
    // 2、通过server->worker_count取得worker数量
    // 3、通过server->ports_count取得监听端口数量
    // 4、通过server->ports取得每一个端口的配置
    // 如下代码是示例

    if ( GR_PROCESS_MASTER == proc_type || GR_PROCESS_CHILD == proc_type ) {
        int i;
        char buf[ 1024 ];
        char * log = (char *)malloc( 1024 * 1024 );
        if ( NULL != log ) {

            char pt[ 32 ] = "";
            const char * addr = NULL;

            * log = '\0';

            if ( GR_PROCESS_MASTER == proc_type ) {
                strcpy( pt, "MASTER" );
            } else {
                strcpy( pt, "CHILD" );
            }
            sprintf( buf, "[pid=%d] gr_init called, proc_type = %d(%s)\n", getpid(), (int)proc_type, pt );
            strcat( log, buf );

            // argc, argv
            strcat( log, "        args = " );
            for ( i = 0; i < server->argc; ++ i ) {
                if ( 0 != i ) {
                    strcat( log, " " );
                }
                strcat( log, server->argv[ i ] );
            }
            strcat( log, "\n" );
            // worker_count
            sprintf( buf, "        worker_count = %d\n", server->worker_count );
            strcat( log, buf );
            // port
            sprintf( buf, "        ports, count = %d:\n", server->ports_count );
            strcat( log, buf );

            for ( i = 0; i < server->ports_count; ++ i ) {
                gr_port_item_t * item = & server->ports[ i ];

                // 不白看这儿，长经验呢！inet_ntoa在Windows下能转出0.0.0.0的地址，在linux下会core
                if ( 0 != item->addr4.sin_addr.s_addr ) {
                    //TODO: zouyueming 2013-10-21 21:40 we should avoid warning:
                    // demo_module/demo_module.c: In function ‘gr_init’:
                    // demo_module/demo_module.c:206: warning: assignment makes pointer from integer without a cast
                    addr = inet_ntoa( item->addr4.sin_addr );
                } else {
                    addr = "(empty)";
                }

                sprintf( buf, "            %s port = %d, addr_len = %d, addr = %s:%d, fd = %d\n",
                    item->is_tcp ? "TCP" : "UDP",
                    item->port,
                    (int)item->addr_len,
                    addr,
                    (int)ntohs(item->addr4.sin_port),
                    item->fd );
                strcat( log, buf );
            }
            printf( "%s", log );
            free( log );
        } else {
            printf( "搞什么啊？分配内存还能出错？！\n" );
        }
    } else if ( proc_type >= GR_PROCESS_THREAD_1 ) {
        printf( "[pid=%d] gr_init called, proc_type = %d(WORKER + %d)\n",
            getpid(), (int)proc_type, proc_type - GR_PROCESS_THREAD_1 );
    } else {
        assert( 0 );
    }
    fflush(stdout);

    // 如果返回0，则服务器会继续初始化，否则服务器会退出。

    return 0;
}

// 注意这个函数是可选的，你可以不实现它。
// 反初始化函数。

void gr_term(
    gr_process_type_t   proc_type,
    void *              must_be_zero
)
{
    // 和gr_init一样，proc_type参数告诉我们是哪个线程或者哪个进程在反初始化

    printf( "[pid=%d] gr_term called, proc_type = %d\n", getpid(), (int)proc_type );fflush(stdout);
}

// 注意这个函数是可选的，你可以不实现它。
// 一但accept了一个TCP连接，则该函数会被调用。

void gr_tcp_accept(
    int                 port,
    int                 sock,
    bool *              need_disconnect
)
{
    // 模块可以有选择的决定是否要关掉这个TCP连接，如果要关掉，就*need_disconnect=true即可。
    // 模块可以从port参数得到这个连接是从服务器的哪个监听端口连上来的

    printf( "%d port accepted socket %d\n", port, sock );
}

// 注意这个函数是可选的，你可以不实现它。
// 在关闭一个TCP连接之前，该函数会被调用。

void gr_tcp_close(
	int                 port,
    int                 sock
)
{
    // 模块可以从port参数得到这个要断掉的TCP连接是从服务器的哪个监听端口连上来的

    printf( "%d port will close socket %d\n", port, sock );
}

// 该函数用于检查data, len指定的数据包是否是个有效的数据包。这是个协议检查。本函数算行数。

void gr_check(
    void *              data,
    int                 len,
    gr_port_item_t *    port_info,
    int                 sock,
    gr_check_ctxt_t *   ctxt,
    bool *              is_error,
    bool *              is_full
)
{
    // 看着参数挺多的，但听我解释一下：
    // 1、data与len就是你还没决定的数据包。框架保证data不会为NULL，len一定>=0
    // 2、port_info用于取得服务这个数据包端口信息，它有如下几个字段：
    //    1) 服务于这个数据包的监听地址，为什么用这个而不是sockaddr_in? 支持IPV6啊老大。
    //       struct sockaddr *           addr;
    //       socklen_t                   addr_len;
    //
    //    2) 服务于这个数据包的监听端口的SOCKET描述符。注意TCP的监听端口和通信SOCKET不是一个。
    //       int                         fd;
    // 
    //    3) 服务于这个数据包的监听端口号
    //       uint16_t                    port;
    //
    //    4) 服务于这个数据包的监听端口是TCP还是UDP。
    //       看到这儿亮了吧？你只需要管协议，支持TCP和UDP的事儿交给服务器框架。
    //       bool                        is_tcp;
    // 3、sock就是当前连接的描述符。对于UDP，该描述符和port_info->fd是相同的。
    // 4、ctxt存储一些额外的上下文信息，其中包括了对HTTP数据包的解析中间结果，但那都不是
    //    服务模块编写者需要关心的。真正需要填写的是如下两个字段。
    //
    //     1) 数据包类型
    //        uint16_t                   cc_package_type;
    //
    //            cc_package_type可以填写的值如下列出：
    //            typedef enum
    //            {
    //                // HTTP 请求包
    //                GR_PACKAGE_HTTP_REQ         = 1,
    //                // HTTP 回复包
    //                GR_PACKAGE_HTTP_REPLY       = 2,
    //                // 用户自定义数据包
    //                GR_PACKAGE_PRIVATE          = 3,
    //                // 使用3bit存储该数值，所以最大可用的值为7，
    //                // 也就是说服务器端模块最多可以在一个模块中支持5种完全不同的自定义协议。
    //            } gr_package_type_t;
    //
    //            由于HTTP协议是服务器框架内置支持，所以用户模块永远都不必填写GR_PACKAGE_HTTP_REQ和
    //            GR_PACKAGE_HTTP_REPLY。只有可能填写的就是GR_PACKAGE_PRIVATE，表示它是一个用户数据包，
    //            如果在一个程序里支持多个协议，则还可以用GR_PACKAGE_PRIVATE + 1 到 GR_PACKAGE_PRIVATE + 4。
    //
    //     2) 数据包完整长度
    //        uint32_t                   cc_package_length;
    //            如果在解析数据包的过程中发现它是一个有效数据包，那就必须在这个字段里填写完整数据包的长度。
    //            因为TCP是流协议，这个方法帮助服务器框架切分数据包。UDP数据包也会通过该函数的调用来确定是否有效。
    // 5、is_error。如果在解析数据包过程中发现这是个无效包，则直接*is_err = true即可，否则不用动这个字段。
    // 6、is_full。如果is_error值为false，也就是说这是个有效包，本字段才会被服务器框架判断。如果发现数据区
    //             中至少包括了一包完整数据包，则*is_full = true。这时服务器框架才会切出一个包把它扔给工作进程(或线程)。

    // 为了演示包没收全的处理，我这里假设包每过两字节收一次。

    if ( len < 2 ) {
        // 初步判断为有效的用户私有包
        ctxt->cc_package_type = GR_PACKAGE_PRIVATE;
        // 没出错
        *is_error = false;
        // 但不是完整包，让服务器框架继续收
        *is_full = false;
        printf( "not full\n" );
        return;
    }

    // 有效的用户私有包
    ctxt->cc_package_type = GR_PACKAGE_PRIVATE;
    // 没出错
    *is_error = false;
    // 是完整包
    *is_full = true;
    // 必须填写完整包长度

    ctxt->cc_package_length = len;
    printf( "full\n" );
    // 看，实际上我们只用了data和len两个参数判断数据包，给了那么多参数，
    // 实际是为了方便模块编写者根据不同的场景、环境做不同的处理。

}

// 服务器框架会在工作进程(或线程)调用本函数处理数据包。本函数算行数。

void gr_proc(
    const void *        data,
    int                 len,
    gr_proc_ctxt_t *    ctxt,
    int *               processed_len
)
{
    // 我们这里会用到服务器内置对象 gr_i_server_t 的功能，如果常用，就应该将它保存起来

    gr_i_server_t * o = g_server->library->buildin;
    char *          rsp;

    // data, len 两个参数就是在 gr_check 函数中，自己切分好的完整包。
    // ctxt是处理数据包的上下文，它提供了更多相关数据：
    //      1、当前数据包是否TCP包
    //         uint16_t                 pc_is_tcp( bit field )
    //      2、在gr_check函数中确定的数据包类型,该机制用于支持多套协议
    //         uint16_t                 pc_package_type( bit field )
    //      4、使用的监听端口号
    //         int                      pc_port;
    //      5、处理SOCKET描述符
    //         int                      pc_fd;
    //      6、处理线程ID, 配置中有线程数量. [0, n)。服务器承诺，一个
    //         TCP连接的所有请求会分配到一个固定的工作线程上。
    //         相同客户端地址的UDP所有请求会分配到一个固定的工作线程上。
    //         int                      pc_thread_id;
    //      7、返回缓冲区
    //         char *                   pc_result_buf;
    //      8、返回缓冲区最大长度
    //         int                      pc_result_buf_max;
    //      9、返回缓冲区中的数据长度
    //         int *                    pc_result_buf_len;
    //     10、实际处理输入数据包的字节数。
    //         int *                    processed_len
    //                     < 0，数据包有错误，需要断连接.
    //                     = 0，数据包正确，但需要服务器端主动断连接。
    //                     > 0。数据包正确，返回已经处理的数据包长度.

    printf( "process %d byte user data\n", len );

    // 确认服务器框架填充的默认值
    assert( * processed_len == len );

    // 设置最大返回值字节数。该函数内部在必要时会分配内存，
    // 填充pc_result_buf和pc_result_buf_max字段，并将 * pc_result_buf_len 置为0
    rsp = (char *)o->set_max_response( o, ctxt, len );
    if ( NULL == rsp ) {
        // 服务器框架提供了个打日志功能
        o->log( o, __FILE__, __LINE__, __FUNCTION__, GR_LOG_ERROR,
            "set_max_response %d failed", len );
        * processed_len = -1;
        return;
    }

    // 拷贝数据
    memcpy( rsp, data, len );

    // 记录实际写入的数据字节数
    ctxt->pc_result_buf_len = len;

    // 至此我们完成了一个echo服务器，简单么？
}

// 累死我了，这套解释够详细吧？

// 以上就是一个完整的二进制数据包处理的服务器，
// 这套服务器支持TCP，UDP，甚至通过http夹带二进制文件的方式传递，
// 但这些对于服务器模块来说全透明，而又提供了让模块知晓的能力。

// 实现了上述功能的服务器，用了多少代码？你可以回头找一下“算行数”，看看涉及的代码有几行...

// 如果觉得还不够刺激，那我们再继续吧：

void gr_proc_http(
    gr_http_ctxt_t *    http
)
{
    // 是的，你没猜错，当服务器收到了一个HTTP数据包时，就会调用该函数。
    // 为了不被累死，我简单贴一下gr_http_ctxt_t 的成员变量，你可以根据
    // 变量名来猜出它的作用和使用方法：
    /*
    // uint16_t                 hc_is_tcp( bit field )
    // uint16_t                 hc_package_type( bit field )
    // int                      hc_port
    // int                      hc_fd
    // int                      hc_thread_id
    // char *                   hc_result_buf
    // int                      hc_result_buf_max
    // int *                    hc_result_buf_len
    // bool                     hc_is_error
    // bool                     hc_keep_alive
    //char                      request_type;
    //char *                    version;
    //char *                    directory;
    //char *                    object;
    //char *                    content_type;
    //char *                    user_agent;
    //gr_http_param_t *         params;
    //size_t                    params_count;
    //gr_http_param_t *         header;
    //size_t                    header_count;
    //gr_http_param_t *         form;
    //size_t                    form_count;
    //char *                    body;
    //size_t                    body_len;
    //int                       http_reply_code;

    typedef struct
    {
        // 参数名
        char *                      name;
        // 参数值
        char *                      value;
    } gr_http_param_t;
    */

    // 通过服务器框架提供的函数，你可以方便的生成一个HTTP返回包并写入http->hc_result_buf中
    // 但为了简单，这个过程暂时忽略了。
    const char * dir;
    int i;

    // 以下代码简单打印出调用方访问的请求内容。
    // 从URL,QueryString,Headers,Form, HttpBody都有。
    dir = http->hc_directory;
    if ( 0 == strcmp( "/", dir ) )
        dir = "";
    printf( "HTTP port %d,ThreadID=%d,SOCKET=%d,%s/%s",
            http->hc_port, http->hc_thread_id, http->hc_fd,
            dir, http->hc_object );
    for ( i = 0; i != http->hc_params_count; ++ i ) {
        if ( 0 == i ) {
            printf( "%s", "?" );
        } else {
            printf( "%s", "&" );
        }

        printf( "%s=%s", http->hc_params[ i ].name, http->hc_params[ i ].value );
    }
    printf( "\n" );
    // HTTP Headers
    for ( i = 0; i != http->hc_header_count; ++ i ) {
        printf( "%s : %s\n", http->hc_header[ i ].name, http->hc_header[ i ].value );
    }
    printf( "\n" );
    // 数据区
    printf( "%s\n", http->hc_body );
}

// 这回够刺激了吧？

// 请注意，这个服务器模块是跨平台的，它可以跟随服务器框架跨到它所支持的所有平台。

// 关于HTTP，我还没说完呢：
// 1、服务器框架支持同时在TCP、UDP里面处理HTTP协议
// 2、服务器框架支持一个TCP连接里使用不同的协议，比如第一个请求使用用户自定义协议，
//    第二个请求是个HTTP协议，第三个请求又是一个用户自定义协议...服务器框架会很Happy的支持。

// 说完了，喝口水去...

// 2013-09-28
