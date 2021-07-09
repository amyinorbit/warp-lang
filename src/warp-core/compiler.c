//===--------------------------------------------------------------------------------------------===
// compiler.c
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#include <warp/instr.h>
#include "compiler.h"
#include "scanner.h"
#include "diag.h"
#include "debug.h"
#include <stdarg.h>

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
} precedence_t;

typedef struct {
    token_t current;
    token_t previous;
    bool had_error;
    bool panic;
} parser_t;

typedef struct {
    warp_vm_t *vm;
    chunk_t *chunk;
    scanner_t scanner;
    parser_t parser;
} compiler_t;

typedef void (*parse_fn_t)(compiler_t *comp);

typedef struct {
    parse_fn_t prefix;
    parse_fn_t infix;
    precedence_t precedence;
} parse_rule_t;

static void
error_at_varg(compiler_t *comp, const token_t *token, const char *fmt, va_list args) {
    if(comp->parser.panic) return;
    emit_diag_varg(&comp->scanner.source, WARP_DIAG_ERROR, token, fmt, args);
    comp->parser.panic = true;
    comp->parser.had_error = true;
}

static inline token_t *previous(compiler_t *comp) { return &comp->parser.previous; }
static inline token_t *current(compiler_t *comp) { return &comp->parser.current; }
static inline chunk_t *current_chunk(compiler_t *comp) { return comp->chunk; }

static void error_at(compiler_t *comp, const token_t *token, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    error_at_varg(comp, token, fmt, args);
    va_end(args);
}

static void error(compiler_t *comp, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    error_at_varg(comp, previous(comp), fmt, args);
    va_end(args);
}

static void error_current(compiler_t *comp, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    error_at_varg(comp, current(comp), fmt, args);
    va_end(args);
}

static void advance(compiler_t *comp) {
    *previous(comp) = *current(comp);
    for(;;) {
        *current(comp) = scan_token(&comp->scanner);
        if(current(comp)->kind != TOK_INVALID) break;
        error_current(comp, "invalid character");
    }
}

static void consume(compiler_t *comp, token_kind_t kind, const char *msg) {
    if(current(comp)->kind == kind) {
        advance(comp);
        return;
    }
    error_current(comp, msg);
}

static void emit_byte(compiler_t *comp, uint8_t byte) {
    chunk_write(comp->vm, current_chunk(comp), byte, previous(comp)->line);
}

static void emit_bytes(compiler_t *comp, uint8_t byte1, uint8_t byte2) {
    emit_byte(comp, byte1);
    emit_byte(comp, byte2);
}

static void emit_bytes_long(compiler_t *comp, uint8_t byte1, uint16_t operand) {
    emit_byte(comp, byte1);
    emit_byte(comp, operand & 0x00ff);
    emit_byte(comp, operand >> 8);
}

static void emit_return(compiler_t *comp) {
    emit_byte(comp, OP_RETURN);
}

static void emit_const(compiler_t *comp, warp_value_t value) {
    int idx = chunk_add_const(comp->vm, current_chunk(comp), value);
    if(idx < UINT8_MAX) {
        emit_bytes(comp, OP_CONST, (uint8_t)idx);
    } else if(idx < UINT16_MAX) {
        emit_bytes_long(comp, OP_CONST, (uint16_t)idx);
    } else {
        error(comp, "too many constants in one bytecode unit");
    }
}

static void end_compiler(compiler_t *comp) {
    emit_return(comp);
}

static void expression(compiler_t *comp);
static const parse_rule_t *get_rule(token_kind_t kind);
static void parse_precedence(compiler_t *comp, precedence_t prec);

static void number(compiler_t *comp) {
    double val = strtod(previous(comp)->start, NULL);
    emit_const(comp, WARP_NUMBER_VAL(val));
}

static void literal(compiler_t *comp) {
    token_t tok = *previous(comp);
    switch(tok.kind) {
        case TOK_TRUE: emit_byte(comp, OP_TRUE); break;
        case TOK_FALSE: emit_byte(comp, OP_FALSE); break;
        case TOK_NIL: emit_byte(comp, OP_NIL); break;
        default: UNREACHABLE(); break;
    }
}

static void expression(compiler_t *comp) {
    parse_precedence(comp, PREC_ASSIGNMENT);
}

static void unary(compiler_t *comp) {
    token_t op = *previous(comp);
    
    expression(comp);    
    switch(op.kind) {
    case TOK_MINUS: emit_byte(comp, OP_NEG); break;
    case TOK_BANG: emit_byte(comp, OP_NOT); break;
    default: UNREACHABLE(); break;
    }
}

static void binary(compiler_t *comp) {
    token_t op = *previous(comp);
    const parse_rule_t *rule = get_rule(op.kind);
    parse_precedence(comp, rule->precedence + 1);
    
    switch(op.kind) {
    case TOK_PLUS: emit_byte(comp, OP_ADD); break;
    case TOK_MINUS: emit_byte(comp, OP_SUB); break;
    case TOK_STAR: emit_byte(comp, OP_MUL); break;
    case TOK_SLASH: emit_byte(comp, OP_DIV); break;
    
    case TOK_LT: emit_byte(comp, OP_LT); break;
    case TOK_GT: emit_byte(comp, OP_GT); break;
    case TOK_LTEQ: emit_byte(comp, OP_LTEQ); break;
    case TOK_GTEQ: emit_byte(comp, OP_GTEQ); break;
    case TOK_EQEQ: emit_byte(comp, OP_EQ); break;
    case TOK_BANGEQ: emit_bytes(comp, OP_EQ, OP_NOT); break;
    
    default: UNREACHABLE(); return;
    }
}

static void grouping(compiler_t *comp) {
    expression(comp);
    consume(comp, TOK_RPAREN, "missing ')' after expression");
}

const parse_rule_t rules[] = {
    [TOK_LPAREN] =      {grouping,  NULL,       PREC_NONE},
    [TOK_RPAREN] =      {NULL,      NULL,       PREC_NONE},
    [TOK_LBRACE] =      {NULL,      NULL,       PREC_NONE},
    [TOK_RBRACE] =      {NULL,      NULL,       PREC_NONE},
    [TOK_LBRACKET] =    {NULL,      NULL,       PREC_NONE},
    [TOK_RBRACKET] =    {NULL,      NULL,       PREC_NONE},
    [TOK_PLUS] =        {NULL,      binary,     PREC_TERM},
    [TOK_MINUS] =       {unary,     binary,     PREC_TERM},
    [TOK_SLASH] =       {NULL,      binary,     PREC_FACTOR},
    [TOK_STAR] =        {NULL,      binary,     PREC_FACTOR},
    [TOK_STARSTAR] =    {NULL,      NULL,       PREC_NONE},
    [TOK_PERCENT] =     {NULL,      NULL,       PREC_NONE},
    [TOK_CARET] =       {NULL,      NULL,       PREC_NONE},
    [TOK_TILDE] =       {NULL,      NULL,       PREC_NONE},
    [TOK_AMP] =         {NULL,      NULL,       PREC_NONE},
    [TOK_PIPE] =        {NULL,      NULL,       PREC_NONE},
    [TOK_BANG] =        {unary,     NULL,       PREC_UNARY},
    [TOK_QUESTION] =    {NULL,      NULL,       PREC_NONE},
    [TOK_LT] =          {NULL,      binary,     PREC_COMPARISON},
    [TOK_GT] =          {NULL,      binary,     PREC_COMPARISON},
    [TOK_EQUALS] =      {NULL,      NULL,       PREC_NONE},
    [TOK_LTEQ] =        {NULL,      binary,     PREC_COMPARISON},
    [TOK_GTEQ] =        {NULL,      binary,     PREC_COMPARISON},
    [TOK_EQEQ] =        {NULL,      binary,     PREC_EQUALITY},
    [TOK_PLUSEQ] =      {NULL,      NULL,       PREC_NONE},
    [TOK_MINUSEQ] =     {NULL,      NULL,       PREC_NONE},
    [TOK_STAREQ] =      {NULL,      NULL,       PREC_NONE},
    [TOK_SLASHEQ] =     {NULL,      NULL,       PREC_NONE},
    [TOK_BANGEQ] =      {NULL,      binary,     PREC_EQUALITY},
    [TOK_LTLT] =        {NULL,      NULL,       PREC_NONE},
    [TOK_GTGT] =        {NULL,      NULL,       PREC_NONE},
    [TOK_GTGTEQ] =      {NULL,      NULL,       PREC_NONE},
    [TOK_AMPAMP] =      {NULL,      NULL,       PREC_NONE},
    [TOK_PIPEPIPE] =    {NULL,      NULL,       PREC_NONE},
    [TOK_SEMICOLON] =   {NULL,      NULL,       PREC_NONE},
    [TOK_NEWLINE] =     {NULL,      NULL,       PREC_NONE},
    [TOK_COLON] =       {NULL,      NULL,       PREC_NONE},
    [TOK_COMMA] =       {NULL,      NULL,       PREC_NONE},
    [TOK_DOT] =         {NULL,      NULL,       PREC_NONE},
    [TOK_ARROW] =       {NULL,      NULL,       PREC_NONE},
    [TOK_THEN] =        {NULL,      NULL,       PREC_NONE},
    [TOK_NUMBER] =      {number,    NULL,       PREC_NONE},
    [TOK_STRING] =      {NULL,      NULL,       PREC_NONE},
    [TOK_IDENTIFIER] =  {NULL,      NULL,       PREC_NONE},
    [TOK_SELF] =        {NULL,      NULL,       PREC_NONE},
    [TOK_TRUE] =        {literal,   NULL,       PREC_NONE},
    [TOK_FALSE] =       {literal,   NULL,       PREC_NONE},
    [TOK_NIL] =         {literal,   NULL,       PREC_NONE},
    [TOK_FUN] =         {NULL,      NULL,       PREC_NONE},
    [TOK_VAR] =         {NULL,      NULL,       PREC_NONE},
    [TOK_LET] =         {NULL,      NULL,       PREC_NONE},
    [TOK_RETURN] =      {NULL,      NULL,       PREC_NONE},
    [TOK_FOR] =         {NULL,      NULL,       PREC_NONE},
    [TOK_WHILE] =       {NULL,      NULL,       PREC_NONE},
    [TOK_BREAK] =       {NULL,      NULL,       PREC_NONE},
    [TOK_CONTINUE] =    {NULL,      NULL,       PREC_NONE},
    [TOK_IF] =          {NULL,      NULL,       PREC_NONE},
    [TOK_ELSE] =        {NULL,      NULL,       PREC_NONE},
    [TOK_INIT] =        {NULL,      NULL,       PREC_NONE},
    [TOK_PRINT] =       {NULL,      NULL,       PREC_NONE},
    [TOK_EOF] =         {NULL,      NULL,       PREC_NONE},
    [TOK_INVALID] =     {NULL,      NULL,       PREC_NONE},
};

static void parse_precedence(compiler_t *comp, precedence_t prec) {
    advance(comp);
    parse_fn_t prefix = get_rule(previous(comp)->kind)->prefix;
    if(!prefix) {
        error(comp, "expected an expression");
        return;
    }
    prefix(comp);
    
    while(prec <= get_rule(current(comp)->kind)->precedence) {
        advance(comp);
        parse_fn_t infix = get_rule(previous(comp)->kind)->infix;
        infix(comp);
    }
}

static const parse_rule_t *get_rule(token_kind_t kind) {
    return &rules[kind];
}


bool compile(warp_vm_t *vm, chunk_t *chunk, const char *src, size_t length) {
    compiler_t comp;
    
    UNUSED(error_at);
    
    scanner_init_text(&comp.scanner, src, length);
    comp.vm = vm;
    comp.parser.panic = false;
    comp.parser.had_error = false;
    comp.chunk = chunk;
    
    advance(&comp);
    expression(&comp);
    end_compiler(&comp);
    
    consume(&comp, TOK_EOF, "expected end of expression");
    
#ifdef DEBUG_PRINT_CODE
    if(!comp.parser.had_error) {
        disassemble_chunk(chunk, "compiled code", stdout);
    }
#endif
    
    return !comp.parser.had_error;
}

