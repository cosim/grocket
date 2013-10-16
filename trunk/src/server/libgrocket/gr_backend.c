/**
 * @file libgrocket/gr_backend.c
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   
 * Revision History
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-05    Created.
 **/
/* 
 *
 * Copyright (C) 2013-now da_ming at hotmail.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "gr_backend.h"
#include "gr_thread.h"
#include "gr_log.h"
#include "gr_global.h"
#include "gr_errno.h"
#include "gr_tools.h"
#include "gr_mem.h"
#include "gr_config.h"

typedef struct
{
    gr_threads_t    threads;

} gr_backend_t;

void backend_thread( gr_thread_t * thread )
{
    gr_backend_t * self = (gr_backend_t *)thread->param;

    while ( ! thread->is_need_exit ) {
        sleep_ms( 1000 );
    };
}

int gr_backend_init()
{
    gr_backend_t *  p;
    int thread_count = gr_config_backend_thread_count();
    int r;

    if ( thread_count < 1 ) {
        gr_fatal( "[init]gr_worker_init thread_count invalid" );
        return GR_ERR_INVALID_PARAMS;
    }

    if ( NULL != g_ghost_rocket_global.backend ) {
        gr_fatal( "[init]gr_backend_init already called" );
        return GR_ERR_WRONG_CALL_ORDER;
    }

    p = (gr_backend_t *)gr_calloc( 1, sizeof( gr_backend_t ) );
    if ( NULL == p ) {
        gr_fatal( "[init]malloc %d bytes failed, errno=%d,%s",
            (int)sizeof(gr_backend_t), errno, strerror( errno ) );
        return GR_ERR_BAD_ALLOC;
    }

    r = GR_ERR_UNKNOWN;

    do {

        /* r = gr_threads_start(
            & p->threads,
            thread_count,
            NULL,
            backend_thread,
            p,
            0,
            true,
            "backend" );
        if ( GR_OK != r ) {
            break;
        } */

        r = GR_OK;
    } while ( false );

    if ( GR_OK != r ) {
        gr_free( p );
        return r;
    }

    g_ghost_rocket_global.backend = p;
    return GR_OK;
}

void gr_backend_term()
{
    if ( NULL != g_ghost_rocket_global.backend ) {

        gr_backend_t *  p = g_ghost_rocket_global.backend;

        gr_threads_close( & p->threads );

        gr_free( p );
        g_ghost_rocket_global.backend = NULL;
    }
}
