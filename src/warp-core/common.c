//===--------------------------------------------------------------------------------------------===
// common.c - Implementation of basic utilities for Warp
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#include <warp/common.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static void default_log_fn(const char *msg) {
    fprintf(stderr, "warp: %s\n", msg);
}

static void (*print_fn)(const char*) = default_log_fn;

void warp_log_init(void (*new_print_fn)(const char*)) {
    CHECK(print_fn);
    print_fn = new_print_fn;
}

#define PREAMBLE_FMT "warp (%s:%d): ", short_fname, line

void warp_log(const char *filename, int line, const char *fmt, ...) {
    if(!filename || !fmt) abort();
    
    const char *short_fname = strrchr(filename, PATH_SEP);
    if(!short_fname) {
        short_fname = filename;
    } else {
        short_fname++;
    }
    
    va_list args, args_copy;
    va_start(args, fmt);
    va_copy(args_copy, args);
    
    size_t preamble_len = snprintf(NULL, 0, PREAMBLE_FMT);
    size_t msg_len = vsnprintf(NULL, 0, fmt, args_copy);
    
    char *buffer = malloc(preamble_len + msg_len + 2);
    if(!buffer) abort(); // At this stage, if we can't allocate, we're 10 miles past toast already
    
    (void)snprintf(buffer, preamble_len + 1, PREAMBLE_FMT);
    (void)vsnprintf(buffer + preamble_len, msg_len + 1, fmt, args);
    buffer[strlen(buffer)] = '\n';
    print_fn(buffer);
    free(buffer);
}
