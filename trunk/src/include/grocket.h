/**
 * @file include/grocket.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/09/24
 * @version $Revision$ 
 * @brief   服务器框架扩展模块头文件，调用方只需要这一个头文件即可编写模块
 * @warning 在包含本文件前，调用方必须保证
 *          uint16_t, uint32_t, socklen_t, bool, size_t, sockaddr_in, sockaddr_in6
 *          上述数据类型都可用（如果是C99以前的C语言，保证bool是一个字节）。
 *          如此设计的目的是防止包头文件冲突，有时这类问题非常难解决。
 *          本程序提供了一个gr_stdinc.h头文件确认了上述数据类型都可用，
 *          调用方可以选择是否使用。
 * Revision History 大事件记
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-09-24    Created.
 **/

#ifndef _GHOST_ROCKET_INCLUDE_GROCKET_H_
#define _GHOST_ROCKET_INCLUDE_GROCKET_H_

#ifdef __cplusplus
extern "C" {
#endif

        struct gr_server_t;
typedef struct gr_server_t  gr_server_t;

typedef enum
{
    // 主进程, 适用于主进程初始化资源, 子进程使用资源的场景
    GR_PROCESS_MASTER           = 1,

    // 子进程
    GR_PROCESS_CHILD            = 2,

    // 第一个线程,第二个线程就是本值加1,以此类推
    GR_PROCESS_THREAD_1         = 3

} gr_process_type_t;

// never change at running, so multiple thread concurrent is OK
typedef struct
{
    // listen address
    union
    {
        struct sockaddr         addr;
        struct sockaddr_in      addr4;
        struct sockaddr_in6     addr6;
    };
    socklen_t                   addr_len;

    // SOCKET fd
    int                         fd;
    // listen port, host endian
    uint16_t                    port;

    // is it TCP or UDP
    bool                        is_tcp;

    char                        reserved[ 1 ];

} gr_port_item_t;

// package type
typedef enum
{
    // invalid package
    GR_PACKAGE_ERROR            = 0,

    // HTTP request
    GR_PACKAGE_HTTP_REQ         = 1,

    // HTTP reply
    GR_PACKAGE_HTTP_REPLY       = 2,

    // user package, implementation in module
    GR_PACKAGE_PRIVATE          = 3,

    // use 3 bit to store, so max is 7

} gr_package_type_t;

#pragma pack( push, 4 )
// 为什么将 GsCheckPackageCtxt 的字节数限制在 12 字节？
// 因为它将被放在Connection里，Connection必须控制在64字节大小

typedef struct
{
    // uint16_t                cc_is_tcp              : 1;
    // uint16_t                cc_package_type        : 3;
    // uint32_t                cc_package_length;
    // uint32_t                cc_udp_addr_id;

#define cc_is_tcp              is_tcp
#define cc_package_type        package_type
#define cc_package_length      package_length
#define cc_udp_addr_id         udp_addr_id

    ///////////////////////////////////////////////////////////////////
    // Inner field

    uint16_t                    http_header_offset  : 11;

    uint16_t                    is_tcp              : 1;

    uint16_t                    _reserved_bits      : 1;

    uint16_t                    package_type        : 3;

    uint16_t                    http_body_offset;


    // Content-Length in HTTP, full package length otherwise
    union
    {
        uint32_t                http_content_length;

        // in http, package_length = http_body_offset + http_content_length
        uint32_t                package_length;
    };

    // [init by] Io
    // only for UDP, compatibile with IPV6
    uint32_t                    udp_addr_id;

} gr_check_ctxt_t;

#pragma pack( pop ) // #pragma pack( push, 4 )

typedef struct
{
    // uint16_t                 pc_is_tcp( bit field )
    // uint16_t                 pc_package_type( bit field )
    // uint32_t                 pc_udp_addr_id;
    // int                      pc_port;
    // int                      pc_fd;
    // int                      pc_thread_id;
    // char *                   pc_result_buf;
    // int                      pc_result_buf_max;
    // int *                    pc_result_buf_len;

#define pc_is_tcp               check_ctxt->is_tcp
#define pc_package_type         check_ctxt->package_type
#define pc_udp_addr_id          check_ctxt->udp_addr_id
#define pc_port                 port
#define pc_fd                   fd
#define pc_thread_id            thread_id
#define pc_result_buf           result_buf
#define pc_result_buf_max       result_buf_max
#define pc_result_buf_len       result_buf_len

    ///////////////////////////////////////////////////////////////////
    // Inner field

    gr_check_ctxt_t *          check_ctxt;

    int                         port;
    int                         fd;
    int                         thread_id;

    int                         result_buf_max;
    char *                      result_buf;
    int                         result_buf_len;

} gr_proc_ctxt_t;

typedef struct
{
    // 参数名
    char *                      name;

    // 参数值
    char *                      value;

} gr_http_param_t;


typedef struct
{
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

#define hc_is_tcp               base->check_ctxt->is_tcp
#define hc_package_type         base->check_ctxt->package_type
#define hc_port                 base->port
#define hc_fd                   base->fd
#define hc_thread_id            base->thread_id
#define hc_result_buf           base->result_buf
#define hc_result_buf_max       base->result_buf_max
#define hc_result_buf_len       base->result_buf_len
#define hc_is_error             is_error
#define hc_keep_alive           keep_alive
#define hc_request_type         request_type
#define hc_version              version
#define hc_directory            directory
#define hc_object               object
#define hc_content_type         content_type
#define hc_user_agent           user_agent
#define hc_params               params
#define hc_params_count         params_count
#define hc_header               header
#define hc_header_count         header_count
#define hc_form                 form
#define hc_form_count           form_count
#define hc_body                 body
#define hc_body_len             body_len
#define hc_reply_code           http_reply_code

    ///////////////////////////////////////////////////////////////////
    // Inner field

    gr_proc_ctxt_t *            base;

    // is error happens, default false
    bool                        is_error;
    // is need Connection: Keep-Alive, default true
    bool                        keep_alive;

    // HTTP

    // G, P, H, C
    // '\0' 表示HTTP返回包
    char                        request_type;

    char *                      version; // HTTP/1.1  HTTP/0.9

    // 空内容同时object也为空内容表示返回包
    char *                      directory;

    char *                      object;

    char *                      content_type;

    char *                      user_agent;

    gr_http_param_t *           params;
    size_t                      params_count;

    gr_http_param_t *           header;
    size_t                      header_count;

    gr_http_param_t *           form;
    size_t                      form_count;

    char *                      body;
    size_t                      body_len;

    // 只为HTTP返回包，返回错误码
    int                         http_reply_code;

} gr_http_ctxt_t;

///////////////////////////////////////////////////////////////////////
//
// gr_init_t
//
// init the module
//
// parameters:
//     proc_type      : process type
//                      主进程初始化、子进程初始化、所有线程初始化时都会调用本函数。
//                      那如何区分呢？答案是proc_type参数，它的取值会有如下几种：
//                      1、GR_PROCESS_MASTER 则表示是主进程, 适用于主进程初始化资源, 
//                                           子进程使用资源的场景
//                      2、GR_PROCESS_CHILD 正常的工作进程
//                      3、GR_PROCESS_THREAD_1 表示工作进程中的第一个工作线程初始化
//                      4、GR_PROCESS_THREAD_1 + n 表示工作进程中的第 n 个工作线程初始化
//     server         : pinter to gr_server_t, valid in all server lifecycle.
//                      文档上说：“写服务器模块唯一需要包含的文件，不需要链接任何库”，
//                      那服务器框架导出函数的实现在哪里？答案是：在服务器框架进程里，
//                      它在gr_init函数中通过gr_server_t *接口以函数指针的方式暴露给
//                      用户模块，所以用户模块当然要把这个指针保存起来。
// return:
//     int, initialize OK? 0 if successed, failed otherwise.
// remark:
//     optional function
//
typedef int ( * gr_init_t )(
    gr_process_type_t   proc_type,
    gr_server_t *       server
);
#define GR_INIT_NAME     "gr_init"

///////////////////////////////////////////////////////////////////////
//
// gr_term_t
//
// uninit the module
//
// parameters:
//     proc_type      : process type
//                      主进程反初始化、子进程反初始化、所有线程反初始化时都会调用本函数。
//                      那如何区分呢？答案是proc_type参数，它的取值会有如下几种：
//                      1、GR_PROCESS_MASTER 则表示是主进程, 适用于主进程初始化资源, 
//                                           子进程使用资源的场景
//                      2、GR_PROCESS_CHILD 正常的工作进程
//                      3、GR_PROCESS_THREAD_1 表示工作进程中的第一个工作线程反初始化
//                      4、GR_PROCESS_THREAD_1 + n 表示工作进程中的第 n 个工作线程反初始化
//     must_be_zero   : reserved, must be 0
// remark:
//     optional function
//
typedef void ( * gr_term_t )(
    gr_process_type_t   proc_type,
    void *              must_be_zero
);
#define GR_TERM_NAME     "gr_term"

///////////////////////////////////////////////////////////////////////
//
// gr_tcp_accept
//
//     After accept, this function will call
//
// parameters:
//     port           : 模块可以从port参数得到这个连接是从服务器的哪个监听端口连上来的
//     sock           : return by accept function
//     need_disconnect: 1 if need disconnect the connection, default 0
//                      一但accept了一个TCP连接，则该函数会被调用。
//                      模块可以有选择的决定是否要关掉这个TCP连接，如果要关掉，
//                      就*need_disconnect=1即可。
// remark:
//     optional function
//
typedef void ( * gr_tcp_accept_t )(
    int                 port,
    int                 sock,
    int *               need_disconnect
);
#define GR_TCP_ACCEPT_NAME     "gr_tcp_accept"

///////////////////////////////////////////////////////////////////////
//
// gr_tcp_close
//
// called before connection close
//
// parameters:
//     port           : 模块可以从port参数得到这个要断掉的TCP连接是
//                      从服务器的哪个监听端口连上来的
//     sock           : SOCKET
// remark:
//     option functions
//
typedef void ( * gr_tcp_close_t )(
	int                 port,
    int                 sock
);
#define GR_TCP_CLOSE_NAME     "gr_tcp_close"

///////////////////////////////////////////////////////////////////////
//
// gr_check_t
//
// called when recved new data into a connection
// 针对同一个TCP连接，该函数有可能在I/O线程里调用，也可能在工作线程里调用，但不会产生并发。
// UDP，本函数只可能在I/O线程里调用。
//
// parameters:
//     data           : received data
//     len            : received data len
//     port           : listen port
//     sock           : recv socket
//     ctxt           : function fill data to caller
//     is_error       : is it valie package
//     is_full        : is al full package
// remark:
//     option functions
//
typedef void ( * gr_check_t )(
    void *              data,
    int                 len,
    gr_port_item_t *    port_info,
    int                 sock,
    gr_check_ctxt_t *   ctxt,
    bool *              is_error,
    bool *              is_full
);
#define GR_CHECK_NAME   "gr_check"

///////////////////////////////////////////////////////////////////////
//
// gr_proc_t
//
// called when process a full binary package( call by worker thread )
// 服务器承诺，一个TCP连接的所有请求会分配到一个固定的工作线程上。
//            相同客户端地址的UDP所有请求会分配到一个固定的工作线程上。
//
// parameters:
//     data           : full package data
//     len            : full package data len
//     ctxt           : process context
//     processed_len  : 返回值 < 0，数据包有错误，需要断连接.
//                      返回值 = 0，数据包正确，但需要服务器端主动断连接。
//                      返回值 > 0。数据包正确，返回已经处理的数据包长度.
// remark:
//     option functions
//
typedef void ( * gr_proc_t )(
    const void *        data,
    int                 len,
    gr_proc_ctxt_t *    ctxt,
    int *               processed_len
);
#define GR_PROC_NAME     "gr_proc"

///////////////////////////////////////////////////////////////////////
//
// gr_poc_http_t
//
// Http process( call by worker thread )
// 服务器承诺，一个TCP连接的所有请求会分配到一个固定的工作线程上。
//            相同客户端地址的UDP所有请求会分配到一个固定的工作线程上。
// HTTP也支持UDP？是的，支持。
//
// parameters:
//     http       : http context
// remark:
//     option functions
//
typedef void ( * gr_proc_http_t )(
    gr_http_ctxt_t *    http
);
#define GR_PROC_HTTP_NAME     "gr_proc_http"

///////////////////////////////////////////////////////////////////////
//
// gr_config_t
//     use this function to avoid the config file
// parameters:
//     buf    : buffer
//     buf_max: buffer capacity
//     buf_len: buffer length
// remark:
//     option functions
//
typedef bool ( * gr_config_t )(
    char * buf,
    int    buf_max,
    int *  buf_len
);
#define GR_CONFIG_FUNC_NAME     "gr_config"

///////////////////////////////////////////////////////////////////////
//
// gr_class_t
// gr_object_t
//
        struct gr_object_t;
typedef struct gr_object_t  gr_object_t;
        struct gr_class_t;
typedef struct gr_class_t   gr_class_t;
        struct gr_library_t;
typedef struct gr_library_t gr_library_t;

struct gr_object_t
{
    // 当前对象实例所属的类
    gr_class_t *    klass;
};

struct gr_class_t
{
    gr_object_t     base;

    // 如果该类是一个单件，则在这里存储单件实例
    gr_object_t *   singleton;

    // 删除类
    void            ( * destroy_class )( gr_class_t *    self );

    // 创建类实例
    gr_object_t *   ( * create_object )( gr_class_t *    self );

    // 删除类实例
    void            ( * destroy_object )( gr_object_t *   object );
};

#define     GR_CLASS_DECLARE_BEGIN( name, interface_type )  \
    struct name##_class_t;                                  \
    typedef struct name##_class_t name##_class_t;           \
    struct name##_object_t;                                 \
    typedef struct name##_object_t name##_object_t;         \
                                                            \
    struct name##_class_t                                   \
    {                                                       \
        union {                                             \
            gr_class_t      base;                           \
            interface_type  face;                           \
        };


#define     GR_CLASS_DECLARE_OBJECT( name )                 \
    };                                                      \
                                                            \
    struct name##_object_t                                  \
    {                                                       \
        gr_object_t     base;


#define     GR_CLASS_DECLARE_END( name )                    \
    };

#define     GR_CLASS_DECLARE_SINGLETON( name )              \
    name##_class_t      name##_class_singleton;

#define GR_CLASS_INSTALL_SINGLETON( library, parent, name, id )                                         \
do {                                                                                                    \
    bool cls_ok = name##_class_construct( & (parent)->name##_class_singleton );                         \
    (parent)->name##_class_singleton.base.base.klass    = & (parent)->name##_class_singleton.base;      \
    (parent)->name##_class_singleton.base.singleton     = & (parent)->name##_class_singleton.base.base; \
    (parent)->name##_class_singleton.base.destroy_class = NULL;                                         \
    (parent)->name##_class_singleton.base.create_object = NULL;                                         \
    (parent)->name##_class_singleton.base.destroy_object= NULL;                                         \
    if ( cls_ok ) {                                                                                     \
        (library)->classes[ (id) ] = & (parent)->name##_class_singleton.base;                           \
    } else {                                                                                            \
        (library)->classes[ (id) ] = NULL;                                                              \
    }                                                                                                   \
} while( false );

///////////////////////////////////////////////////////////////////////
//
//
//
        struct gr_i_server_t;
typedef struct gr_i_server_t gr_i_server_t;

typedef enum
{
    // 允许所有日志输出
    GR_LOG_ALL      = 0,
    // 允许debug及更高级别的日志输出
    GR_LOG_DEBUG    = 1,
    // 允许info及更高级别的日志输出
    GR_LOG_INFO     = 2,
    // 允许warning及更高级别的日志输出
    GR_LOG_WARNING  = 3,
    // 允许error及更高级别的日志输出
    GR_LOG_ERROR    = 4,
    // 允许fatal及更高级别的日志输出
    GR_LOG_FATAL    = 5,
    // 不允许任何日志输出
    GR_LOG_NONE     = 6,
} gr_log_level_t;

struct gr_i_server_t
{
    gr_class_t      base;

    // 取配置文件bool类型字段
    bool            ( * config_get_bool )(  gr_i_server_t *     self,
                                            const char *        section,
                                            const char *        name,
                                            bool                default_value );

    // 取配置文件int类型字段
    int             ( * config_get_int )(   gr_i_server_t *     self,
                                            const char *        section,
                                            const char *        name,
                                            int                 default_value );

    // 取配置文件字符串类型字段
    const char *    ( * config_get_string )(gr_i_server_t *     self,
                                            const char *        section,
                                            const char *        name,
                                            const char *        default_value );

    // 设置最大返回包长度
    void *          ( * set_max_response )( gr_i_server_t *     self,
                                            gr_proc_ctxt_t *    ctxt,
                                            size_t              bytes );

    // 输出日志
    void            ( * log )(  gr_i_server_t * self,
                                const char *    file,
                                int             line,
                                const char *    func,
                                gr_log_level_t  level,
                                const char *    fmt,
                                ... );

};

///////////////////////////////////////////////////////////////////////
//
// gr_library_t
//

#define GR_LIBRARY_MAGIC        "GL"
#define GR_LIBRARY_VERSION      1
#define GR_LIBRARY_LOW_VERSION  1

struct gr_library_t
{
    // 接口标记。鬼知道会不会有一天用到。GR_LIBRARY_MAGIC
    char            magic[ 2 ];

    // 当前接口版本号。只有本结构有修改时才升级版本号。GR_LIBRARY_VERSION
    unsigned char   version;

    // 当前接口实现可兼容的最低版本号，调用方必须大于等于该版本号才能使用。GR_LIBRARY_LOW_VERSION
    unsigned char   low_version;

    // classes 类的最大数量
    uint32_t        class_max;

    // classes 类
    gr_class_t *    classes[ 1 ];

};

typedef enum
{
    // 服务器内置类的编号
    CLASS_SERVER    = 0

} gr_class_id_t;

///////////////////////////////////////////////////////////////////////
//
// gr_library_init
//
// parameters:
//     http       : http context
// remark:
//     option functions
//
typedef int ( * gr_library_init_t )(
    gr_library_t *  library
);
#define GR_LIBRARY_INIT_NAME    "gr_library_init"

///////////////////////////////////////////////////////////////////////
//
// gr_server_t
//

#define GR_SERVER_MAGIC         "GS"
#define GR_SERVER_VERSION       1
#define GR_SERVER_LOW_VERSION   1

// listen port count limit
#define GR_PORT_MAX             16

struct gr_server_t
{
    // 接口标记。鬼知道会不会有一天用到。GR_SERVER_MAGIC
    char            magic[ 2 ];

    // 当前接口版本号。只有本结构有修改时才升级版本号。GR_SERVER_VERSION
    unsigned char   version;

    // 当前接口实现可兼容的最低版本号，调用方必须大于等于该版本号才能使用。GR_SERVER_LOW_VERSION
    unsigned char   low_version;

    // worker process/thread count
    int             worker_count;

    // program parameters
    char**          argv;
    int             argc;

    // listen port info
    int             ports_count;
    gr_port_item_t  ports[ GR_PORT_MAX ];

    // 服务器函数库
    gr_library_t *  library;

    // 保留
    char            reserved[ 256 ];
};

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GHOST_ROCKET_INCLUDE_GROCKET_H_
