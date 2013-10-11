/**
 * @file include/gr_tools.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/03
 * @version $Revision$ 
 * @brief   ͨ�ù��ߺ���
 * Revision History ���¼���
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-03    Created.
 **/
#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_TOOLS_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_TOOLS_H_

#include "gr_stdinc.h"

#ifdef __cplusplus
extern "C" {
#endif

int
get_errno();

/**
* @brief ȡ�ÿ�ִ���ļ�·��
* @param [out] char * path: ·��������
* @param [in] size_t path_len: ·���������ֽ��������ַ���������
* @return ����ʵ�ʵ�·���ֽ����������ַ���������
*/
size_t
get_exe_path(
    char * path,
    size_t  path_len
);

/**
 * @brief ���ַ������ߵĿո�ȥ���������µ���ʼλ�ã����µĳ���
 *   @param[in,modify] char * s: �����ַ����������ڲ���������������޸�
 *   @param[in,out] size_t * len: �����ַ������ȣ������������������ȥ���ո����ַ������ȣ������ֵΪ-1����������strlen
 * @return char *: ����ȥ���ո����ַ�����ʼλ�á����ָ��>= s����
 * @warning: ���������޸��ַ�������
 */
char *
str_trim(
    char * s,
    size_t * len
);

/**
 * @brief transform / or \ separate path to current OS separate path
 * @param[in, out] car * path: path
 * @code
       char path[ 256 ] = "./log/today.log";
       path_to_os( path );
 * @endcode
 */
void
path_to_os(
    char * path
);

void
sleep_ms(
    uint32_t ms
);

bool
is_exists(
    const char * path
);

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_TOOLS_H_
