/**
 * @file libgrocket/server_object.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/09
 * @version $Revision$ 
 * @brief   
 * Revision History ���¼���
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-09    Created.
 **/
#include "server_object.h"
#include "gr_ini.h"
#include "gr_global.h"
#include "gr_mem.h"
#include "gr_log.h"
#include <stdarg.h>

static inline
bool check( gr_i_server_t * self, void * func1, void * func2 )
{
    // һ����ȫ��飬��ֹ�û��Ѵ����ͷ�ļ��ŵ��˴���Ķ����ƶ���ָ����
    return (void*)g_ghost_rocket_global.server_interface.library->classes[ CLASS_SERVER ] == (void*)self
        && func1 == func2;
}

static
bool server_class_config_get_bool(
    gr_i_server_t * self,
    const char *    section,
    const char *    name,
    bool            default_value )
{
    gr_ini * ini;

    if ( ! check( self, (void*)self->config_get_bool, (void*)server_class_config_get_bool ) ) {
        // ����ʹ��default_value����Ϊ����ջ�����Ǵ��
        return false;
    }

    ini = (gr_ini *)g_ghost_rocket_global.config;
    return gr_ini_get_bool( ini, section, name, default_value );
}

static
int server_class_config_get_int(
    gr_i_server_t * self,
    const char *    section,
    const char *    name,
    int             default_value )
{
    gr_ini * ini;

    if ( ! check( self, (void*)self->config_get_int, (void*)server_class_config_get_int ) ) {
        // ����ʹ��default_value����Ϊ����ջ�����Ǵ��
        return -1;
    }

    ini = (gr_ini *)g_ghost_rocket_global.config;
    return gr_ini_get_int( ini, section, name, default_value );
}

static
const char * server_class_config_get_string(
    gr_i_server_t * self,
    const char *    section,
    const char *    name,
    const char *    default_value )
{
    gr_ini * ini;

    if ( ! check( self, (void*)self->config_get_string, (void*)server_class_config_get_string ) ) {
        // ����ʹ��default_value����Ϊ����ջ�����Ǵ��
        return NULL;
    }

    ini = (gr_ini *)g_ghost_rocket_global.config;
    return gr_ini_get_string( ini, section, name, default_value );
}

// ������󷵻ذ�����
static
void * server_class_set_max_response(
    gr_i_server_t *     self,
    gr_proc_ctxt_t *    ctxt,
    size_t              bytes )
{
    if ( ctxt->pc_result_buf_max >= (int)bytes ) {
        assert( ctxt->pc_result_buf );
        ctxt->pc_result_buf_len = 0;
        return ctxt->pc_result_buf;
    } else {

        void * p;

        // align to K
        bytes = ALIGN_UP( bytes, g_ghost_rocket_global.rsp_buf_align );

        p = gr_malloc( bytes );
        if ( p ) {

            void * op = ctxt->pc_result_buf;

            ctxt->pc_result_buf_len = 0;
            ctxt->pc_result_buf = (char *)p;
            ctxt->pc_result_buf_max = (int)bytes;

            gr_free( op );
            return ctxt->pc_result_buf;
        } else {
            gr_error( "malloc %d bytes failed", (int)bytes );
            return NULL;
        }
    }
}

static
void server_class_log(
    gr_i_server_t * self,
    const char *    file,
    int             line,
    const char *    func,
    gr_log_level_t  level,
    const char *    fmt,
    ... )
{
    if ( g_ghost_rocket_global.log_start_level <= level ) {
        char        buf[ 1024 ] = "";
        va_list     vl;

        if ( ! check( self, (void*)self->log, (void*)server_class_log ) ) {
            return;
        }

        va_start( vl, fmt );
        vsnprintf( buf, sizeof(buf), fmt, vl );
        gr_log_write( file, line, func, level, "%s", buf );
    }
}

bool server_class_construct( server_class_t * sc )
{
    // ��ʼ�����ܺ���
    sc->face.config_get_bool    = server_class_config_get_bool;
    sc->face.config_get_int     = server_class_config_get_int;
    sc->face.config_get_string  = server_class_config_get_string;
    sc->face.set_max_response   = server_class_set_max_response;
    sc->face.log                = server_class_log;

    return true;
}
