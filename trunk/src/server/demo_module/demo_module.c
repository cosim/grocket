/**
 * @file demo_module/demo_module.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/09/27
 * @version $Revision$ 
 * @brief   ���������demo��չģ��
 * Revision History ���¼���
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-09-27    Created.
 **/

///////////////////////////////////////////////////////////////////////
//
// grocket.hҪ�����ڰ�����֮ǰ��֤
// uint16_t, uint32_t, socklen_t, bool, size_t, sockaddr_in, sockaddr_in6�⼸
// ������������Ч���ұ�Ӧ��ֱ��#include "gr_stdinc.h"�㶨һ�У���Ϊ�˱�ʾgrocket.h��
// �����ࡱ�����ֹ�������ʵ�֡���������һ�Ρ����ۡ��Ĵ��룬������������Ļ�������Ȼ��
// �������ڡ�
//
// ���ۿ�ʼ
// 
#include <stdio.h>  // ������һ��ͷ�ļ������� size_t ���;Ϳ϶�����
#include <string.h> // for strcmp
#include <assert.h> // for assert

#if defined(WIN32) || defined(WIN64)

// ����ļ������sockaddr_in
    #include <WinSock2.h>
    // ����ļ������sockaddr_in6
    #include <WS2tcpip.h>
    #include <Windows.h>
    #pragma comment( lib, "ws2_32.lib" )

    // Windows��û��socklen_t��ͨ��������������д���sockaddr��ַ����ʱʹ�ã�int�Ǽ��ݶ��塣
    typedef int                 socklen_t;
    // ������ý�����
    typedef unsigned short      uint16_t;
    // ������ý�����
    typedef unsigned int        uint32_t;

#elif defined(__linux)

    // ��linux�£�sockaddr_in��sockaddr_in6��in.h��
    #include <netinet/in.h>

#elif defined( __APPLE__ )

    // �� Mac �£�uint16_t ʲô�Ķ���������ļ���
    #include <stdint.h>
    // �� Mac �£�malloc ������stdlib.h��
    #include <stdlib.h>
    // �� Mac �£�socklen_t ������netdb.h��
    #include <netdb.h>
#endif

#if ! defined(__cplusplus)
    // ��֧��C99��ǰ��C��������û��bool���͵ģ��ұ���Ҫ���������һ���ֽ�
    typedef unsigned char   bool;
    #define true    1
    #define false   0
#endif

#if defined(WIN32) || defined(WIN64)
// ���������demo��������ID����windows��û��getpid�������Լ��ṩһ��
static int getpid()
{
    return (int)GetProcessId( GetCurrentProcess() );
}
#endif

//
// ���۽���
//
///////////////////////////////////////////////////////////////////////

// д������ģ��Ψһ��Ҫ�������ļ�������Ҫ�����κο⡣������������
#include "grocket.h"

// ����������ṩ���û�ģ��Ľӿ�ָ�롣������������
gr_server_t *   g_server = NULL;

// ע����������ǿ�ѡ�ģ�����Բ�ʵ������
// ��ʼ��������
int gr_init(
    gr_process_type_t   proc_type,
    gr_server_t *       server
)
{
    // �����̳�ʼ�����ӽ��̳�ʼ���������̳߳�ʼ��ʱ������ñ�������
    // ����������أ�����proc_type����������ȡֵ�������¼��֣�
    // 1��GR_PROCESS_MASTER ���ʾ��������, �����������̳�ʼ����Դ, �ӽ���ʹ����Դ�ĳ���
    // 2��GR_PROCESS_CHILD �����Ĺ�������
    // 3��GR_PROCESS_THREAD_1 ��ʾ���������еĵ�һ�������̳߳�ʼ��
    // 4��GR_PROCESS_THREAD_1 + n ��ʾ���������еĵ� n �������̳߳�ʼ��

    // �汾�ż�飬��ܵ���ͼ��ݰ汾�ű���С�ڻ����ģ�鿪����ʹ�õĿ�����ܵİ汾�š����ж���������
    // �����飬ʹ�ÿ���ֱ����������������ܵĶ������ļ�����ģ��ļ����Լ��ֻ��Ҫ��ģ���Ƿ��ܱ��ɹ�װ�ء�
    if ( server->low_version > GR_SERVER_VERSION ) {
        // �����ܽӿڼ����Լ��ʧ�ܡ�ֱ�ӳ�ʼ��ʧ�ܣ�ʡ��������ʱcore����׷��ɱ�
        printf( "version compatible check failed, I'm %d, Framework %d\n",
            GR_SERVER_VERSION, server->low_version );
        return -1;
    }

    // ǰ��˵����д������ģ��Ψһ��Ҫ�������ļ�������Ҫ�����κο⡱���Ƿ�������ܵ���������ʵ��
    // ��������ǣ��ڷ�������ܽ��������gr_init������ͨ��gr_server_t *�ӿ��Ժ���ָ���
    // ��ʽ��¶���û�ģ�飬�����û�ģ�鵱ȻҪ�����ָ�뱣��������������������
    g_server = server;

    {
        int n;

        // ��仰���ӷ�����ȡ��һ��������Ľӿ�ָ�룬����������ID�� CLASS_SERVER
        gr_i_server_t * o =
            (gr_i_server_t *)g_server->library->classes[ CLASS_SERVER ]->singleton;
        assert( o );

        // ��������config_get_int�ӷ����������ļ���ȡ[server]�ε�tcp.accept.concurrent��ֵ
        n = o->config_get_int( o, "server", "tcp.accept.concurrent", 0 );
        printf( "I guess, [server]tcp.accept.concurrent = %d, is it right? haha!!!!\n", n );

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // ��������׷�������ܵ�һ����ɫ�����ṩ�๦�ܿ�����ã�����Ҫ���������κο⡣
        // ��֧����ʵ���Լ��Ĺ��ܺ����⣬�Զ�̬�����ʽ���ڷ���������й�������ʹ�ã�
        // ��Ȼʹ������Ȼ����Ҫ�����κο⣡��
        // ������ʵ���޸ġ����¡������º�����������Ҫ��ĳ����������ӡ�
    }

    // �����ܺ��棬���ԣ�
    // 1��ͨ��server->argc��server_argvȡ�������в���
    // 2��ͨ��server->worker_countȡ��worker����
    // 3��ͨ��server->ports_countȡ�ü����˿�����
    // 4��ͨ��server->portsȡ��ÿһ���˿ڵ�����
    // ���´�����ʾ��
    if ( GR_PROCESS_MASTER == proc_type || GR_PROCESS_CHILD == proc_type ) {
        int i;
        char buf[ 1024 ];
        char * log = (char *)malloc( 1024 * 1024 );
        if ( NULL != log ) {

            char pt[ 32 ] = "";

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
                sprintf( buf, "            %s port = %d, addr_len = %d, addr = %s:%d, fd = %d\n",
                    item->is_tcp ? "TCP" : "UDP",
                    item->port,
                    (int)item->addr_len,
                    // ���׿�������������أ�inet_ntoa��Windows����ת��0.0.0.0�ĵ�ַ����linux�»�core
                    ( 0 != item->addr4.sin_addr.s_addr ) ? inet_ntoa(item->addr4.sin_addr) : "(empty)",
                    (int)ntohs(item->addr4.sin_port),
                    item->fd );
                strcat( log, buf );
            }
            printf( "%s", log );
            free( log );
        } else {
            printf( "��ʲô���������ڴ滹�ܳ�����\n" );
        }
    } else if ( proc_type >= GR_PROCESS_THREAD_1 ) {
        printf( "[pid=%d] gr_init called, proc_type = %d(WORKER + %d)\n",
            getpid(), (int)proc_type, proc_type - GR_PROCESS_THREAD_1 );
    } else {
        assert( 0 );
    }
    fflush(stdout);

    // �������0����������������ʼ����������������˳���
    return 0;
}

// ע����������ǿ�ѡ�ģ�����Բ�ʵ������
// ����ʼ��������
void gr_term(
    gr_process_type_t   proc_type,
    void *              must_be_zero
)
{
    // ��gr_initһ����proc_type���������������ĸ��̻߳����ĸ������ڷ���ʼ��
    printf( "[pid=%d] gr_term called, proc_type = %d\n", getpid(), (int)proc_type );fflush(stdout);
}

// ע����������ǿ�ѡ�ģ�����Բ�ʵ������
// һ��accept��һ��TCP���ӣ���ú����ᱻ���á�
void gr_tcp_accept(
    int                 port,
    int                 sock,
    int *               need_disconnect
)
{
    // ģ�������ѡ��ľ����Ƿ�Ҫ�ص����TCP���ӣ����Ҫ�ص�����*need_disconnect=1���ɡ�
    // ģ����Դ�port�����õ���������Ǵӷ��������ĸ������˿���������
    printf( "%d port accepted socket %d\n", port, sock );
}

// ע����������ǿ�ѡ�ģ�����Բ�ʵ������
// �ڹر�һ��TCP����֮ǰ���ú����ᱻ���á�
void gr_tcp_close(
	int                 port,
    int                 sock
)
{
    // ģ����Դ�port�����õ����Ҫ�ϵ���TCP�����Ǵӷ��������ĸ������˿���������
    printf( "%d port will close socket %d\n", port, sock );
}

// �ú������ڼ��data, lenָ�������ݰ��Ƿ��Ǹ���Ч�����ݰ������Ǹ�Э���顣��������������
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
    // ���Ų���ͦ��ģ������ҽ���һ�£�
    // 1��data��len�����㻹û���������ݰ�����ܱ�֤data����ΪNULL��lenһ��>=0
    // 2��port_info����ȡ�÷���������ݰ��˿���Ϣ���������¼����ֶΣ�
    //    1) ������������ݰ��ļ�����ַ��Ϊʲô�����������sockaddr_in? ֧��IPV6���ϴ�
    //       struct sockaddr *           addr;
    //       socklen_t                   addr_len;
    //
    //    2) ������������ݰ��ļ����˿ڵ�SOCKET��������ע��TCP�ļ����˿ں�ͨ��SOCKET����һ����
    //       int                         fd;
    // 
    //    3) ������������ݰ��ļ����˿ں�
    //       uint16_t                    port;
    //
    //    4) ������������ݰ��ļ����˿���TCP����UDP��
    //       ����������˰ɣ���ֻ��Ҫ��Э�飬֧��TCP��UDP���¶�������������ܡ�
    //       bool                        is_tcp;
    // 3��sock���ǵ�ǰ���ӵ�������������UDP������������port_info->fd����ͬ�ġ�
    // 4��ctxt�洢һЩ�������������Ϣ�����а����˶�HTTP���ݰ��Ľ����м��������Ƕ�����
    //    ����ģ���д����Ҫ���ĵġ�������Ҫ��д�������������ֶΡ�
    //
    //     1) ���ݰ�����
    //        uint16_t                   cc_package_type;
    //
    //            cc_package_type������д��ֵ�����г���
    //            typedef enum
    //            {
    //                // HTTP �����
    //                GR_PACKAGE_HTTP_REQ         = 1,
    //                // HTTP �ظ���
    //                GR_PACKAGE_HTTP_REPLY       = 2,
    //                // �û��Զ������ݰ�
    //                GR_PACKAGE_PRIVATE          = 3,
    //                // ʹ��3bit�洢����ֵ�����������õ�ֵΪ7��
    //                // Ҳ����˵��������ģ����������һ��ģ����֧��5����ȫ��ͬ���Զ���Э�顣
    //            } gr_package_type_t;
    //
    //            ����HTTPЭ���Ƿ������������֧�֣������û�ģ����Զ��������дGR_PACKAGE_HTTP_REQ��
    //            GR_PACKAGE_HTTP_REPLY��ֻ�п�����д�ľ���GR_PACKAGE_PRIVATE����ʾ����һ���û����ݰ���
    //            �����һ��������֧�ֶ��Э�飬�򻹿�����GR_PACKAGE_PRIVATE + 1 �� GR_PACKAGE_PRIVATE + 4��
    //
    //     2) ���ݰ���������
    //        uint32_t                   cc_package_length;
    //            ����ڽ������ݰ��Ĺ����з�������һ����Ч���ݰ����Ǿͱ���������ֶ�����д�������ݰ��ĳ��ȡ�
    //            ��ΪTCP����Э�飬���������������������з����ݰ���UDP���ݰ�Ҳ��ͨ���ú����ĵ�����ȷ���Ƿ���Ч��
    // 5��is_error������ڽ������ݰ������з������Ǹ���Ч������ֱ��*is_err = true���ɣ������ö�����ֶΡ�
    // 6��is_full�����is_errorֵΪfalse��Ҳ����˵���Ǹ���Ч�������ֶβŻᱻ����������жϡ��������������
    //             �����ٰ�����һ���������ݰ�����*is_full = true����ʱ��������ܲŻ��г�һ���������Ӹ���������(���߳�)��

    // Ϊ����ʾ��û��ȫ�Ĵ�������������ÿ�����ֽ���һ�Ρ�
    if ( len < 2 ) {
        // �����ж�Ϊ��Ч���û�˽�а�
        ctxt->cc_package_type = GR_PACKAGE_PRIVATE;
        // û����
        *is_error = false;
        // ���������������÷�������ܼ�����
        *is_full = false;
        printf( "not full\n" );
        return;
    }

    // ��Ч���û�˽�а�
    ctxt->cc_package_type = GR_PACKAGE_PRIVATE;
    // û����
    *is_error = false;
    // ��������
    *is_full = true;
    // ������д����������
    ctxt->cc_package_length = len;
    printf( "full\n" );
    // ����ʵ��������ֻ����data��len���������ж����ݰ���������ô�������
    // ʵ����Ϊ�˷���ģ���д�߸��ݲ�ͬ�ĳ�������������ͬ�Ĵ���
}

// ��������ܻ��ڹ�������(���߳�)���ñ������������ݰ�����������������
void gr_proc(
    const void *        data,
    int                 len,
    gr_proc_ctxt_t *    ctxt,
    int *               processed_len
)
{
    // ����������õ����������ö��� gr_i_server_t �Ĺ��ܣ�������ã���Ӧ�ý�����������
    gr_i_server_t * o = (gr_i_server_t *)g_server->library->classes[ CLASS_SERVER ]->singleton;
    char *          rsp;

    // data, len �������������� gr_check �����У��Լ��зֺõ���������
    // ctxt�Ǵ������ݰ��������ģ����ṩ�˸���������ݣ�
    //      1����ǰ���ݰ��Ƿ�TCP��
    //         uint16_t                 pc_is_tcp( bit field )
    //      2����gr_check������ȷ�������ݰ�����,�û�������֧�ֶ���Э��
    //         uint16_t                 pc_package_type( bit field )
    //      4��ʹ�õļ����˿ں�
    //         int                      pc_port;
    //      5������SOCKET������
    //         int                      pc_fd;
    //      6�������߳�ID, ���������߳�����. [0, n)����������ŵ��һ��
    //         TCP���ӵ������������䵽һ���̶��Ĺ����߳��ϡ�
    //         ��ͬ�ͻ��˵�ַ��UDP�����������䵽һ���̶��Ĺ����߳��ϡ�
    //         int                      pc_thread_id;
    //      7�����ػ�����
    //         char *                   pc_result_buf;
    //      8�����ػ�������󳤶�
    //         int                      pc_result_buf_max;
    //      9�����ػ������е����ݳ���
    //         int *                    pc_result_buf_len;
    //     10��ʵ�ʴ����������ݰ����ֽ�����
    //         int *                    processed_len
    //                     < 0�����ݰ��д�����Ҫ������.
    //                     = 0�����ݰ���ȷ������Ҫ�����������������ӡ�
    //                     > 0�����ݰ���ȷ�������Ѿ���������ݰ�����.

    printf( "process %d byte user data\n", len );

    // ȷ�Ϸ������������Ĭ��ֵ
    assert( * processed_len == len );

    // ������󷵻�ֵ�ֽ������ú����ڲ��ڱ�Ҫʱ������ڴ棬
    // ���pc_result_buf��pc_result_buf_max�ֶΣ����� * pc_result_buf_len ��Ϊ0
    rsp = (char *)o->set_max_response( o, ctxt, len );
    if ( NULL == rsp ) {
        // ����������ṩ�˸�����־����
        o->log( o, __FILE__, __LINE__, __FUNCTION__, GR_LOG_ERROR,
            "set_max_response %d failed", len );
        * processed_len = -1;
        return;
    }

    // ��������
    memcpy( rsp, data, len );

    // ��¼ʵ��д��������ֽ���
    ctxt->pc_result_buf_len = len;

    // �������������һ��echo����������ô��
}

// �������ˣ����׽��͹���ϸ�ɣ�

// ���Ͼ���һ�������Ķ��������ݰ�����ķ�������
// ���׷�����֧��TCP��UDP������ͨ��http�д��������ļ��ķ�ʽ���ݣ�
// ����Щ���ڷ�����ģ����˵ȫ͸���������ṩ����ģ��֪����������

// ʵ�����������ܵķ����������˶��ٴ��룿����Ի�ͷ��һ�¡����������������漰�Ĵ����м���...

// ������û������̼����������ټ����ɣ�

void gr_proc_http(
    gr_http_ctxt_t *    http
)
{
    // �ǵģ���û�´����������յ���һ��HTTP���ݰ�ʱ���ͻ���øú�����
    // Ϊ�˲����������Ҽ���һ��gr_http_ctxt_t �ĳ�Ա����������Ը���
    // ���������³��������ú�ʹ�÷�����
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
        // ������
        char *                      name;
        // ����ֵ
        char *                      value;
    } gr_http_param_t;
    */

    // ͨ������������ṩ�ĺ���������Է��������һ��HTTP���ذ���д��http->hc_result_buf��
    // ��Ϊ�˼򵥣����������ʱ�����ˡ�
    const char * dir;
    int i;

    // ���´���򵥴�ӡ�����÷����ʵ��������ݡ�
    // ��URL,QueryString,Headers,Form, HttpBody���С�
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
    // ������
    printf( "%s\n", http->hc_body );
}

// ��ع��̼��˰ɣ�

// ��ע�⣬���������ģ���ǿ�ƽ̨�ģ������Ը����������ܿ絽����֧�ֵ�����ƽ̨��

// ����HTTP���һ�û˵���أ�
// 1�����������֧��ͬʱ��TCP��UDP���洦��HTTPЭ��
// 2�����������֧��һ��TCP������ʹ�ò�ͬ��Э�飬�����һ������ʹ���û��Զ���Э�飬
//    �ڶ��������Ǹ�HTTPЭ�飬��������������һ���û��Զ���Э��...��������ܻ��Happy��֧�֡�

// ˵���ˣ��ȿ�ˮȥ...

// 2013-09-28
