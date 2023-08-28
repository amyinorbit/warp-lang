//===--------------------------------------------------------------------------------------------===
// value.h - Private API for the Warp value system
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#include <warp/value.h>

bool value_is_falsey(warp_value_t a);
bool value_equals(warp_value_t a, warp_value_t b);
