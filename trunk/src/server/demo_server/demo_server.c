/**
 * @file demo_server/demo_server.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/09/29
 * @version $Revision$ 
 * @brief   ���������demo����������
 * Revision History ���¼���
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-09-29    Created.
 **/

// �ǳ���ϸ�ķ�������ܽӿ�ʹ�õ�ϸ����� demo_module.c
// ������ʾ�����дһ�����������̡�

// Ҫ�Լ��ṩһ����������ִ�г������ֳ���������һЩ�Ƚϸ��ӵ�Ӧ�ó���������Ǩ�Ƶȡ�
// ʹ���߿��Գ�ʼ�����Լ����¶�֮���ٳ�ʼ�����������������������Ķ�����һ����������
// �̻򵥶��Ľ�����ȥ��������ֻ����ʾһ����δ�ɸ�Ŀ�ꡣ
// ��α���������� libgrocket.h ͷ�ļ���
#include "libgrocket.h"

// ��α���Ҫ���Ӿ�̬����
// Windows�µľ�̬������
#if defined(WIN32)

    #if defined(_DEBUG)
        #pragma comment( lib, "../bin/Win32/Debug/libgrocket.lib" )
    #else
        #pragma comment( lib, "../bin/Win32/Release/libgrocket.lib" )
    #endif
#elif defined(WIN64)
    #if defined(_DEBUG)
        #pragma comment( lib, "../bin/x64/Debug/libgrocket.lib" )
    #else
        #pragma comment( lib, "../bin/x64/Release/libgrocket.lib" )
    #endif
#endif

// ע�⣺������ֱ��ʹ����demo_module.c Դ�ļ������Ը�������д��չ�ķ�������ԭ�ⲻ����,
// Ψһ���������չ�Ĵ��������ڿ�ִ���ļ��������װ�ض�̬���ˡ�
// ��ֻ��Ҫ�����ｫ demo_module.c �ļ��еĺ�������һ�¡�

extern int gr_init(
    gr_process_type_t   proc_type,
    gr_server_t *       server
);

extern void gr_term(
    gr_process_type_t   proc_type,
    void *              must_be_zero
);

extern void gr_tcp_accept(
    int                 port,
    int                 sock,
    int *               need_disconnect
);

extern void gr_tcp_close(
	int                 port,
    int                 sock
);

extern void gr_check(
    void *              data,
    int                 len,
    gr_port_item_t *    port_info,
    int                 sock,
    gr_check_ctxt_t *   ctxt,
    bool *              is_error,
    bool *              is_full
);

extern void gr_proc(
    const void *        data,
    int                 len,
    gr_proc_ctxt_t *    ctxt,
    int *               processed_len
);

extern void gr_proc_http(
    gr_http_ctxt_t *    http
);

// ������д main ����
int main( int argc, char ** argv )
{
    // ������Ѿ���� 89% �ˣ�������һ�����⣺�����������Ҫ������grocket.iniҲ��Ҫ
    // ��������ִ�г���Ŀ¼�£�����ô��
    // �����˵���У���˵����������������ļ��ж�ȡ���ã���������Ҫһ��û�������ļ��Ŀ�ִ�г���
    // �Ǻã������ֹ���һ�����ã�
    char config[ 1024 ] = "";
    int config_len;
    strcpy( config,
        "[server]\n"
        "daemon                      = false\n"
        "manual_tcp                  = false\n"
        "network_in_concurrent       = 10000\n"
        "tcp_accept_send_buf         = 8388608\n"
        "tcp_accept_recv_buf         = 8388608\n"
        "udp_send_buf                = 8388608\n"
        "udp_recv_buf                = 8388608\n"
        "listen_backlog              = 511\n"
        "\n"
        "network_in_thread_count     = 1\n"
        "network_out_thread_count    = 1\n"
        "worker_thread_count         = 10\n"
        "backend_thread_count        = 1\n"
        "\n"
        "module                      = ./demo_module\n"
        "\n"
        "[listen]\n"
        "0 = tcp://0.0.0.0:65535\n"
        //"1 = udp://0.0.0.0:65535\n"
    );
    config_len = strlen( config );

    // ������Ѿ���� 99% �ˣ��������ʣ��� 1%

    return gr_main(
        // ��ִ���ļ�����
        argc, argv,
        // �����ļ�
        config, config_len,
        // ������ģ�鴦����
        gr_init, gr_term, gr_tcp_accept, gr_tcp_close, gr_check, gr_proc, gr_proc_http
    );
}

// д���ˣ�
// д���ˡ�
// д���ˣ�!
// д���ˡ�
