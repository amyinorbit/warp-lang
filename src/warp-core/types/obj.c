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
#include "../warp_internal.h"
#include <stdio.h>
#include <string.h>

void obj_destroy(warp_vm_t *vm, warp_obj_t *obj) {
    switch(obj->kind) {
    case WARP_OBJ_STR:
        warp_str_free(vm, (warp_str_t *)obj);
        break;
    case WARP_OBJ_MAP:
        warp_map_free(vm, (warp_map_t *)obj);
        break;
    }
}

warp_obj_t *alloc_obj(warp_vm_t *vm, size_t size, warp_obj_kind_t kind) {
    warp_obj_t *obj = warp_alloc(vm, NULL, 0, size);
    obj->kind = kind;
    return obj;
}

void init_obj(warp_vm_t *vm, warp_obj_t *obj, warp_obj_kind_t kind) {
    (void)vm;
    obj->kind = kind;
    obj->next = vm->objects;
    vm->objects = obj;
}

void print_obj(warp_vm_t *vm, warp_value_t val) {
    (void)vm;
    
    switch(WARP_OBJ_KIND(val)) {
    case WARP_OBJ_STR:
        printf("%s", WARP_AS_CSTR(val));
        break;
    case WARP_OBJ_MAP:
        printf("<map %p>", (void *)WARP_AS_OBJ(val));
        break;
    }
}

bool obj_equals(const warp_obj_t *a, const warp_obj_t *b) {
    if(a == b) return true;
    if(a->kind != b->kind) return false;
    switch(a->kind) {
    case WARP_OBJ_STR:
        return ((warp_str_t *)a)->length == ((warp_str_t *)b)->length
            && memcmp(((warp_str_t *)a)->data, ((warp_str_t *)b)->data, ((warp_str_t *)a)->length) == 0;
    default:
        break;
    }
    return false;
}
