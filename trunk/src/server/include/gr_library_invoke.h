/**
 * @file include/gr_library_invoke.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/08
 * @version $Revision$ 
 * @brief   
 * Revision History 大事件记
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-08    Created.
 **/
#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_LIBRARY_INVOKE_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_LIBRARY_INVOKE_H_

#include "gr_stdinc.h"

#ifdef __cplusplus
extern "C" {
#endif

// 示例代码见gr_invoke_test

int gr_invoke_test();

typedef enum
{
    GR_CDECL    = 0,

    GR_STDCALL  = 1

} gr_call_type_t;

bool gr_invoke_rb(
    void *          func_addr,
    gr_call_type_t  ct,
    void *          stack,
    size_t          stack_bytes
);

int gr_invoke_r32(
    void *          func_addr,
    gr_call_type_t  ct,
    void *          stack,
    size_t          stack_bytes
);

long long gr_invoke_r64(
    void *          func_addr,
    gr_call_type_t  ct,
    void *          stack,
    size_t          stack_bytes
);

void * gr_invoke_rp(
    void *          func_addr,
    gr_call_type_t  ct,
    void *          stack,
    size_t          stack_bytes
);

void gr_invoke_void(
    void *          func_addr,
    gr_call_type_t  ct,
    void *          stack,
    size_t          stack_bytes
);

// fmt说明：
// p, s    : pointer
// f       : double
// i, d, 4 : 4 byte integer
// l       : long
// L, 8    : 8 byte integer

bool gr_call_rb(
    void *          func_addr,
    gr_call_type_t  ct,
    const char *    fmt,
    ...
);

int gr_call_r32(
    void *          func_addr,
    gr_call_type_t  ct,
    const char *    fmt,
    ...
);

long long gr_call_r64(
    void *          func_addr,
    gr_call_type_t  ct,
    const char *    fmt,
    ...
);

void * gr_call_rp(
    void *          func_addr,
    gr_call_type_t  ct,
    const char *    fmt,
    ...
);

void gr_call_void(
    void *          func_addr,
    gr_call_type_t  ct,
    const char *    fmt,
    ...
);

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_LIBRARY_INVOKE_H_
