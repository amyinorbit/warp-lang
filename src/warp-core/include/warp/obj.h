/*===--------------------------------------------------------------------------------------------===
 * obj.h
 *
 * Created by Amy Parent <amy@amyparent.com>
 * Copyright (c) 2023 Amy Parent
 *
 * Licensed under the MIT License
 *===--------------------------------------------------------------------------------------------===
*/
#ifndef _WARP_OBJ_H_
#define _WARP_OBJ_H_

#include <warp/common.h>
#include <warp/value.h>
#include <stdint.h>

typedef enum {
    WARP_OBJ_STR,
} warp_obj_kind_t;

struct warp_obj_t {
    warp_obj_kind_t kind;
};

static inline bool warp_is_obj_kind(warp_value_t val, warp_obj_kind_t kind) {
  return WARP_IS_OBJ(val) && WARP_AS_OBJ(val)->kind == kind;
}

#define WARP_OBJ_KIND(value)    (WARP_AS_OBJ(value)->kind)
#define WARP_IS_STR(value)      (warp_is_obj_kind(value, WARP_OBJ_STR))
#define WARP_AS_STR(value)      ((warp_str_t *)WARP_AS_OBJ(value))
#define WARP_AS_CSTR(value)     (warp_str_get_c(WARP_AS_STR(value)))


// MARK: - String interface

warp_str_t *warp_copy_c_str(warp_vm_t *vm, const char *c_str, int length);
int warp_str_get_length(const warp_str_t *str);
const char *warp_str_get_c(const warp_str_t *str);

#endif /* ifndef _WARP_OBJ_H_ */
