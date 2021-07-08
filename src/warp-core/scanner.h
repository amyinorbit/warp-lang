//===--------------------------------------------------------------------------------------------===
// scanner.h
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#pragma once
#include <warp/common.h>
#include <unic/unic.h>

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
    TOK_THEN,
    
    // Language objects
    TOK_NUMBER,
    TOK_STRING,
    TOK_IDENTIFIER,
    
    // Language keywords
    TOK_SELF,
    TOK_TRUE,
    TOK_FALSE,
    TOK_NIL,
    TOK_FUN,
    TOK_VAR,
    TOK_LET,
    TOK_RETURN,
    TOK_FOR,
    TOK_WHILE,
    TOK_BREAK,
    TOK_CONTINUE,
    TOK_IF,
    TOK_ELSE,
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

typedef struct scanner_s {
    src_t source;
    const char *start;
    const char *current;
    int line;
    unicode_scalar_t copy;
} scanner_t;

typedef struct token_s {
    token_kind_t kind;
    int length;
    int line;
    const char *start;
} token_t;

void scanner_init_text(scanner_t *scanner, const char *text, size_t length);
token_t scan_token(scanner_t *scanner);
const char *token_name(token_kind_t kind);
