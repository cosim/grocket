/**
 * @file libgrocket/gr_config.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   配置文件，速度很慢，如果要频繁取配置项，自己cache到自己那里
 * Revision History 大事件记
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-05    Created.
 **/
#include "gr_config.h"
#include "gr_errno.h"
#include "gr_log.h"
#include "gr_global.h"
#include "gr_tools.h"
#include "gr_dll.h"
#include "gr_mem.h"
#include "gr_ini.h"

static inline
bool
calc_ini_path(
    char * path,
    size_t path_max
)
{
    char * p;
    char * p2;

    // config path: same with current process path, suffix by .ini
    path[ 0 ] = '\0';
    get_exe_path( path, path_max );
    if ( '\0' == path[ 0 ] ) {
        gr_error( "get_exe_path failed" );
        return false;
    }

    // find file name
    p = (char*)strrchr( path, S_PATH_SEP_C );
    if ( NULL == p ) {
        gr_error( "%s not found in %s", S_PATH_SEP, path );
        return false;
    }

    p2 = strrchr( p, '.' );
    if ( NULL == p2 )
        strcat( path, ".ini" );
    else
        strcpy( p2, ".ini" );

    if ( ! is_exists( path ) ) {
        path[ 0 ] = '\0';
    }

    return true;
}

static inline
int
calc_module_path(
    char * path,
    size_t path_max
)
{
    char * p;
    char * p2;
    size_t path_len;

    path[ 0 ] = '\0';
    path_len = get_exe_path( path, path_max );
    if ( '\0' == path[ 0 ] ) {
        gr_error( "get_exe_path failed" );
        return GR_ERR_UNKNOWN;
    }

    // find file name
    p = (char*)strrchr( path, S_PATH_SEP_C );
    if ( NULL == p ) {
        gr_error( "%s not found in %s", S_PATH_SEP, path );
        return GR_ERR_UNKNOWN;
    }

    ++ p;
    if ( '.' == * p ) {
        gr_error( "file name can not leadding by '.' %s", path );
        return GR_ERR_UNKNOWN;
    }

    // insert "lib" on non windows
#if ! defined( WIN32 ) && ! defined( WIN64 )
    if ( sizeof( path ) - path_len <= 3 ) {
        gr_error( "path tooooooooo long %s", path );
        return GR_ERR_UNKNOWN;
    }

    memmove( p + 3, p, path_len - (p - (const char *)path) );
    path_len += 3;
    path[ path_len ] = '\0';

    memcpy( p, "lib", 3 );
#endif

#if defined( WIN32 ) || defined( WIN64 )
    #define MODULE_SUFFIX   "_module.dll"
#elif defined( __linux )
    #define MODULE_SUFFIX   "_module.so"
#elif defined( __APPLE__ )
    #define MODULE_SUFFIX   "_module.dylib"
#else
    #define MODULE_SUFFIX   "_module.so"
#endif

    p2 = strrchr( p, '.' );
    if ( NULL == p2 )
        strcat( path, MODULE_SUFFIX );
    else
        strcpy( p2, MODULE_SUFFIX );
    
    if ( ! is_exists( path ) ) {
        path[ 0 ] = '\0';
    } else {
        gr_info( "Module file: %s", path );
    }

    return 0;
}

static inline
int
add_port(
    gr_server_t * server,
    void * pip,
    int ip_bytes,
    int port,
    bool is_tcp
)
{
    gr_port_item_t * lp;

    if ( server->ports_count >= COUNT_OF( server->ports ) ) {
        gr_error( "ports too much!" );
        return -1;
    }

    lp = & server->ports[ server->ports_count ];
    lp->is_tcp = is_tcp;
    lp->port = port;
    lp->fd = -1;

    if ( sizeof( struct in_addr ) == ip_bytes ) {
        memset( & lp->addr4, 0, sizeof( lp->addr4 ) );
        lp->addr4.sin_family = AF_INET;
        lp->addr4.sin_port = htons( (u_short)port );
        lp->addr4.sin_addr = * ((struct in_addr *)pip);
        lp->addr_len = sizeof( struct sockaddr_in );
    } else if ( sizeof( struct in6_addr ) == ip_bytes ) {
        gr_fatal( "IPV6 not implement" );
        memset( & lp->addr6, 0, sizeof( lp->addr6 ) );
        //lp->addr4.sin_family = AF_INET;
        //lp->addr4.sin_port = htons( (u_short)port );
        //lp->addr4.sin_addr = * ((struct in_addr *)pip);
        lp->addr_len = sizeof( struct sockaddr_in6 );
        //TODO: no implement IPV6
        return -99;
    } else {
        gr_error( "invalid addr length" );
        return -2;
    }

    if ( is_tcp && server->ports_count > 0 ) {
        if ( ! server->ports[ server->ports_count - 1 ].is_tcp ) {
            gr_error( "all TCP must leading by all UDP" );
            return -3;
        }
    }

    ++ server->ports_count;
    return 0;
}

static
int load_listen_info(
    gr_ini * ini
)
{
    int i;
    int j;
    const char * p;
    bool is_tcp;
    const char * ip;
    const char * port;
    int nport;
    union {
        struct in_addr  nip4;
        struct in6_addr nip6;
    } nip;
    int ip_bytes = 0;
    char key[ 8 ];
    int r;
    gr_server_t * server = & g_ghost_rocket_global.server_interface;

    server->ports_count = 0;

    // [listen]
    // 0 = tcp://0.0.0.0:65535
    // 1 = udp://0.0.0.0:65535

    for ( j = 0; j != 2; ++ j ) {

        for ( i = 0; i != GR_PORT_MAX; ++ i ) {

            sprintf( key, "%d", i );

            p = gr_ini_get_string( ini, "listen", key, NULL );
            if ( NULL == p || '\0' == * p ) {
                break;
            }

            if ( 0 == strncmp( "tcp://", p, 6 ) ) {
                if ( 0 != j ) {
                    continue;
                }
                is_tcp = true;
                ip = p + 6;
            } else if ( 0 == strncmp( "udp://", p, 6 ) ) {
                if ( 1 != j ) {
                    continue;
                }
                is_tcp = false;
                ip = p + 6;
            } else {
                gr_error( "invalid protocol, [listen]%s = %s", key, p );
                return -1;
                break;
            }

            port = strchr( ip, ':' );
            if ( NULL == port ) {
                gr_error( "invalid binding port, [listen]%s = %s", key, p );
                return -2;
                break;
            }

            * (char*)port = '\0';

            nip.nip4.s_addr = inet_addr( ip );
            if ( INADDR_NONE == nip.nip4.s_addr ) {
                //TODO:IPV6 not implemention
                gr_error( "invalid binding ip, [listen]%s = %s", key, p );
                return -3;
                break;
            } else {
                ip_bytes = sizeof( struct in_addr );
            }

            * (char*)port = ':';
            ++ port;
            if ( * port <= 0 || ! isdigit( * port ) ) {
                gr_error( "invalid binding port, [listen]%s = %s", key, p );
                return -4;
                break;
            }
            nport = atoi( port );
            if ( nport <= 0 || nport > 65535 ) {
                gr_error( "invalid binding port, [listen]%s = %s", key, p );
                return -5;
                break;
            }

            r = add_port( server, & nip, ip_bytes, nport, is_tcp );
            if ( 0 != r ) {
                gr_error( "add_port failed, return %d", r );
                return -6;
                break;
            }

            gr_info( "Listen %s port %s", is_tcp ? "TCP" : "UDP", ip );
        }
    }

    if ( 0 == server->ports_count ) {
        gr_error( "load empty in [listen] section" );
        return -7;
    }

    /*
    r = IoListenAll( This->io );
    if ( 0 != r ) {
        LogOut( This->log, "error", "%s:%d IoListenAll return %d", __FILE__, __LINE__, r );
        return -8;
    }
    */

    return 0;
}

int gr_config_init(
    const char *        ini_content,
    size_t              ini_content_len
)
{
    gr_ini *  ini;
    int r = 0;

    if ( NULL != g_ghost_rocket_global.config ) {
        gr_fatal( "[init]gr_config_init already called" );
        return GR_ERR_WRONG_CALL_ORDER;
    }

    ini = (gr_ini *)gr_calloc( 1, sizeof( gr_ini ) );
    if ( NULL == ini ) {
        gr_fatal( "[init]gr_calloc( %d ) failed", (int)sizeof( gr_ini ) );
        return GR_ERR_BAD_ALLOC;
    }

    do {

        if ( ini_content && * ini_content && ini_content_len ) {

            // 从内存里装载配置文件

            r = gr_ini_open_memory( ini, ini_content, ini_content_len );
            if ( 0 != r ) {
                gr_error( "[init]gr_ini_open_memory %d bytes failed, return %d",
                    (int)ini_content_len, r );
                break;
            }

        } else {

            char path[ MAX_PATH ];
            if ( ! calc_ini_path( path, sizeof( path ) ) ) {
                gr_error( "[init]calc_ini_path failed" );
                r = GR_ERR_UNKNOWN;
                break;
            }

            if ( '\0' == path[ 0 ] ) {

                // 配置文件不存在
                // 试图从模块装载

                char *          buf;
                int             buf_len = 0;
                gr_dll_t           h = NULL;
                gr_config_t     f;
                const int       buf_max = 2048; //TODO: 2K够吗？

                buf = (char *)gr_malloc( buf_max );
                if ( NULL == buf ) {
                    gr_error( "[init]allocate memory failed" );
                    return GR_ERR_BAD_ALLOC;
                }

                do {

                    r = calc_module_path( path, sizeof( path ) );
                    if ( 0 != r || '\0' == path[ 0 ] ) {
                        gr_error( "[init]calc_module_path failed: %d", r );
                        r = GR_ERR_UNKNOWN;
                        break;
                    }

                    h = gr_dll_open( path );
                    if ( NULL == h ) {
                        gr_error( "[init]gr_dll_open '%s' failed. (%d)", path, get_errno() );
                        r = GR_ERR_UNKNOWN;
                        break;
                    }

                    f = (gr_config_t)gr_dll_symbol( h, GR_CONFIG_FUNC_NAME );
                    if ( NULL == f ) {
                        gr_error( "[init]gr_dll_symbol '%s' failed", GR_CONFIG_FUNC_NAME );
                        r = GR_ERR_UNKNOWN;
                        break;
                    }

                    if ( ! f( buf, buf_max, & buf_len ) ) {
                        gr_error( "[init]%s call failed", GR_CONFIG_FUNC_NAME );
                        r = GR_ERR_UNKNOWN;
                        break;
                    }

                    gr_dll_close( h );
                    h = NULL;

                    r = gr_ini_open_memory( ini, buf, buf_len );
                    if ( 0 != r ) {
                        gr_error( "[init]gr_ini_open_memory %d bytes failed, return %d", (int)buf_len, r );
                        r = GR_ERR_UNKNOWN;
                        break;
                    }

                    r = 0;

                } while( 0 );

                if ( h ) {
                    gr_dll_close( h );
                    h = NULL;
                }

                assert( buf );
                gr_free( buf );

            } else {

                // 配置文件存在

                r = gr_ini_open( ini, path );
                if ( 0 != r ) {
                    gr_error( "[init]gr_ini_open %s failed, return %d", path, r );
                    break;
                }
            }
        }

    } while ( false );

    if ( 0 != r ) {
        gr_ini_close( ini );
        gr_free( ini );
        ini = NULL;
        return r;
    }

    // 装载配置中的端口信息
    r = load_listen_info( ini );
    if ( 0 != r ) {
        gr_fatal( "[init]load_listen_info() return %d", r );
        gr_ini_close( ini );
        gr_free( ini );
        ini = NULL;
        return r;
    }

    g_ghost_rocket_global.config = ini;

    // 返回数据包对齐字节数
    g_ghost_rocket_global.rsp_buf_align = gr_config_rsp_buff_align();

    // worker count
    g_ghost_rocket_global.server_interface.worker_count =
        gr_config_worker_thread_count();

    // initialize magic, interface version
    g_ghost_rocket_global.server_interface.version      = GR_SERVER_VERSION;
    g_ghost_rocket_global.server_interface.low_version  = GR_SERVER_LOW_VERSION;
    memcpy( g_ghost_rocket_global.server_interface.magic, GR_SERVER_MAGIC, sizeof( GR_SERVER_MAGIC ) - 1 );

    return 0;
}

void gr_config_term()
{
    gr_ini *  ini = (gr_ini *)g_ghost_rocket_global.config;

    if ( NULL != ini ) {
        g_ghost_rocket_global.config = NULL;
        gr_ini_close( ini );
        gr_free( ini );
    }

}

static inline
bool config_get_bool(
    gr_ini *  ini,
    const char *        section,
    const char *        name,
    bool                def
)
{
    return gr_ini_get_bool( ini, section, name, def );
}


static inline
int config_get_int(
    gr_ini *  ini,
    const char *        section,
    const char *        name,
    int                 def
)
{
    return gr_ini_get_int( ini, section, name, def );
}

static inline
const char * config_get_string(
    gr_ini *  ini,
    const char *        section,
    const char *        name,
    const char *        def
)
{
    return gr_ini_get_string( ini, section, name, def );
}

bool gr_config_is_daemon()
{
    return config_get_bool( g_ghost_rocket_global.config, "server", "daemon", false );
}

bool gr_config_is_tcp_disabled()
{
    /*
    // 默认TCP启动就可用
    server->is_tcp_disabled = gr_config_is_tcp_disabled();
    if ( server->is_tcp_disabled ) {
        gr_info( "[init]Manual listen TCP, now TCP disabled" );
    }
    */
    return config_get_bool( g_ghost_rocket_global.config, "server", "tcp.manual_open", false );
}

int gr_config_tcp_in_concurrent()
{
    static const int def = 10000;
    int n = config_get_int( g_ghost_rocket_global.config, "server", "tcp.in.concurrent", def );
    if ( n < def ) {
        n = def;
    }

    return n;
}

int gr_config_tcp_in_thread_count()
{
    static const int def = 1;
    int n = config_get_int( g_ghost_rocket_global.config, "server", "tcp.in.thread_count", def );
    if ( n < def ) {
        n = def;
    }

    return n;
}

int gr_config_udp_out_concurrent()
{
    static const int def = 10000;
    int n = config_get_int( g_ghost_rocket_global.config, "server", "udp.out.concurrent", def );
    if ( n < def ) {
        n = def;
    }

    return n;
}

int gr_config_udp_in_concurrent()
{
    static const int def = 10000;
    int n = config_get_int( g_ghost_rocket_global.config, "server", "udp.in.concurrent", def );
    if ( n < def ) {
        n = def;
    }

    return n;
}

int gr_config_udp_in_thread_count()
{
    static const int def = 1;
    int n = config_get_int( g_ghost_rocket_global.config, "server", "udp.in.thread_count", def );
    if ( n < def ) {
        n = def;
    }

    return n;
}

int gr_config_tcp_accept_concurrent()
{
    static const int def = 10000;
    int n = config_get_int( g_ghost_rocket_global.config, "server", "tcp.accept.concurrent", def );
    if ( n < def ) {
        n = def;
    }

    return n;
}

int gr_config_tcp_accept_thread_count()
{
    static const int def = 1;
    int n = config_get_int( g_ghost_rocket_global.config, "server", "tcp.accept.thread_count", def );
    if ( n < def ) {
        n = def;
    }

    return n;
}

int gr_config_tcp_out_concurrent()
{
    static const int def = 10000;
    int n = config_get_int( g_ghost_rocket_global.config, "server", "tcp.out.concurrent", def );
    if ( n < def ) {
        n = def;
    }

    return n;
}

int gr_config_tcp_out_thread_count()
{
    static const int def = 1;
    int n = config_get_int( g_ghost_rocket_global.config, "server", "tcp.out.thread_count", def );
    if ( n < def ) {
        n = def;
    }

    return n;
}

int gr_config_udp_out_thread_count()
{
    static const int def = 1;
    int n = config_get_int( g_ghost_rocket_global.config, "server", "udp.out.thread_count", def );
    if ( n < def ) {
        n = def;
    }

    return n;
}

int gr_config_worker_thread_count()
{
    static const int def = 1;
    int n = config_get_int( g_ghost_rocket_global.config, "server", "worker.thread_count", def );
    if ( n < def ) {
        n = def;
    }

    return n;
}

int gr_config_backend_thread_count()
{
    static const int def = 1;
    int n = config_get_int( g_ghost_rocket_global.config, "server", "backend.thread_count", def );
    if ( n < def ) {
        n = def;
    }

    return n;
}

void gr_config_get_module_path( char * path, size_t path_max, bool * is_absolute )
{
    const char * p;
    int r;

    p = gr_ini_get_string( g_ghost_rocket_global.config, "server", "module", NULL );
    if ( NULL != p && * p ) {
        strncpy( path, p, path_max );
        path[ path_max - 1 ] = '\0';
        * is_absolute = false;
        return;
    }

    r = calc_module_path( path, path_max );
    if ( 0 != r ) {
        gr_error( "calc_module_path failed, return %d", r );
        path[ 0 ] = '\0';
        return;
    }
    * is_absolute = true;
}

int gr_config_tcp_accept_send_buf()
{
    static const int def = 8 * 1024 * 1024;
    int n = config_get_int( g_ghost_rocket_global.config, "server", "tcp.accept.send_buf", def );
    if ( n < 0 ) {
        n = 0;
    }

    return n;
}

int gr_config_tcp_accept_recv_buf()
{
    static const int def = 8 * 1024 * 1024;
    int n = config_get_int( g_ghost_rocket_global.config, "server", "tcp.accept.recv_buf", def );
    if ( n < 0 ) {
        n = 0;
    }

    return n;
}

int gr_config_udp_send_buf()
{
    static const int def = 8 * 1024 * 1024;
    int n = config_get_int( g_ghost_rocket_global.config, "server", "udp.send_buf", def );
    if ( n < 0 ) {
        n = 0;
    }

    return n;
}

int gr_config_udp_recv_buf()
{
    static const int def = 8 * 1024 * 1024;
    int n = config_get_int( g_ghost_rocket_global.config, "server", "udp.recv_buf", def );
    if ( n < 0 ) {
        n = 0;
    }

    return n;
}

int gr_config_get_listen_backlog()
{
    static const int def = 511;
    int n = config_get_int( g_ghost_rocket_global.config, "server", "tcp.accept.backlog", def );
    if ( n < 5 ) {
        n = 5;
    }

    return n;
}

int gr_config_get_tcp_recv_buf_init()
{
    static const int def = 256;
    int n = config_get_int( g_ghost_rocket_global.config, "server", "tcp.in.recv_buf.init", def );
    if ( n < 4 ) {
        n = 4;
    }

    return n;
}

int gr_config_get_tcp_recv_buf_max()
{
    static const int def = 1024 * 1024 * 10;
    int min_val = gr_config_get_tcp_recv_buf_init();
    int n = config_get_int( g_ghost_rocket_global.config, "server", "tcp.in.recv_buf.max", def );
    if ( n < min_val ) {
        n = min_val;
    }

    return n;
}

int gr_config_log_level( int def )
{
    const char * s = config_get_string( g_ghost_rocket_global.config, "server", "log_level", NULL );
    if ( NULL != s ) {
        if ( 0 == stricmp( "all", s ) ) {
            return GR_LOG_ALL;
        } else if ( 0 == stricmp( "debug", s ) ) {
            return GR_LOG_DEBUG;
        } else if ( 0 == stricmp( "info", s ) ) {
            return GR_LOG_INFO;
        } else if ( 0 == stricmp( "warning", s ) ) {
            return GR_LOG_WARNING;
        } else if ( 0 == stricmp( "error", s ) ) {
            return GR_LOG_ERROR;
        } else if ( 0 == stricmp( "fatal", s ) ) {
            return GR_LOG_FATAL;
        } else if ( 0 == stricmp( "none", s ) ) {
            return GR_LOG_NONE;
        }
    }

    return def;
}

int gr_config_library_class_max()
{
    static const int def = 100;
    int min_val = 10;
    int n = config_get_int( g_ghost_rocket_global.config, "server", "library.class_max", def );
    if ( n < min_val ) {
        n = min_val;
    }

    return n;
}

const char * gr_config_library_core_path()
{
    static const char * def = "./library";
    const char * p = config_get_string( g_ghost_rocket_global.config, "server", "library.core", def );
    if ( NULL == p || '\0' == * p ) {
        return NULL;
    }

    return p;
}

int gr_config_rsp_buff_align()
{
    static const int def = 1024;
    int min_val = 8;
    int n = config_get_int( g_ghost_rocket_global.config, "server", "response.buff_align", def );
    if ( n < min_val ) {
        n = min_val;
    }

    return n;
}
