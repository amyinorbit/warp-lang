/*===--------------------------------------------------------------------------------------------===
 * str.c
 *
 * Created by Amy Parent <amy@amyparent.com>
 * Copyright (c) 2023 Amy Parent. All rights reserved
 *
 * Licensed under the MIT License
 *===--------------------------------------------------------------------------------------------===
*/
#include "obj_impl.h"
#include "../warp_internal.h"
#include <string.h>


static uint32_t str_hash(const char *chars, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)chars[i];
        hash *= 16777619;
    }
    return hash;
}

warp_str_t *alloc_str(warp_vm_t *vm, int length) {
    warp_str_t *str = ALLOCATE_SARRAY(vm, warp_str_t, length + 1);
    init_obj(vm, (warp_obj_t *)str, WARP_OBJ_STR);
    str->length = length;
    return str;
}

void warp_str_free(warp_vm_t *vm, warp_str_t *str) {
    DEALLOCATE_SARRAY(vm, str, warp_str_t, char, str->length + 1);
}

warp_str_t *warp_copy_c_str(warp_vm_t *vm, const char *c_str, int length) {
    uint32_t hash = str_hash(c_str, length);
    warp_str_t *str = warp_map_find_str(vm->strings, c_str, length, hash);
    if(str != NULL) return str;
    
    str = alloc_str(vm, length);
    memcpy(str->data, c_str, length);
    str->data[length] = '\0';
    str->length = length;
    str->hash = hash;
    warp_map_set(vm, vm->strings, WARP_OBJ_VAL(str), WARP_NIL_VAL);
    return str;
}

warp_str_t *warp_concat_str(warp_vm_t *vm, const warp_str_t *a, const warp_str_t *b) {

    warp_uint_t length = a->length + b->length;
    char *c_str = ALLOCATE_ARRAY(vm, char, length + 1);
    memcpy(c_str, a->data, a->length);
    memcpy(c_str + a->length, b->data, b->length);
    c_str[length] = '\0';
    
    warp_str_t *str = warp_copy_c_str(vm, c_str, length);
    FREE_ARRAY(vm, c_str, char, length+1);
    return str;
}

int warp_str_get_length(const warp_str_t *str) {
    return str->length;
}

const char *warp_str_get_c(const warp_str_t *str) {
    return str->data;
}
