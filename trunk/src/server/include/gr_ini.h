/**
 * @file include/gr_ini.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/10/05
 * @version $Revision$ 
 * @brief   INI文件操作
 * Revision History 大事件记
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+
 *       1     zouyueming   2013-10-05    Created.
 **/
#ifndef _GHOST_ROCKET_SERVER_LIBGROCKET_GR_INI_H_
#define _GHOST_ROCKET_SERVER_LIBGROCKET_GR_INI_H_

#include "gr_stdinc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INI_SECTION_COUNT   32
#define INI_ITEM_COUNT      64

typedef struct
{
    const char *        key;
    const char *        val;
} gr_ini_item;

typedef struct
{
    const char *        name;
    gr_ini_item         items[ INI_ITEM_COUNT ];
    size_t              items_count;
} gr_ini_section;

typedef struct
{
    char *              buf;
    size_t              len;
    gr_ini_section      sections[ INI_SECTION_COUNT ];
    size_t              sections_count;
} gr_ini;

int
gr_ini_open(
    gr_ini * ini,
    const char * path
);

int
gr_ini_open_memory(
    gr_ini * ini,
    const char * content,
    size_t content_len
);

void
gr_ini_close(
    gr_ini * This
);

size_t
gr_ini_get_sections_count(
    gr_ini * ini
);

bool
gr_ini_get_sections(
   gr_ini * ini,
   const char ** sections,
   size_t * sections_count
);

bool
gr_ini_get_bool(
    gr_ini * ini,
    const char * section,
    const char * name,
    bool def
);

int
gr_ini_get_int(
    gr_ini * ini,
    const char * section,
    const char * name,
    int def
);

const char *
gr_ini_get_string(
    gr_ini * This,
    const char * section,
    const char * name,
    const char * def
);

#ifdef __cplusplus
}
#endif

#endif
