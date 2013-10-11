/**
 * @file libgrocket/server_object.h
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
#ifndef _server_object_h_
#define _server_object_h_

#include "gr_stdinc.h"
#include "grocket.h"

// ����������

GR_CLASS_DECLARE_BEGIN( server, gr_i_server_t )

    // class ����

GR_CLASS_DECLARE_END( server )

bool server_class_construct( server_class_t * sc );

#endif // #ifndef _server_object_h_
