/**
 * @file libgrocket/server_object.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/09
 * @version $Revision$ 
 * @brief   
 * Revision History 大事件记
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-09    Created.
 **/
#ifndef _server_object_h_
#define _server_object_h_

#include "gr_stdinc.h"
#include "grocket.h"

// 服务器对象

GR_CLASS_DECLARE_BEGIN( server, gr_i_server_t )

    // class 声明

GR_CLASS_DECLARE_END( server )

bool server_class_construct( server_class_t * sc );

#endif // #ifndef _server_object_h_
