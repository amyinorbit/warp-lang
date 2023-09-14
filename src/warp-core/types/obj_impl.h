/*===--------------------------------------------------------------------------------------------===
 * obj_impl.h
 *
 * Created by Amy Parent <amy@amyparent.com>
 * Copyright (c) 2023 Amy Parent
 *
 * Licensed under the MIT License
 *===--------------------------------------------------------------------------------------------===
*/
#ifndef _OBJ_IMPL_H_
#define _OBJ_IMPL_H_
#include <warp/obj.h>
#include <stdint.h>
#include <stdio.h>
#include "../chunk.h"
#include "../memory.h"

#define ALLOCATE_OBJ(vm, T, tag) \
    (T *)alloc_obj(vm, sizeof(T), tag);

warp_obj_t *alloc_obj(warp_vm_t *vm, size_t size, warp_obj_kind_t kind);
void init_obj(warp_vm_t *vm, warp_obj_t *obj, warp_obj_kind_t kind);
bool obj_equals(const warp_obj_t *a, const warp_obj_t *b);
void obj_destroy(warp_vm_t *vm, warp_obj_t *obj);
void obj_print(warp_value_t val, FILE *out);

// MARK: - String interface

struct warp_str_t {
    warp_obj_t  obj;
    uint32_t    hash;
    warp_uint_t length;
    char        data[];
};

warp_str_t *alloc_str(warp_vm_t *vm, int length);
void warp_str_free(warp_vm_t *vm, warp_str_t *str);

// MARK: - Table interface

typedef struct entry_t {
    warp_value_t    key;
    warp_value_t    value;
} entry_t;

struct warp_map_t {
    warp_obj_t      obj;
    warp_uint_t     capacity;
    warp_uint_t     load;
    warp_uint_t     count;
    entry_t         *entries;
};

warp_str_t *warp_map_find_str(warp_map_t *map, const char *str, warp_uint_t length, uint32_t hash);
void warp_map_free(warp_vm_t *vm, warp_map_t *map);

// MARK: Func Interface

struct warp_fn_t {
    warp_obj_t      obj;
    warp_str_t      *name;
    uint8_t         arity;
    chunk_t         chunk;
};

struct warp_native_t {
    warp_obj_t      obj;
    warp_str_t      *name;
    uint8_t         arity;
    warp_native_f   native;
};

void warp_fn_free(warp_vm_t *vm, warp_fn_t *fn);

warp_native_t *warp_native_new(warp_vm_t *vm, const char *name, uint8_t arity, warp_native_f native);
void warp_native_free(warp_vm_t *vm, warp_native_t *fn);

#endif /* ifndef _OBJ_IMPL_H_ */
