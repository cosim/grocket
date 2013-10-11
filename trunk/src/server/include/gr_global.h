/**
 * @file include/gr_global.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   ȫ��Ψһ����
 * Revision History ���¼���
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-05    Created.
 **/
#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_GLOBAL_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_GLOBAL_H_

#include "gr_log.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    gr_server_t         server_interface;

    // ��ʼ����־�ļ��𡣼���ȸ�ֵС����־�����
    gr_log_level_t      log_start_level;

    // �������ݰ������ֽ���
    int                 rsp_buf_align;

    // gr_logģ��
    void *              log;
    // gr_configģ��
    void *              config;
    // gr_serverģ��
    void *              server;
    // gr_moduleģ��
    void *              module;
    // gr_tcp_acceptģ��
    void *              tcp_accept;
    // gr_tcp_inģ��
    void *              tcp_in;
    // gr_udp_inģ��
    void *              udp_in;
    // gr_tcp_outģ��
    void *              tcp_out;
    // gr_udp_outģ��
    void *              udp_out;
    // gr_workerģ��
    void *              worker;
    // gr_backendģ��
    void *              backend;
    // gr_connģ��
    void *              conn;
    // gr_libraryģ��
    void *              library;

} gr_global_t;

// ֱ����externȫ�ֱ���,Ŀ���Ƿ�ֹͨ����������ȡ�ýṹ���µĺ������ÿ���
extern gr_global_t g_ghost_rocket_global;

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_GLOBAL_H_
