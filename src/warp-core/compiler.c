//===--------------------------------------------------------------------------------------------===
// compiler.c
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#include <warp/instr.h>
#include <warp/obj.h>
#include "compiler.h"
#include "parser.h"
#include "types/obj_impl.h"
#include "diag_impl.h"
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
    warp_vm_t   *vm;
    chunk_t     *chunk;
    parser_t    *parser;
} compiler_t;

typedef void (*parse_fn_t)(compiler_t *comp);

typedef struct {
    parse_fn_t prefix;
    parse_fn_t infix;
    precedence_t precedence;
} parse_rule_t;

static chunk_t *current_chunk(compiler_t *comp) {
    return comp->chunk;
}

static void emit_byte(compiler_t *comp, uint8_t byte) {
    chunk_write(comp->vm, current_chunk(comp), byte, previous(comp->parser)->line);
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
        emit_bytes_long(comp, OP_LCONST, (uint16_t)idx);
    } else {
        error_at(comp->parser, previous(comp->parser), "too many constants in one bytecode unit");
    }
}

static void end_compiler(compiler_t *comp) {
    emit_return(comp);
}

static void expression(compiler_t *comp);
static const parse_rule_t *get_rule(token_kind_t kind);
static void parse_precedence(compiler_t *comp, precedence_t prec);

static void number(compiler_t *comp) {
    emit_const(comp, previous(comp->parser)->value);
}

static void string(compiler_t *comp) {
    emit_const(comp, previous(comp->parser)->value);
}

static void literal(compiler_t *comp) {
    token_t tok = *previous(comp->parser);
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
    token_t op = *previous(comp->parser);
    
    expression(comp);    
    switch(op.kind) {
    case TOK_MINUS: emit_byte(comp, OP_NEG); break;
    case TOK_BANG: emit_byte(comp, OP_NOT); break;
    default: UNREACHABLE(); break;
    }
}

static void binary(compiler_t *comp) {
    token_t op = *previous(comp->parser);
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
    consume(comp->parser, TOK_RPAREN, "missing ')' after expression");
}

static void print(compiler_t *comp) {
    expression(comp);
    emit_byte(comp, OP_PRINT);
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
    [TOK_STRING] =      {string,    NULL,       PREC_NONE},
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
    [TOK_PRINT] =       {print,     NULL,       PREC_UNARY},
    [TOK_EOF] =         {NULL,      NULL,       PREC_NONE},
    [TOK_INVALID] =     {NULL,      NULL,       PREC_NONE},
};

static void parse_precedence(compiler_t *comp, precedence_t prec) {
    advance(comp->parser);
    parse_fn_t prefix = get_rule(previous(comp->parser)->kind)->prefix;
    if(!prefix) {
        error_at(comp->parser, previous(comp->parser), "expected an expression");
        return;
    }
    prefix(comp);
    
    while(prec <= get_rule(current(comp->parser)->kind)->precedence) {
        advance(comp->parser);
        parse_fn_t infix = get_rule(previous(comp->parser)->kind)->infix;
        infix(comp);
    }
}

static const parse_rule_t *get_rule(token_kind_t kind) {
    return &rules[kind];
}

static void statement(compiler_t *comp) {
    expression(comp);
}

static void declaration(compiler_t *comp) {
    statement(comp);
}

static void program(compiler_t *comp) {
    while(!match(comp->parser, TOK_EOF)) {
        declaration(comp);
    }
}

bool compile(warp_vm_t *vm, chunk_t *chunk, const char *src, size_t length) {
    compiler_t comp;
    
    parser_t parser;
    parser_init_text(&parser, vm, src, length);
    comp.vm = vm;
    comp.parser = &parser;
    comp.chunk = chunk;
    
    advance(comp.parser);
    program(&comp);
    end_compiler(&comp);
    
    consume(comp.parser, TOK_EOF, "expected end of expression");
    
#if DEBUG_PRINT_CODE == 1
    if(!comp.parser.had_error) {
        disassemble_chunk(chunk, "compiled code", stdout);
    }
#endif
    
    return !parser.had_error;
}

