/**
 * @file libgrocket/gr_library_invoke.c
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
#include "gr_library_invoke.h"
#include "gr_log.h"
#include "gr_global.h"
#include "gr_tools.h"
#include "gr_mem.h"
#include <stdarg.h>

int gr_invoke_test()
{
    // 测试
    {
        char    buf[ 32 ] = "+=";
        char *  stack[ 2 ];
        char *  p;

        stack[ 0 ] = buf;
        stack[ 1 ] = (char*)"xixihaha";

        p = (char*)gr_invoke_rp( (void*)strcat, GR_CDECL, stack, sizeof( stack ) );

        if ( 0 != strcmp( buf, "+=xixihaha" ) ) {
            return -1;
        }
        if ( p != buf ) {
            return -2;
        }
    }
    {
        char buf[ 32 ] = "+=";
        char* p = (char*)gr_call_rp( (void*)strcat, GR_CDECL, "pp", buf, "xixihaha" );

        if ( 0 != strcmp( buf, "+=xixihaha" ) ) {
            return -3;
        }
        if ( p != buf ) {
            return -4;
        }
    }

#if defined( WIN32 ) || defined( WIN64 )
    {
        BOOL b = FALSE;

        //int n = gr_call_r32( MessageBoxA, GR_STDCALL, "pppi", NULL, "lpText", "lpCaption", MB_OK );
        //printf( "%d\n", n );

        // MessageBeep
        b = gr_call_r32( MessageBeep, GR_STDCALL, "i", MB_OK );
        if ( ! b ) {
            return -5;
        }
    }
#endif

    return 0;
}

#pragma pack( push, 1 )

struct stack_4
{
    char        data[ 4 ];
};
struct stack_8
{
    char        data[ 8 ];
};
struct stack_12
{
    char        data[ 12 ];
};
struct stack_16
{
    char        data[ 16 ];
};
struct stack_20
{
    char        data[ 20 ];
};
struct stack_24
{
    char        data[ 24 ];
};
struct stack_28
{
    char        data[ 28 ];
};
struct stack_32
{
    char        data[ 32 ];
};
struct stack_36
{
    char        data[ 36 ];
};
struct stack_40
{
    char        data[ 40 ];
};
struct stack_44
{
    char        data[ 44 ];
};
struct stack_48
{
    char        data[ 48 ];
};
struct stack_52
{
    char        data[ 52 ];
};
struct stack_56
{
    char        data[ 56 ];
};
struct stack_60
{
    char        data[ 60 ];
};
struct stack_64
{
    char        data[ 64 ];
};

typedef union
{
    struct stack_4      s4;
    struct stack_8      s8;
    struct stack_12     s12;
    struct stack_16     s16;
    struct stack_20     s20;
    struct stack_24     s24;
    struct stack_28     s28;
    struct stack_32     s32;
    struct stack_36     s36;
    struct stack_40     s40;
    struct stack_44     s44;
    struct stack_48     s48;
    struct stack_52     s52;
    struct stack_56     s56;
    struct stack_60     s60;
    struct stack_64     s64;
}  stack;

#pragma pack( pop )

bool gr_invoke_rb(
    void *          addr,
    gr_call_type_t  ct,
    void *          stack,
    size_t          stack_bytes
)
{
    typedef bool ( __cdecl * cdecl_4 )( struct stack_4 );
    typedef bool ( __cdecl * cdecl_8 )( struct stack_8 );
    typedef bool ( __cdecl * cdecl_12 )( struct stack_12 );
    typedef bool ( __cdecl * cdecl_16 )( struct stack_16 );
    typedef bool ( __cdecl * cdecl_20 )( struct stack_20 );
    typedef bool ( __cdecl * cdecl_24 )( struct stack_24 );
    typedef bool ( __cdecl * cdecl_28 )( struct stack_28 );
    typedef bool ( __cdecl * cdecl_32 )( struct stack_32 );
    typedef bool ( __cdecl * cdecl_36 )( struct stack_36 );
    typedef bool ( __cdecl * cdecl_40 )( struct stack_40 );
    typedef bool ( __cdecl * cdecl_44 )( struct stack_44 );
    typedef bool ( __cdecl * cdecl_48 )( struct stack_48 );
    typedef bool ( __cdecl * cdecl_52 )( struct stack_52 );
    typedef bool ( __cdecl * cdecl_56 )( struct stack_56 );
    typedef bool ( __cdecl * cdecl_60 )( struct stack_60 );
    typedef bool ( __cdecl * cdecl_64 )( struct stack_64 );

    typedef bool ( __stdcall * stdcall_4 )( struct stack_4 );
    typedef bool ( __stdcall * stdcall_8 )( struct stack_8 );
    typedef bool ( __stdcall * stdcall_12 )( struct stack_12 );
    typedef bool ( __stdcall * stdcall_16 )( struct stack_16 );
    typedef bool ( __stdcall * stdcall_20 )( struct stack_20 );
    typedef bool ( __stdcall * stdcall_24 )( struct stack_24 );
    typedef bool ( __stdcall * stdcall_28 )( struct stack_28 );
    typedef bool ( __stdcall * stdcall_32 )( struct stack_32 );
    typedef bool ( __stdcall * stdcall_36 )( struct stack_36 );
    typedef bool ( __stdcall * stdcall_40 )( struct stack_40 );
    typedef bool ( __stdcall * stdcall_44 )( struct stack_44 );
    typedef bool ( __stdcall * stdcall_48 )( struct stack_48 );
    typedef bool ( __stdcall * stdcall_52 )( struct stack_52 );
    typedef bool ( __stdcall * stdcall_56 )( struct stack_56 );
    typedef bool ( __stdcall * stdcall_60 )( struct stack_60 );
    typedef bool ( __stdcall * stdcall_64 )( struct stack_64 );

    switch( ALIGN_UP( stack_bytes, sizeof( long ) ) )
    {
    case 4:
        if ( GR_CDECL == ct )
            return ((cdecl_4)addr)( * (struct stack_4 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_4)addr)( * (struct stack_4 *)stack );
    case 8:
        if ( GR_CDECL == ct )
            return ((cdecl_8)addr)( * (struct stack_8 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_8)addr)( * (struct stack_8 *)stack );
    case 12:
        if ( GR_CDECL == ct )
            return ((cdecl_12)addr)( * (struct stack_12 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_12)addr)( * (struct stack_12 *)stack );
    case 16:
        if ( GR_CDECL == ct )
            return ((cdecl_16)addr)( * (struct stack_16 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_16)addr)( * (struct stack_16 *)stack );
    case 20:
        if ( GR_CDECL == ct )
            return ((cdecl_20)addr)( * (struct stack_20 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_20)addr)( * (struct stack_20 *)stack );
    case 24:
        if ( GR_CDECL == ct )
            return ((cdecl_24)addr)( * (struct stack_24 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_24)addr)( * (struct stack_24 *)stack );
    case 32:
        if ( GR_CDECL == ct )
            return ((cdecl_32)addr)( * (struct stack_32 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_32)addr)( * (struct stack_32 *)stack );
    case 36:
        if ( GR_CDECL == ct )
            return ((cdecl_36)addr)( * (struct stack_36 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_36)addr)( * (struct stack_36 *)stack );
    case 40:
        if ( GR_CDECL == ct )
            return ((cdecl_40)addr)( * (struct stack_40 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_40)addr)( * (struct stack_40 *)stack );
    case 44:
        if ( GR_CDECL == ct )
            return ((cdecl_44)addr)( * (struct stack_44 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_44)addr)( * (struct stack_44 *)stack );
    case 48:
        if ( GR_CDECL == ct )
            return ((cdecl_48)addr)( * (struct stack_48 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_48)addr)( * (struct stack_48 *)stack );
    case 52:
        if ( GR_CDECL == ct )
            return ((cdecl_52)addr)( * (struct stack_52 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_52)addr)( * (struct stack_52 *)stack );
    case 56:
        if ( GR_CDECL == ct )
            return ((cdecl_56)addr)( * (struct stack_56 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_56)addr)( * (struct stack_56 *)stack );
    case 60:
        if ( GR_CDECL == ct )
            return ((cdecl_60)addr)( * (struct stack_60 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_60)addr)( * (struct stack_60 *)stack );
    case 64:
        if ( GR_CDECL == ct )
            return ((cdecl_64)addr)( * (struct stack_64 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_64)addr)( * (struct stack_64 *)stack );
    default:
        break;
    }

    return false;
}

void* gr_invoke_rp(
    void *          addr,
    gr_call_type_t  ct,
    void *          stack,
    size_t          stack_bytes
)
{
    typedef void * ( __cdecl * cdecl_4 )( struct stack_4 );
    typedef void * ( __cdecl * cdecl_8 )( struct stack_8 );
    typedef void * ( __cdecl * cdecl_12 )( struct stack_12 );
    typedef void * ( __cdecl * cdecl_16 )( struct stack_16 );
    typedef void * ( __cdecl * cdecl_20 )( struct stack_20 );
    typedef void * ( __cdecl * cdecl_24 )( struct stack_24 );
    typedef void * ( __cdecl * cdecl_28 )( struct stack_28 );
    typedef void * ( __cdecl * cdecl_32 )( struct stack_32 );
    typedef void * ( __cdecl * cdecl_36 )( struct stack_36 );
    typedef void * ( __cdecl * cdecl_40 )( struct stack_40 );
    typedef void * ( __cdecl * cdecl_44 )( struct stack_44 );
    typedef void * ( __cdecl * cdecl_48 )( struct stack_48 );
    typedef void * ( __cdecl * cdecl_52 )( struct stack_52 );
    typedef void * ( __cdecl * cdecl_56 )( struct stack_56 );
    typedef void * ( __cdecl * cdecl_60 )( struct stack_60 );
    typedef void * ( __cdecl * cdecl_64 )( struct stack_64 );

    typedef void * ( __stdcall * stdcall_4 )( struct stack_4 );
    typedef void * ( __stdcall * stdcall_8 )( struct stack_8 );
    typedef void * ( __stdcall * stdcall_12 )( struct stack_12 );
    typedef void * ( __stdcall * stdcall_16 )( struct stack_16 );
    typedef void * ( __stdcall * stdcall_20 )( struct stack_20 );
    typedef void * ( __stdcall * stdcall_24 )( struct stack_24 );
    typedef void * ( __stdcall * stdcall_28 )( struct stack_28 );
    typedef void * ( __stdcall * stdcall_32 )( struct stack_32 );
    typedef void * ( __stdcall * stdcall_36 )( struct stack_36 );
    typedef void * ( __stdcall * stdcall_40 )( struct stack_40 );
    typedef void * ( __stdcall * stdcall_44 )( struct stack_44 );
    typedef void * ( __stdcall * stdcall_48 )( struct stack_48 );
    typedef void * ( __stdcall * stdcall_52 )( struct stack_52 );
    typedef void * ( __stdcall * stdcall_56 )( struct stack_56 );
    typedef void * ( __stdcall * stdcall_60 )( struct stack_60 );
    typedef void * ( __stdcall * stdcall_64 )( struct stack_64 );

    switch( ALIGN_UP( stack_bytes, sizeof( long ) ) )
    {
    case 4:
        if ( GR_CDECL == ct )
            return ((cdecl_4)addr)( * (struct stack_4 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_4)addr)( * (struct stack_4 *)stack );
    case 8:
        if ( GR_CDECL == ct )
            return ((cdecl_8)addr)( * (struct stack_8 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_8)addr)( * (struct stack_8 *)stack );
    case 12:
        if ( GR_CDECL == ct )
            return ((cdecl_12)addr)( * (struct stack_12 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_12)addr)( * (struct stack_12 *)stack );
    case 16:
        if ( GR_CDECL == ct )
            return ((cdecl_16)addr)( * (struct stack_16 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_16)addr)( * (struct stack_16 *)stack );
    case 20:
        if ( GR_CDECL == ct )
            return ((cdecl_20)addr)( * (struct stack_20 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_20)addr)( * (struct stack_20 *)stack );
    case 24:
        if ( GR_CDECL == ct )
            return ((cdecl_24)addr)( * (struct stack_24 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_24)addr)( * (struct stack_24 *)stack );
    case 32:
        if ( GR_CDECL == ct )
            return ((cdecl_32)addr)( * (struct stack_32 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_32)addr)( * (struct stack_32 *)stack );
    case 36:
        if ( GR_CDECL == ct )
            return ((cdecl_36)addr)( * (struct stack_36 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_36)addr)( * (struct stack_36 *)stack );
    case 40:
        if ( GR_CDECL == ct )
            return ((cdecl_40)addr)( * (struct stack_40 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_40)addr)( * (struct stack_40 *)stack );
    case 44:
        if ( GR_CDECL == ct )
            return ((cdecl_44)addr)( * (struct stack_44 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_44)addr)( * (struct stack_44 *)stack );
    case 48:
        if ( GR_CDECL == ct )
            return ((cdecl_48)addr)( * (struct stack_48 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_48)addr)( * (struct stack_48 *)stack );
    case 52:
        if ( GR_CDECL == ct )
            return ((cdecl_52)addr)( * (struct stack_52 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_52)addr)( * (struct stack_52 *)stack );
    case 56:
        if ( GR_CDECL == ct )
            return ((cdecl_56)addr)( * (struct stack_56 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_56)addr)( * (struct stack_56 *)stack );
    case 60:
        if ( GR_CDECL == ct )
            return ((cdecl_60)addr)( * (struct stack_60 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_60)addr)( * (struct stack_60 *)stack );
    case 64:
        if ( GR_CDECL == ct )
            return ((cdecl_64)addr)( * (struct stack_64 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_64)addr)( * (struct stack_64 *)stack );
    default:
        break;
    }

    return NULL;
}

int gr_invoke_r32(
    void *          addr,
    gr_call_type_t  ct,
    void *          stack,
    size_t          stack_bytes
)
{
    typedef int ( __cdecl * cdecl_4 )( struct stack_4 );
    typedef int ( __cdecl * cdecl_8 )( struct stack_8 );
    typedef int ( __cdecl * cdecl_12 )( struct stack_12 );
    typedef int ( __cdecl * cdecl_16 )( struct stack_16 );
    typedef int ( __cdecl * cdecl_20 )( struct stack_20 );
    typedef int ( __cdecl * cdecl_24 )( struct stack_24 );
    typedef int ( __cdecl * cdecl_28 )( struct stack_28 );
    typedef int ( __cdecl * cdecl_32 )( struct stack_32 );
    typedef int ( __cdecl * cdecl_36 )( struct stack_36 );
    typedef int ( __cdecl * cdecl_40 )( struct stack_40 );
    typedef int ( __cdecl * cdecl_44 )( struct stack_44 );
    typedef int ( __cdecl * cdecl_48 )( struct stack_48 );
    typedef int ( __cdecl * cdecl_52 )( struct stack_52 );
    typedef int ( __cdecl * cdecl_56 )( struct stack_56 );
    typedef int ( __cdecl * cdecl_60 )( struct stack_60 );
    typedef int ( __cdecl * cdecl_64 )( struct stack_64 );

    typedef int ( __stdcall * stdcall_4 )( struct stack_4 );
    typedef int ( __stdcall * stdcall_8 )( struct stack_8 );
    typedef int ( __stdcall * stdcall_12 )( struct stack_12 );
    typedef int ( __stdcall * stdcall_16 )( struct stack_16 );
    typedef int ( __stdcall * stdcall_20 )( struct stack_20 );
    typedef int ( __stdcall * stdcall_24 )( struct stack_24 );
    typedef int ( __stdcall * stdcall_28 )( struct stack_28 );
    typedef int ( __stdcall * stdcall_32 )( struct stack_32 );
    typedef int ( __stdcall * stdcall_36 )( struct stack_36 );
    typedef int ( __stdcall * stdcall_40 )( struct stack_40 );
    typedef int ( __stdcall * stdcall_44 )( struct stack_44 );
    typedef int ( __stdcall * stdcall_48 )( struct stack_48 );
    typedef int ( __stdcall * stdcall_52 )( struct stack_52 );
    typedef int ( __stdcall * stdcall_56 )( struct stack_56 );
    typedef int ( __stdcall * stdcall_60 )( struct stack_60 );
    typedef int ( __stdcall * stdcall_64 )( struct stack_64 );

    switch( ALIGN_UP( stack_bytes, sizeof( long ) ) )
    {
    case 4:
        if ( GR_CDECL == ct )
            return ((cdecl_4)addr)( * (struct stack_4 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_4)addr)( * (struct stack_4 *)stack );
    case 8:
        if ( GR_CDECL == ct )
            return ((cdecl_8)addr)( * (struct stack_8 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_8)addr)( * (struct stack_8 *)stack );
    case 12:
        if ( GR_CDECL == ct )
            return ((cdecl_12)addr)( * (struct stack_12 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_12)addr)( * (struct stack_12 *)stack );
    case 16:
        if ( GR_CDECL == ct )
            return ((cdecl_16)addr)( * (struct stack_16 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_16)addr)( * (struct stack_16 *)stack );
    case 20:
        if ( GR_CDECL == ct )
            return ((cdecl_20)addr)( * (struct stack_20 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_20)addr)( * (struct stack_20 *)stack );
    case 24:
        if ( GR_CDECL == ct )
            return ((cdecl_24)addr)( * (struct stack_24 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_24)addr)( * (struct stack_24 *)stack );
    case 32:
        if ( GR_CDECL == ct )
            return ((cdecl_32)addr)( * (struct stack_32 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_32)addr)( * (struct stack_32 *)stack );
    case 36:
        if ( GR_CDECL == ct )
            return ((cdecl_36)addr)( * (struct stack_36 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_36)addr)( * (struct stack_36 *)stack );
    case 40:
        if ( GR_CDECL == ct )
            return ((cdecl_40)addr)( * (struct stack_40 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_40)addr)( * (struct stack_40 *)stack );
    case 44:
        if ( GR_CDECL == ct )
            return ((cdecl_44)addr)( * (struct stack_44 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_44)addr)( * (struct stack_44 *)stack );
    case 48:
        if ( GR_CDECL == ct )
            return ((cdecl_48)addr)( * (struct stack_48 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_48)addr)( * (struct stack_48 *)stack );
    case 52:
        if ( GR_CDECL == ct )
            return ((cdecl_52)addr)( * (struct stack_52 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_52)addr)( * (struct stack_52 *)stack );
    case 56:
        if ( GR_CDECL == ct )
            return ((cdecl_56)addr)( * (struct stack_56 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_56)addr)( * (struct stack_56 *)stack );
    case 60:
        if ( GR_CDECL == ct )
            return ((cdecl_60)addr)( * (struct stack_60 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_60)addr)( * (struct stack_60 *)stack );
    case 64:
        if ( GR_CDECL == ct )
            return ((cdecl_64)addr)( * (struct stack_64 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_64)addr)( * (struct stack_64 *)stack );
    default:
        break;
    }

    return -99999;
}

long long gr_invoke_r64(
    void *          addr,
    gr_call_type_t  ct,
    void *          stack,
    size_t          stack_bytes
)
{
    typedef long long ( __cdecl * cdecl_4 )( struct stack_4 );
    typedef long long ( __cdecl * cdecl_8 )( struct stack_8 );
    typedef long long ( __cdecl * cdecl_12 )( struct stack_12 );
    typedef long long ( __cdecl * cdecl_16 )( struct stack_16 );
    typedef long long ( __cdecl * cdecl_20 )( struct stack_20 );
    typedef long long ( __cdecl * cdecl_24 )( struct stack_24 );
    typedef long long ( __cdecl * cdecl_28 )( struct stack_28 );
    typedef long long ( __cdecl * cdecl_32 )( struct stack_32 );
    typedef long long ( __cdecl * cdecl_36 )( struct stack_36 );
    typedef long long ( __cdecl * cdecl_40 )( struct stack_40 );
    typedef long long ( __cdecl * cdecl_44 )( struct stack_44 );
    typedef long long ( __cdecl * cdecl_48 )( struct stack_48 );
    typedef long long ( __cdecl * cdecl_52 )( struct stack_52 );
    typedef long long ( __cdecl * cdecl_56 )( struct stack_56 );
    typedef long long ( __cdecl * cdecl_60 )( struct stack_60 );
    typedef long long ( __cdecl * cdecl_64 )( struct stack_64 );

    typedef long long ( __stdcall * stdcall_4 )( struct stack_4 );
    typedef long long ( __stdcall * stdcall_8 )( struct stack_8 );
    typedef long long ( __stdcall * stdcall_12 )( struct stack_12 );
    typedef long long ( __stdcall * stdcall_16 )( struct stack_16 );
    typedef long long ( __stdcall * stdcall_20 )( struct stack_20 );
    typedef long long ( __stdcall * stdcall_24 )( struct stack_24 );
    typedef long long ( __stdcall * stdcall_28 )( struct stack_28 );
    typedef long long ( __stdcall * stdcall_32 )( struct stack_32 );
    typedef long long ( __stdcall * stdcall_36 )( struct stack_36 );
    typedef long long ( __stdcall * stdcall_40 )( struct stack_40 );
    typedef long long ( __stdcall * stdcall_44 )( struct stack_44 );
    typedef long long ( __stdcall * stdcall_48 )( struct stack_48 );
    typedef long long ( __stdcall * stdcall_52 )( struct stack_52 );
    typedef long long ( __stdcall * stdcall_56 )( struct stack_56 );
    typedef long long ( __stdcall * stdcall_60 )( struct stack_60 );
    typedef long long ( __stdcall * stdcall_64 )( struct stack_64 );

    switch( ALIGN_UP( stack_bytes, sizeof( long ) ) )
    {
    case 4:
        if ( GR_CDECL == ct )
            return ((cdecl_4)addr)( * (struct stack_4 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_4)addr)( * (struct stack_4 *)stack );
    case 8:
        if ( GR_CDECL == ct )
            return ((cdecl_8)addr)( * (struct stack_8 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_8)addr)( * (struct stack_8 *)stack );
    case 12:
        if ( GR_CDECL == ct )
            return ((cdecl_12)addr)( * (struct stack_12 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_12)addr)( * (struct stack_12 *)stack );
    case 16:
        if ( GR_CDECL == ct )
            return ((cdecl_16)addr)( * (struct stack_16 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_16)addr)( * (struct stack_16 *)stack );
    case 20:
        if ( GR_CDECL == ct )
            return ((cdecl_20)addr)( * (struct stack_20 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_20)addr)( * (struct stack_20 *)stack );
    case 24:
        if ( GR_CDECL == ct )
            return ((cdecl_24)addr)( * (struct stack_24 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_24)addr)( * (struct stack_24 *)stack );
    case 32:
        if ( GR_CDECL == ct )
            return ((cdecl_32)addr)( * (struct stack_32 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_32)addr)( * (struct stack_32 *)stack );
    case 36:
        if ( GR_CDECL == ct )
            return ((cdecl_36)addr)( * (struct stack_36 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_36)addr)( * (struct stack_36 *)stack );
    case 40:
        if ( GR_CDECL == ct )
            return ((cdecl_40)addr)( * (struct stack_40 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_40)addr)( * (struct stack_40 *)stack );
    case 44:
        if ( GR_CDECL == ct )
            return ((cdecl_44)addr)( * (struct stack_44 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_44)addr)( * (struct stack_44 *)stack );
    case 48:
        if ( GR_CDECL == ct )
            return ((cdecl_48)addr)( * (struct stack_48 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_48)addr)( * (struct stack_48 *)stack );
    case 52:
        if ( GR_CDECL == ct )
            return ((cdecl_52)addr)( * (struct stack_52 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_52)addr)( * (struct stack_52 *)stack );
    case 56:
        if ( GR_CDECL == ct )
            return ((cdecl_56)addr)( * (struct stack_56 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_56)addr)( * (struct stack_56 *)stack );
    case 60:
        if ( GR_CDECL == ct )
            return ((cdecl_60)addr)( * (struct stack_60 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_60)addr)( * (struct stack_60 *)stack );
    case 64:
        if ( GR_CDECL == ct )
            return ((cdecl_64)addr)( * (struct stack_64 *)stack );
        else if ( GR_STDCALL == ct )
            return ((stdcall_64)addr)( * (struct stack_64 *)stack );
    default:
        break;
    }

    return -99999LL;
}

void gr_invoke_void(
    void *          addr,
    gr_call_type_t  ct,
    void *          stack,
    size_t          stack_bytes
)
{
    typedef void ( __cdecl * cdecl_4 )( struct stack_4 );
    typedef void ( __cdecl * cdecl_8 )( struct stack_8 );
    typedef void ( __cdecl * cdecl_12 )( struct stack_12 );
    typedef void ( __cdecl * cdecl_16 )( struct stack_16 );
    typedef void ( __cdecl * cdecl_20 )( struct stack_20 );
    typedef void ( __cdecl * cdecl_24 )( struct stack_24 );
    typedef void ( __cdecl * cdecl_28 )( struct stack_28 );
    typedef void ( __cdecl * cdecl_32 )( struct stack_32 );
    typedef void ( __cdecl * cdecl_36 )( struct stack_36 );
    typedef void ( __cdecl * cdecl_40 )( struct stack_40 );
    typedef void ( __cdecl * cdecl_44 )( struct stack_44 );
    typedef void ( __cdecl * cdecl_48 )( struct stack_48 );
    typedef void ( __cdecl * cdecl_52 )( struct stack_52 );
    typedef void ( __cdecl * cdecl_56 )( struct stack_56 );
    typedef void ( __cdecl * cdecl_60 )( struct stack_60 );
    typedef void ( __cdecl * cdecl_64 )( struct stack_64 );

    typedef void ( __stdcall * stdcall_4 )( struct stack_4 );
    typedef void ( __stdcall * stdcall_8 )( struct stack_8 );
    typedef void ( __stdcall * stdcall_12 )( struct stack_12 );
    typedef void ( __stdcall * stdcall_16 )( struct stack_16 );
    typedef void ( __stdcall * stdcall_20 )( struct stack_20 );
    typedef void ( __stdcall * stdcall_24 )( struct stack_24 );
    typedef void ( __stdcall * stdcall_28 )( struct stack_28 );
    typedef void ( __stdcall * stdcall_32 )( struct stack_32 );
    typedef void ( __stdcall * stdcall_36 )( struct stack_36 );
    typedef void ( __stdcall * stdcall_40 )( struct stack_40 );
    typedef void ( __stdcall * stdcall_44 )( struct stack_44 );
    typedef void ( __stdcall * stdcall_48 )( struct stack_48 );
    typedef void ( __stdcall * stdcall_52 )( struct stack_52 );
    typedef void ( __stdcall * stdcall_56 )( struct stack_56 );
    typedef void ( __stdcall * stdcall_60 )( struct stack_60 );
    typedef void ( __stdcall * stdcall_64 )( struct stack_64 );

    switch( ALIGN_UP( stack_bytes, sizeof( long ) ) )
    {
    case 4:
        if ( GR_CDECL == ct )
            ((cdecl_4)addr)( * (struct stack_4 *)stack );
        else if ( GR_STDCALL == ct )
            ((stdcall_4)addr)( * (struct stack_4 *)stack );
        break;
    case 8:
        if ( GR_CDECL == ct )
            ((cdecl_8)addr)( * (struct stack_8 *)stack );
        else if ( GR_STDCALL == ct )
            ((stdcall_8)addr)( * (struct stack_8 *)stack );
        break;
    case 12:
        if ( GR_CDECL == ct )
            ((cdecl_12)addr)( * (struct stack_12 *)stack );
        else if ( GR_STDCALL == ct )
            ((stdcall_12)addr)( * (struct stack_12 *)stack );
        break;
    case 16:
        if ( GR_CDECL == ct )
            ((cdecl_16)addr)( * (struct stack_16 *)stack );
        else if ( GR_STDCALL == ct )
            ((stdcall_16)addr)( * (struct stack_16 *)stack );
        break;
    case 20:
        if ( GR_CDECL == ct )
            ((cdecl_20)addr)( * (struct stack_20 *)stack );
        else if ( GR_STDCALL == ct )
            ((stdcall_20)addr)( * (struct stack_20 *)stack );
        break;
    case 24:
        if ( GR_CDECL == ct )
            ((cdecl_24)addr)( * (struct stack_24 *)stack );
        else if ( GR_STDCALL == ct )
            ((stdcall_24)addr)( * (struct stack_24 *)stack );
        break;
    case 32:
        if ( GR_CDECL == ct )
            ((cdecl_32)addr)( * (struct stack_32 *)stack );
        else if ( GR_STDCALL == ct )
            ((stdcall_32)addr)( * (struct stack_32 *)stack );
        break;
    case 36:
        if ( GR_CDECL == ct )
            ((cdecl_36)addr)( * (struct stack_36 *)stack );
        else if ( GR_STDCALL == ct )
            ((stdcall_36)addr)( * (struct stack_36 *)stack );
        break;
    case 40:
        if ( GR_CDECL == ct )
            ((cdecl_40)addr)( * (struct stack_40 *)stack );
        else if ( GR_STDCALL == ct )
            ((stdcall_40)addr)( * (struct stack_40 *)stack );
        break;
    case 44:
        if ( GR_CDECL == ct )
            ((cdecl_44)addr)( * (struct stack_44 *)stack );
        else if ( GR_STDCALL == ct )
            ((stdcall_44)addr)( * (struct stack_44 *)stack );
        break;
    case 48:
        if ( GR_CDECL == ct )
            ((cdecl_48)addr)( * (struct stack_48 *)stack );
        else if ( GR_STDCALL == ct )
            ((stdcall_48)addr)( * (struct stack_48 *)stack );
        break;
    case 52:
        if ( GR_CDECL == ct )
            ((cdecl_52)addr)( * (struct stack_52 *)stack );
        else if ( GR_STDCALL == ct )
            ((stdcall_52)addr)( * (struct stack_52 *)stack );
        break;
    case 56:
        if ( GR_CDECL == ct )
            ((cdecl_56)addr)( * (struct stack_56 *)stack );
        else if ( GR_STDCALL == ct )
            ((stdcall_56)addr)( * (struct stack_56 *)stack );
        break;
    case 60:
        if ( GR_CDECL == ct )
            ((cdecl_60)addr)( * (struct stack_60 *)stack );
        else if ( GR_STDCALL == ct )
            ((stdcall_60)addr)( * (struct stack_60 *)stack );
        break;
    case 64:
        if ( GR_CDECL == ct )
            ((cdecl_64)addr)( * (struct stack_64 *)stack );
        else if ( GR_STDCALL == ct )
            ((stdcall_64)addr)( * (struct stack_64 *)stack );
        break;
    default:
        break;
    }
}

static inline
void push_stack( char * stk, size_t * stk_len, void * data, size_t data_len )
{
    //TODO: 在这儿做padding

    memcpy( & stk[ * stk_len ], data, data_len );
    * stk_len += data_len;
}

#define prepare_stack( vl, fmt, stk, stk_len )              \
do {                                                        \
    void *      p;                                          \
    int         i;                                          \
    double      d;                                          \
    long        l;                                          \
    long long   n64;                                        \
                                                            \
    while ( * fmt ) {                                       \
                                                            \
        switch( * fmt ++ )                                  \
        {                                                   \
        case 'p':                                           \
        case 's':                                           \
            /* pointer */                                   \
            p = va_arg( vl, void * );                       \
            push_stack( stk, & stk_len, & p, sizeof( p ) ); \
            break;                                          \
        case 'f':                                           \
            /* double */                                    \
            d = va_arg( vl, double );                       \
            push_stack( stk, & stk_len, & d, sizeof( d ) ); \
            break;                                          \
        case 'i':                                           \
        case 'd':                                           \
        case '4':                                           \
            /* int */                                       \
            i = va_arg( vl, int );                          \
            push_stack( stk, & stk_len, & i, sizeof( i ) ); \
            break;                                          \
        case 'l':                                           \
            /* long */                                      \
            l = va_arg( vl, long );                         \
            push_stack( stk, & stk_len, & l, sizeof( l ) ); \
            break;                                          \
        case 'L':                                           \
        case '8':                                           \
            /* 64 bit integer */                            \
            n64 = va_arg( vl, long long );                  \
            push_stack( stk, & stk_len, &n64, sizeof(n64) );\
            break;                                          \
        default:                                            \
            stk_len = (size_t)-1;                           \
            break;                                          \
        }                                                   \
    }                                                       \
} while( 0 );

bool gr_call_rb(
    void *          func_addr,
    gr_call_type_t  ct,
    const char *    fmt,
    ...
)
{
    va_list     vl;
    stack       stk;
    size_t      stk_len = 0;

    va_start( vl, fmt );
    prepare_stack( vl, fmt, ((char *)& stk), stk_len );
    //va_end( vl );
    if ( (size_t)-1 == stk_len ) {
        return false;
    }

    return gr_invoke_rb( func_addr, ct, & stk, stk_len );
}

int gr_call_r32(
    void *          func_addr,
    gr_call_type_t  ct,
    const char *    fmt,
    ...
)
{
    va_list     vl;
    stack       stk;
    size_t      stk_len = 0;

    va_start( vl, fmt );
    prepare_stack( vl, fmt, ((char *)& stk), stk_len );
    //va_end( vl );
    if ( (size_t)-1 == stk_len ) {
        return -1;
    }

    return gr_invoke_r32( func_addr, ct, & stk, stk_len );
}

long long gr_call_r64(
    void *          func_addr,
    gr_call_type_t  ct,
    const char *    fmt,
    ...
)
{
    va_list     vl;
    stack       stk;
    size_t      stk_len = 0;

    va_start( vl, fmt );
    prepare_stack( vl, fmt, ((char *)& stk), stk_len );
    //va_end( vl );
    if ( (size_t)-1 == stk_len ) {
        return -1LL;
    }

    return gr_invoke_r64( func_addr, ct, & stk, stk_len );
}

void * gr_call_rp(
    void *          func_addr,
    gr_call_type_t  ct,
    const char *    fmt,
    ...
)
{
    va_list     vl;
    stack       stk;
    size_t      stk_len = 0;

    va_start( vl, fmt );
    prepare_stack( vl, fmt, ((char *)& stk), stk_len );
    //va_end();
    if ( (size_t)-1 == stk_len ) {
        return NULL;
    }

    return gr_invoke_rp( func_addr, ct, & stk, stk_len );
}

void gr_call_void(
    void *          func_addr,
    gr_call_type_t  ct,
    const char *    fmt,
    ...
)
{
    va_list     vl;
    stack       stk;
    size_t      stk_len = 0;

    va_start( vl, fmt );
    prepare_stack( vl, fmt, ((char *)& stk), stk_len );
    //va_end();
    if ( (size_t)-1 == stk_len ) {
        return;
    }

    gr_invoke_void( func_addr, ct, & stk, stk_len );
}
