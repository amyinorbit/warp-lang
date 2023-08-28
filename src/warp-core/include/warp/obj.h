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
    // WARP_OBJ_LIST,
    // WARP_OBJ_TABLE,
} warp_obj_kind_t;

struct warp_obj_t {
    warp_obj_kind_t kind;
    warp_obj_t      *next;
};

static inline bool warp_is_obj_kind(warp_value_t val, warp_obj_kind_t kind) {
  return WARP_IS_OBJ(val) && WARP_AS_OBJ(val)->kind == kind;
}

#define WARP_OBJ_KIND(value)    (WARP_AS_OBJ(value)->kind)
#define WARP_IS_STR(value)      (warp_is_obj_kind(value, WARP_OBJ_STR))
#define WARP_AS_STR(value)      ((warp_str_t *)WARP_AS_OBJ(value))
#define WARP_AS_CSTR(value)     (warp_str_get_c(WARP_AS_STR(value)))

#define WARP_AS_LIST(value)     ((warp_list_t *)WARP_AS_OBJ(value))

// MARK: - String interface

warp_str_t *warp_copy_c_str(warp_vm_t *vm, const char *c_str, int length);
warp_str_t *warp_concat_str(warp_vm_t *vm, const warp_str_t *a, const warp_str_t *b);
int warp_str_get_length(const warp_str_t *str);
const char *warp_str_get_c(const warp_str_t *str);

// MARK: - List interface

warp_list_t *warp_list_new(warp_vm_t *vm);
int warp_list_get_size(warp_list_t *list);
warp_value_t warp_list_get(warp_vm_t *vm, int idx);
void warp_list_insert(warp_vm_t *vm, warp_list_t *list, int idx, warp_value_t val);
void warp_list_set(warp_vm_t *vm, warp_list_t *list, int idx, warp_value_t val);

// MARK: - Table interface

warp_table_t *warp_table_new(warp_vm_t *vm);


#endif /* ifndef _WARP_OBJ_H_ */
