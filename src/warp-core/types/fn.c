/*===--------------------------------------------------------------------------------------------===
 * fn.c
 *
 * Created by Amy Parent <amy@amyparent.com>
 * Copyright (c) 2023 Amy Parent. All rights reserved
 *
 * Licensed under the MIT License
 *===--------------------------------------------------------------------------------------------===
*/
#include "obj_impl.h"

warp_fn_t *warp_fn_new(warp_vm_t *vm, warp_fn_kind_t kind) {
    warp_fn_t *fn = ALLOCATE_OBJ(vm, warp_fn_t, WARP_OBJ_FN);
    fn->name = NULL;
    fn->arity = 0;
    chunk_init(vm, &fn->chunk);
    UNUSED(kind);
    return fn;
}

void warp_fn_free(warp_vm_t *vm, warp_fn_t *fn) {
    chunk_fini(vm, &fn->chunk);
    FREE(vm, fn, warp_fn_t);
}
