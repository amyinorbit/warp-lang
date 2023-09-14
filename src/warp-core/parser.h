//===--------------------------------------------------------------------------------------------===
// parser.h
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#pragma once
#include <warp/common.h>
#include <warp/value.h>
#include <unic/unic.h>
#include <stdarg.h>

#define DEBUG_LEX 0


typedef enum {
    // Bracket types
    // (){}[]
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LBRACKET,
    TOK_RBRACKET,
    
    // Operator Tokens
    // + - / * % ^ & | < > = >= <= == += ~ >> << && || != ! ?
    TOK_PLUS,
    TOK_MINUS,
    TOK_SLASH,
    TOK_STAR,
    TOK_STARSTAR,
    TOK_PERCENT,
    TOK_CARET,
    TOK_TILDE,
    TOK_AMP,
    TOK_PIPE,
    TOK_BANG,
    TOK_QUESTION,
    TOK_LT,
    TOK_GT,
    TOK_EQUALS,
    TOK_LTEQ,
    TOK_GTEQ,
    TOK_EQEQ,
    TOK_PLUSEQ,
    TOK_MINUSEQ,
    TOK_STAREQ,
    TOK_SLASHEQ,
    TOK_BANGEQ,
    TOK_LTLT,
    TOK_GTGT,
    TOK_GTGTEQ,
    TOK_AMPAMP,
    TOK_PIPEPIPE,
    
    TOK_SEMICOLON,
    TOK_NEWLINE,
    
    // Punctuation:
    // :,.
    TOK_COLON,
    TOK_COMMA,
    TOK_DOT,
    TOK_ARROW,
    
    // Language objects
    TOK_NUMBER,
    TOK_STRING,
    TOK_IDENTIFIER,
    
    // Language keywords
    TOK_SELF,
    TOK_TRUE,
    TOK_FALSE,
    TOK_NIL,
    TOK_FN,
    TOK_VAR,
    TOK_LET,
    TOK_RETURN,
    TOK_FOR,
    TOK_WHILE,
    TOK_BREAK,
    TOK_CONTINUE,
    TOK_IF,
    TOK_THEN,
    TOK_ELSE,
    TOK_END,
    TOK_INIT,
    TOK_PRINT, // TODO: remove when FFI/native functions allow print() to be impl.
    
    TOK_EOF,
    TOK_INVALID,
} token_kind_t;

typedef struct {
    const char *fname;
    const char *start;
    const char *end;
} src_t;

typedef struct token_s {
    token_kind_t        kind;
    int                 length;
    int                 line;
    bool                start_of_line;
    const char          *start;
    warp_value_t        value;
} token_t;

typedef struct parser_s {
    warp_vm_t           *vm;
    
    // Scanner info
    src_t               source;
    const char          *start;
    const char          *current;
    int                 line;
    unicode_scalar_t    copy;
    bool                start_of_line;
    
    // Parser/RD info
    token_t             current_token;
    token_t             previous_token;
    bool                had_error;
    bool                panic;
} parser_t;

void parser_init(parser_t *parser, warp_vm_t *vm, const char *fname, const char *text, size_t length);
token_t scan_token(parser_t *parser);
const char *token_name(token_kind_t kind);

void error_silent(parser_t *parser);
void error_at(parser_t *parser, const token_t *token, const char *fmt, ...);
void error_at_varg(parser_t *parser, const token_t *token, const char *fmt, va_list args);

token_t *previous(parser_t *parser);
token_t *current(parser_t *parser);

void synchronize(parser_t *parser);

bool check(parser_t *parser, token_kind_t kind);
bool check_terminator(parser_t *parser);
bool match(parser_t *parser, token_kind_t kind);

void consume_terminator(parser_t *parser, const char *msg);
void advance(parser_t *parser);
void consume(parser_t *parser, token_kind_t kind, const char *msg);
