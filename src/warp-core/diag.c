//===--------------------------------------------------------------------------------------------===
// diag.c
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#include "diag.h"
#include <term/colors.h>


static const char *diag_level_str(warp_diag_level_t level) {
    switch(level) {
    case WARP_DIAG_INFO: return "info";
    case WARP_DIAG_WARN: return "warning";
    case WARP_DIAG_ERROR: return "error";
    }
    UNREACHABLE();
}

static term_color_t diag_level_color(warp_diag_level_t level) {
    switch(level) {
    case WARP_DIAG_INFO: return TERM_DEFAULT;
    case WARP_DIAG_WARN: return TERM_YELLOW;
    case WARP_DIAG_ERROR: return TERM_RED;
    }
    UNREACHABLE();
}

void warp_print_diag(const warp_diag_t *diag, void *user_info) {
    UNUSED(user_info);
    
    term_set_bold(stderr, true);
    term_set_fg(stderr, diag_level_color(diag->level));
    fprintf(stderr, "%s", diag_level_str(diag->level));
    
    term_style_reset(stderr);
    term_set_bold(stderr, true);
    fprintf(stderr, " in %s @ %d:%d:\n\n", diag->fname, diag->span.line, diag->span.column);
    term_style_reset(stderr);
    fprintf(stderr, "%.*s\n",
            (int)(diag->line_end - diag->line_start),
            diag->line_start);
            
    for(int i = 1; i < diag->span.column; ++i) {
        fputc(' ', stderr);
    }
    term_set_fg(stderr, TERM_GREEN);
    for(int i = 0; i < diag->span.length; ++i) {
        fputc(i ? '~' : '^', stderr);
    }
    term_style_reset(stderr);
    fputc('\n', stderr);
    
    term_set_fg(stderr, TERM_MAGENTA);
    fprintf(stderr, "%s\n", diag->message);
    term_style_reset(stderr);
    fputc('\n', stderr);
}

const char *get_line_start(const src_t *src, const token_t *token) {
    CHECK(token->start >= src->start && token->start < src->end);
    const char *ptr = token->start;
    while(ptr > src->start && ptr[-1] != '\n') {
        ptr--;
    }
    return ptr;
}

const char *get_line_end(const src_t *src, const token_t *token) {
    CHECK(token->start >= src->start && token->start < src->end);
    const char *ptr = token->start;
    while(ptr < src->end && *ptr != '\n' && *ptr != '\r') {
        ptr++;
    }
    return ptr;
}

void emit_diag_varg(
    const src_t *src,
    warp_diag_level_t level,
    const token_t *token,
    const char *fmt,
    va_list args
) {
    va_list copy;
    va_copy(copy, args);
    size_t size = vsnprintf(NULL, 0, fmt, copy) + 1;
    
    warp_diag_t diag;
    char *message = malloc(size);
    vsnprintf(message, size, fmt, args);
    
    diag.level = level;
    diag.message = message;
    diag.line_start = get_line_start(src, token);
    diag.line_end = get_line_end(src, token);
    
    diag.span.length = token->length;
    diag.span.line = token->line;
    diag.span.column = 1 + (int)(token->start - diag.line_start);
    diag.fname = src->fname;
    
    // TODO: call user function if present
    warp_print_diag(&diag, NULL);
    
    free(message);
}
