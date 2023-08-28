/*===--------------------------------------------------------------------------------------------===
 * list.c
 *
 * Created by Amy Parent <amy@amyparent.com>
 * Copyright (c) 2023 Amy Parent. All rights reserved
 *
 * Licensed under the MIT License
 *===--------------------------------------------------------------------------------------------===
*/
#include <warp/obj.h>
#include "../memory.h"

struct warp_list_t {
    warp_obj_t      obj;
    val_buf_t       buf;
};


