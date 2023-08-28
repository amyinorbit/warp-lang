/*===--------------------------------------------------------------------------------------------===
 * table.h
 *
 * Created by Amy Parent <amy@amyparent.com>
 * Copyright (c) 2023 Amy Parent
 *
 * Licensed under the MIT License
 *===--------------------------------------------------------------------------------------------===
*/
#ifndef _WARP_TABLE_H_
#define _WARP_TABLE_H_

#include <warp/common.h>
#include <warp/obj.h>
#include <warp/value.h>

typedef struct {
    warp_str_t      *key;
    warp_value_t    value;
} entry_t;

typedef struct table_t {
    int             load;
    int             count;
    int             capacity;
    entry_t         *entries;
} table_t;


void table_init(table_t *table);
void table_fini(warp_vm_t *vm, table_t *table);


#endif /* ifndef _WARP_TABLE_H_ */
