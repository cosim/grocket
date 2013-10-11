/**
 * @file grocketd/grocketd.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/09/24
 * @version $Revision$ 
 * @brief �������Լ��ṩ�Ŀ�ִ��������
 *
 * Revision History ���¼���
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-09-24    Created.
 **/

#include "gr_stdinc.h"
#include "grocket.h"
#include "libgrocket.h"

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

int main( int argc, char ** argv )
{
    // Ĭ�Ϸ���������Լ��ṩ��ִ�г���
    // ���е�������ͻص�������ͨ�������ļ��ṩ��
    // ����gr_main����ֻ����ǰ����������
    // ���gsocketʹ�������Լ��ṩ��ִ�г���ͬʱ����Ҫ�����ļ��������Ĳ�������Ҫ�Լ�ָ����
    // ʹ�þ����� demo_server.c
    return gr_main(
        argc, argv,
        NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL
    );
}
