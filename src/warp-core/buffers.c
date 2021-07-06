//===--------------------------------------------------------------------------------------------===
// buffers.c - Implementation of generic buffers
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#include "buffers.h"

DEFINE_BUFFER(u8, uint8_t)
DEFINE_BUFFER(i32, int32_t)
DEFINE_BUFFER(f32, float)
DEFINE_BUFFER(val, warp_value_t)
