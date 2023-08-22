/*===--------------------------------------------------------------------------------------------===
 * obj.c
 *
 * Created by Amy Parent <amy@amyparent.com>
 * Copyright (c) 2023 Amy Parent. All rights reserved
 *
 * Licensed under the MIT License
 *===--------------------------------------------------------------------------------------------===
*/
#include "obj_impl.h"
#include "../memory.h"
#include <stdio.h>

warp_obj_t *alloc_obj(warp_vm_t *vm, size_t size, warp_obj_kind_t kind) {
    warp_obj_t *obj = warp_alloc(vm, NULL, 0, size);
    obj->kind = kind;
    return obj;
}

void init_obj(warp_vm_t *vm, warp_obj_t *obj, warp_obj_kind_t kind) {
    (void)vm;
    obj->kind = kind;
}

void print_obj(warp_vm_t *vm, warp_value_t val) {
    (void)vm;
    
    switch(WARP_OBJ_KIND(val)) {
        case WARP_OBJ_STR:
        printf("%s", WARP_AS_CSTR(val));
        break;
    }
}
