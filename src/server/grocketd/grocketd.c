/**
 * @file grocketd/grocketd.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/09/24
 * @version $Revision$ 
 * @brief 服务框架自己提供的可执行主程序。
 *
 * Revision History 大事件记
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-09-24    Created.
 **/

#include "gr_stdinc.h"
#include "grocket.h"
#include "libgrocket.h"

// Windows下的静态库依赖
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
    // 默认服务器框架自己提供可执行程序，
    // 所有的配置项和回调函数都通过配置文件提供，
    // 所以gr_main函数只用了前两个参数。
    // 如果gsocket使用者想自己提供可执行程序，同时不需要配置文件，则后面的参数才需要自己指定。
    // 使用举例见 demo_server.c
    return gr_main(
        argc, argv,
        NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL
    );
}
