//===--------------------------------------------------------------------------------------------===
// scanner.c
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#include "scanner.h"
#include <string.h>

static const char * token_names[] = {
    [TOK_LPAREN] = "l_paren",
    [TOK_RPAREN] = "r_paren",
    [TOK_LBRACE] = "l_brace",
    [TOK_RBRACE] = "r_brace",
    [TOK_LBRACKET] = "l_bracket",
    [TOK_RBRACKET] = "r_bracket",

    [TOK_PLUS] = "plus",
    [TOK_MINUS] = "minus",
    [TOK_SLASH] = "slash",
    [TOK_STAR] = "star",
    [TOK_STARSTAR] = "star_star",
    [TOK_PERCENT] = "percent",
    [TOK_CARET] = "caret",
    [TOK_TILDE] = "tilde",
    [TOK_AMP] = "amp",
    [TOK_PIPE] = "pipe",
    [TOK_BANG] = "bang",
    [TOK_QUESTION] = "question",
    [TOK_LT] = "less",
    [TOK_GT] = "greater",
    [TOK_EQUALS] = "equal",
    [TOK_LTEQ] = "less_equal",
    [TOK_GTEQ] = "greater_equal",
    [TOK_EQEQ] = "equal_equal",
    [TOK_PLUSEQ] = "plus_eqal",
    [TOK_MINUSEQ] = "minus_eqal",
    [TOK_STAREQ] = "star_eqal",
    [TOK_SLASHEQ] = "slash_eqal",
    [TOK_BANGEQ] = "bang_eqal",
    [TOK_LTLT] = "less_less",
    [TOK_GTGT] = "greater_greater",
    [TOK_GTGTEQ] = "greater_greater_equal",
    [TOK_AMPAMP] = "and_and",
    [TOK_PIPEPIPE] = "pipe_pipe",
    
    [TOK_SEMICOLON] = "semicolon",
    [TOK_NEWLINE] = "newline",
    
    [TOK_COLON] = "colon",
    [TOK_COMMA] = "comma",
    [TOK_DOT] = "dot",
    [TOK_ARROW] = "arrow",
    [TOK_THEN] = "then",
    
    [TOK_NUMBER_LITERAL] = "number_literal",
    [TOK_STRING_LITERAL] = "string_literal",
    [TOK_IDENTIFIER] = "identifier",
    
    [TOK_SELF] = "self",
    [TOK_TRUE] = "true",
    [TOK_FALSE] = "false",
    [TOK_NIL] = "nil",
    [TOK_FUN] = "fun",
    [TOK_VAR] = "var",
    [TOK_LET] = "let",
    [TOK_RETURN] = "return",
    [TOK_FOR] = "for",
    [TOK_WHILE] = "while",
    [TOK_BREAK] = "break",
    [TOK_CONTINUE] = "continue",
    [TOK_IF] = "if",
    [TOK_ELSE] = "else",
    [TOK_INIT] = "init",
    [TOK_PRINT] = "print",
    
    [TOK_EOF] = "eof",
    [TOK_INVALID] = "invalid",
};

const char *token_name(token_kind_t kind) {
    return token_names[kind];
}

static token_t make_token(scanner_t *scanner, token_kind_t kind) {
    token_t token;
    token.kind = kind;
    token.start = scanner->start;
    token.length = (int)(scanner->current - scanner->start);
    token.loc = scanner->loc;
    return token;
}

static token_t error_token(scanner_t *scanner, const char *msg) {
    token_t token;
    token.kind = TOK_INVALID;
    token.start = msg;
    token.length = strlen(msg);
    token.loc = scanner->loc;
    return token;
}

static bool is_at_end(const scanner_t *scanner) {
    return *scanner->current == '\0';
}

static inline const char *src_end(const scanner_t *scanner) {
    return scanner->source.end;
}

static inline size_t src_left(const scanner_t *scanner) {
    return (size_t)(src_end(scanner) - scanner->current);
}

static unicode_scalar_t advance(scanner_t *scanner) {
    unicode_scalar_t current = scanner->copy;
    scanner->current += unicode_utf8_size(scanner->copy);

    uint8_t size = 0;
    scanner->copy = unicode_utf8_read(scanner->current, src_left(scanner), &size);
    return size ? current : '\0';
}

static unicode_scalar_t peek(const scanner_t *scanner) {
    return scanner->copy;
}

static unicode_scalar_t peek_next(const scanner_t *scanner) {
    if(is_at_end(scanner)) return '\0';
    uint8_t current_size = unicode_utf8_size(scanner->copy);
    uint8_t size = 0;
    unicode_scalar_t nc = unicode_utf8_read(scanner->current + current_size,
                                            src_left(scanner)-current_size,
                                            &size);
    return size ? nc : '\0';
}


static void skip_whitespace(scanner_t *scanner) {
    for(;;) {
        unicode_scalar_t c = peek(scanner);
        switch(c) {
        case ' ':
        case '\t':
        case '\r':
            advance(scanner);
            break;
            
        case '\n':
            advance(scanner);
            scanner->loc ++;
            break;
            
        case '/':
            if(peek_next(scanner) == '/') {
                while(peek(scanner) != '\n' && !is_at_end(scanner)) advance(scanner);
            // } else if(peek_next(scanner) == '*') {
            //     block_comment(scanner);
            } else {
                return;
            }
            break;
            
        default:
            return;
        }
    }
}

static bool match(scanner_t *scanner, unicode_scalar_t expected) {
    if(is_at_end(scanner)) return false;
    if(peek(scanner) != expected) return false;
    advance(scanner);
    return true;
}

static inline bool is_digit(unicode_scalar_t c) {
    return (c >= '0' && c <= '9');
}

static token_t string(scanner_t *scanner) {
    while(peek(scanner) != '"' && !is_at_end(scanner)) {
        if(peek(scanner) == '\n') scanner->loc++;
        advance(scanner);
    }
    
    if(is_at_end(scanner)) return error_token(scanner, "unterminated character string");
    
    advance(scanner); // We make sure to eat the closing quote
    return make_token(scanner, TOK_STRING_LITERAL);
}

static token_t number(scanner_t *scanner) {
    while(is_digit(peek(scanner))) {
        advance(scanner);
    }
    
    if(peek(scanner) == '.' && is_digit(peek_next(scanner))) {
        advance(scanner);
        
        while(is_digit(peek(scanner))) {
            advance(scanner);
        }
    }
    
    return make_token(scanner, TOK_NUMBER_LITERAL);
}

static token_kind_t check_keyword(const scanner_t *scanner,
                                  int start,
                                  int length,
                                  const char *rest,
                                  token_kind_t kind) {
    if(scanner->current - scanner->start != start + length) return TOK_IDENTIFIER;
    if(memcmp(scanner->start+start, rest, length)) return TOK_IDENTIFIER;
    return kind;
}

static token_kind_t identifier_kind(const scanner_t *scanner) {
    switch(scanner->start[0]) {
    case 'b': return check_keyword(scanner, 1, 4, "reak", TOK_BREAK);
    case 'c': return check_keyword(scanner, 1, 7, "ontinue", TOK_CONTINUE);
    case 'e': return check_keyword(scanner, 1, 3, "lse", TOK_ELSE);
    case 'f':
        if (scanner->current - scanner->start > 1) {
            switch(scanner->start[1]) {
                case 'a': return check_keyword(scanner, 2, 3, "lse", TOK_FALSE);
                case 'o': return check_keyword(scanner, 2, 1, "r", TOK_FOR);
                case 'u': return check_keyword(scanner, 2, 1, "n", TOK_FUN);
            }
        }
        break;    
    case 'i':
        if (scanner->current - scanner->start > 1) {
            switch(scanner->start[1]) {
                case 'f': return check_keyword(scanner, 2, 0, "", TOK_IF);
                case 'n': return check_keyword(scanner, 2, 2, "it", TOK_INIT);
            }
        }
        break;
    case 'l': return check_keyword(scanner, 1, 2, "et", TOK_LET);
    case 'n': return check_keyword(scanner, 1, 2, "il", TOK_NIL);
    case 'p': return check_keyword(scanner, 1, 4, "rint", TOK_PRINT);
    case 'r': return check_keyword(scanner, 1, 5, "eturn", TOK_RETURN);
    case 't': return check_keyword(scanner, 1, 3, "rue", TOK_TRUE);
    case 'v': return check_keyword(scanner, 1, 2, "ar", TOK_VAR);
    case 'w': return check_keyword(scanner, 1, 4, "hile", TOK_WHILE);
    }
    return TOK_IDENTIFIER;
}

static token_t identifier(scanner_t *scanner) {
    while(unicode_is_identifier(peek(scanner))) {
        advance(scanner);
    }
    return make_token(scanner, identifier_kind(scanner));
}

token_t scan_token(scanner_t *scanner) {
    skip_whitespace(scanner);
    scanner->start = scanner->current;
    if(is_at_end(scanner)) return make_token(scanner, TOK_EOF);
    
    unicode_scalar_t c = advance(scanner);
    

    if(is_digit(c)) return number(scanner);
    if(unicode_is_identifier_head(c)) return identifier(scanner);
    
    switch(c) {
        case ';': return make_token(scanner, TOK_SEMICOLON);
        
        case '{': return make_token(scanner, TOK_LBRACE);
        case '}': return make_token(scanner, TOK_RBRACE);
        case '[': return make_token(scanner, TOK_LBRACKET);
        case ']': return make_token(scanner, TOK_RBRACKET);
        case '(': return make_token(scanner, TOK_LPAREN);
        case ')': return make_token(scanner, TOK_RPAREN);
        case ':': return make_token(scanner, TOK_COLON);
        case '.': return make_token(scanner, TOK_DOT);
        case ',': return make_token(scanner, TOK_COMMA);

        case '^': return make_token(scanner, TOK_CARET);
        case '~': return make_token(scanner, TOK_TILDE);
        case '%': return make_token(scanner, TOK_PERCENT);
        case '?': return make_token(scanner, TOK_QUESTION);
        
        case '+': return make_token(scanner, match(scanner, '=') ? TOK_PLUSEQ : TOK_PLUS);
        case '-': return make_token(scanner, match(scanner, '=') ? TOK_MINUSEQ : TOK_MINUS);
        case '*': return make_token(scanner, match(scanner, '=') ? TOK_STAREQ : TOK_STAR);
        case '/': return make_token(scanner, match(scanner, '=') ? TOK_SLASHEQ : TOK_SLASH);
        
        case '=': return make_token(scanner, match(scanner, '=') ? TOK_EQEQ : TOK_EQUALS);
        case '!': return make_token(scanner, match(scanner, '=') ? TOK_BANGEQ : TOK_BANG);
        case '>': return make_token(scanner, match(scanner, '=') ? TOK_GTEQ : TOK_GT);
        case '<': return make_token(scanner, match(scanner, '=') ? TOK_LTEQ : TOK_LT);
        
        case '"': return string(scanner);
    }
    
    return error_token(scanner, "unexpected character");
}


/*
    src_t source;
    const char *start;
    const char *current;
    src_loc_t loc;
    unicode_scalar_t copy;
*/

void scanner_init_text(scanner_t *scanner, const char *text, size_t length) {
    UNUSED(scanner);
    UNUSED(text);
    scanner->source.start = text;
    scanner->source.end = text + length;
    scanner->start = text;
    scanner->current = text;
    scanner->loc = 1;
    
    uint8_t size = 0;
    scanner->copy = unicode_utf8_read(text, src_left(scanner), &size);
    if(!size) scanner->copy = '\0';
}
