#pragma once
#ifndef _LEXER_H
#define _LEXER_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include "dynarr.h"

typedef enum TokenType {
    // for some reason, if it starts at 0, the parser has a mental breakdown and explodes violently
    TT_EOF=1, TT_ERROR, TT_IDENT, TT_NUM, TT_PLUS, TT_MINUS, TT_STAR, TT_SLASH, TT_LPAREN, TT_RPAREN, TT_COMMA, TT_ASSIGN
} TokenType;

typedef struct Token {
    TokenType type;
    char *start;
    uint32_t length;
    uint32_t line;
} Token;

bool is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

typedef struct Lexer {
    char *start;
    char *current;
    uint32_t line;
    uint32_t length;
} Lexer;

Lexer* lexer_create(const char* expr) {
    Lexer* lexer = malloc(sizeof(Lexer));
    lexer->start = (char*)expr;
    lexer->current = lexer->start;
    lexer->line = 1;
    lexer->length = strlen(expr);
    return lexer;
}

void lexer_free(Lexer* lexer) {
    free(lexer);
}

Token lexer_make_token(Lexer* lexer, TokenType type) {
    Token token;
    token.type = type;
    token.start = lexer->start;
    token.length = (uint32_t)(lexer->current - lexer->start);
    token.line = lexer->line;
    return token;
}

Token lexer_number(Lexer* lexer) {
    while (is_digit(*lexer->current)) lexer->current++;
    if (*lexer->current == '.' && is_digit(*(lexer->current + 1))) {
        lexer->current++;
        while (is_digit(*lexer->current)) lexer->current++;
    }
    return lexer_make_token(lexer, TT_NUM);
}

Token lexer_identifier(Lexer* lexer) {
    while (isalpha(*lexer->current)) lexer->current++;
    return lexer_make_token(lexer, TT_IDENT);
}

Token lexer_next_token(Lexer* lexer) {
    while (is_whitespace(*lexer->current)) {
        if (*lexer->current == '\n') {
            lexer->line++;
        }
        lexer->current++;
    }
    lexer->start = lexer->current;
    if (*lexer->current == '\0') return lexer_make_token(lexer, TT_EOF);
    lexer->current++;
    switch (*(lexer->current - 1)) {
        case '+': return lexer_make_token(lexer, TT_PLUS);
        case '-': return lexer_make_token(lexer, TT_MINUS);
        case '*': return lexer_make_token(lexer, TT_STAR);
        case '/': return lexer_make_token(lexer, TT_SLASH);
        case '(': return lexer_make_token(lexer, TT_LPAREN);
        case ')': return lexer_make_token(lexer, TT_RPAREN);
        case ',': return lexer_make_token(lexer, TT_COMMA);
        case '=': return lexer_make_token(lexer, TT_ASSIGN);
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': return lexer_number(lexer);
        default: {
            if (isalpha(*(lexer->current - 1))) return lexer_identifier(lexer);
            printf("ERROR %d: Unexpected character '%c'\n", lexer->line, *(lexer->current - 1));
            return lexer_make_token(lexer, TT_ERROR);
        }
    }
}


#endif //_LEXER_H
