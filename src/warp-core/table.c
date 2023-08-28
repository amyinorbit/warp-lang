/*===--------------------------------------------------------------------------------------------===
 * table.c
 *
 * Created by Amy Parent <amy@amyparent.com>
 * Copyright (c) 2023 Amy Parent. All rights reserved
 *
 * Licensed under the MIT License
 *===--------------------------------------------------------------------------------------------===
*/
#include "table.h"
#include "memory.h"

/*
    int             load;
    int             count;
    int             capacity;
    entry_t         *entries;
*/

void table_init(table_t *table) {
    table->load = 0;
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void table_fini(warp_vm_t *vm, table_t *table) {
    FREE_ARRAY(vm, table->entries, entry_t, table->capacity);
    table_init(table);
}
