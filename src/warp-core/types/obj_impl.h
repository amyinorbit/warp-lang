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
#include "../memory.h"

#define ALLOCATE_OBJ(vm, T, tag) \
    (T *)alloc_obj(vm, sizeof(T), tag);

warp_obj_t *alloc_obj(warp_vm_t *vm, size_t size, warp_obj_kind_t kind);
void init_obj(warp_vm_t *vm, warp_obj_t *obj, warp_obj_kind_t kind);
bool obj_equals(const warp_obj_t *a, const warp_obj_t *b);
void obj_destroy(warp_vm_t *vm, warp_obj_t *obj);

// MARK: - String interface

struct warp_str_t {
    warp_obj_t  obj;
    uint32_t    hash;
    int         length;
    char        data[];
};

warp_str_t *alloc_str(warp_vm_t *vm, int length);



#endif /* ifndef _OBJ_IMPL_H_ */
