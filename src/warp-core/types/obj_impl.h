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
#include "../memory.h"

#define ALLOCATE_OBJ(vm, T, tag) \
    (T *)alloc_obj(vm, sizeof(T), tag);

warp_obj_t *alloc_obj(warp_vm_t *vm, size_t size, warp_obj_kind_t kind);
void init_obj(warp_vm_t *vm, warp_obj_t *obj, warp_obj_kind_t kind);

// MARK: - String interface

struct warp_str_t {
    warp_obj_t  obj;
    int         length;
    char        data[];
};

#endif /* ifndef _OBJ_IMPL_H_ */
