#pragma once
#ifndef _PARSER_H
#define _PARSER_H

#include "lexer.h"

typedef enum ExprNodeType {
    NT_ERROR, NT_NUMBER, NT_POSITIVE, NT_NEGATIVE, NT_ADD, NT_SUB, NT_MUL, NT_DIV, NT_IDENT, NT_CALL, NT_ARGS, NT_ASSIGN
} ExprNodeType;

typedef struct ExprNode {
    ExprNodeType type;
    union {
        double number;
        struct { struct ExprNode* operand; } unary;
        struct { struct ExprNode* left; struct ExprNode* right; } binary;
        struct { char* identifier; } ident;
    };
    bool top_level;
} ExprNode;

typedef enum NodeClass {
    NC_UNARY, NC_BINARY, NC_VALUE, NC_CALL, NC_ARGS
} NodeClass;

NodeClass get_node_class(ExprNode* node) {
    switch(node->type) {
        case NT_NUMBER: return NC_VALUE;
        case NT_POSITIVE: return NC_UNARY;
        case NT_NEGATIVE: return NC_UNARY;
        case NT_ADD: return NC_BINARY;
        case NT_SUB: return NC_BINARY;
        case NT_MUL: return NC_BINARY;
        case NT_DIV: return NC_BINARY;
        case NT_IDENT: return NC_VALUE;
        case NT_CALL: return NC_CALL;
        case NT_ARGS: return NC_ARGS;
        case NT_ASSIGN: return NC_BINARY;
        default: return NC_UNARY;
    }
}

typedef enum Precedence {
    PREC_MIN, PREC_ASSIGN, PREC_TERM, PREC_FACTOR, PREC_MAX
} Precedence;

static Precedence precedence[] = {
        [TT_PLUS] = PREC_TERM,
        [TT_MINUS] = PREC_TERM,
        [TT_STAR] = PREC_FACTOR,
        [TT_SLASH] = PREC_FACTOR,
        [TT_ASSIGN] = PREC_ASSIGN,
};

typedef struct Parser {
    Lexer* lexer;
    Token curr;
    bool has_first;
} Parser;

void parser_advance(Parser* parser) {
    parser->curr = lexer_next_token(parser->lexer);
}

double token_to_double(Token token) {
    if (token.type != TT_NUM) {
        printf("Warning %d: token_to_double called on non-number token: %d\n", token.line, token.type);
    }
    char* str = malloc(token.length + 1);
    memcpy(str, token.start, token.length);
    str[token.length] = '\0';
    double num = strtod(str, NULL);
    free(str);
    return num;
}

ExprNode* parser_parse_expr(Parser* parser, Precedence prec);

ExprNode* parser_parse_args(Parser* parser) {
    ExprNode* node = malloc(sizeof(ExprNode));
    node->top_level = false;
    node->type = NT_ARGS;
    node->binary.left = parser_parse_expr(parser, PREC_MIN);
    if (parser->curr.type == TT_COMMA) {
        parser_advance(parser);
        node->binary.right = parser_parse_args(parser);
    } else {
        node->binary.right = NULL;
    }
    return node;
}

ExprNode* parser_parse_number(Parser* parser) {
    if (parser->curr.type != TT_NUM) {
        printf("ERROR %d: Expected number\n", parser->curr.line);
        return NULL;
    }
    ExprNode* node = malloc(sizeof(ExprNode));
    node->top_level = false;
    node->type = NT_NUMBER;
    node->number = token_to_double(parser->curr);
    parser_advance(parser);
    return node;
}

ExprNode* parser_parse_ident(Parser* parser) {
    if (parser->curr.type != TT_IDENT) {
        printf("ERROR %d: Expected identifier\n", parser->curr.line);
        return NULL;
    }
    ExprNode* node = malloc(sizeof(ExprNode));
    node->top_level = false;
    node->type = NT_IDENT;
    node->ident.identifier = malloc(parser->curr.length + 1);
    memcpy(node->ident.identifier, parser->curr.start, parser->curr.length);
    node->ident.identifier[parser->curr.length] = '\0';
    parser_advance(parser);
    return node;
}

ExprNode* parser_parse_infix_expr(Parser* parser, Token op, ExprNode* left) {
    ExprNode* node = malloc(sizeof(ExprNode));
    node->top_level = false;
    switch (op.type) {
        case TT_PLUS: node->type = NT_ADD; break;
        case TT_MINUS: node->type = NT_SUB; break;
        case TT_STAR: node->type = NT_MUL; break;
        case TT_SLASH: node->type = NT_DIV; break;
        case TT_ASSIGN: node->type = NT_ASSIGN; break;
        case TT_RPAREN: return left; // I don't know why this happens, but when I change the lexer tokens to start at 1, this isn't necessary, but i'm leaving it in
        default: node->type = NT_ERROR; printf("ERROR %d: infixExpr bad op %d\n", op.line, op.type); break;
    }
    node->binary.left = left;
    node->binary.right = parser_parse_expr(parser, precedence[op.type]);
    return node;
}

ExprNode* parser_parse_terminal_expr(Parser* parser) {
    ExprNode* node = NULL;
    if (parser->curr.type == TT_NUM) {
        node = parser_parse_number(parser);
    } else if (parser->curr.type == TT_IDENT) {
        node = parser_parse_ident(parser);
        if (node == NULL) {
            printf("ERROR %d: Expected identifier\n", parser->curr.line);
        }
    } else if (parser->curr.type == TT_LPAREN) {
        parser_advance(parser);
        node = parser_parse_expr(parser, PREC_MIN);
        if (parser->curr.type == TT_RPAREN) {
            parser_advance(parser);
        } else if (parser->curr.type == TT_COMMA) {
            // we need to progress to the next token
        } else {
            printf("ERROR %d: Expected ')'\n", parser->curr.line);
            return NULL;
        }
    } else if (parser->curr.type == TT_PLUS) {
        parser_advance(parser);
        node = (ExprNode*)malloc(sizeof(ExprNode));
        node->top_level = false;
        node->type = NT_POSITIVE;
        node->unary.operand = parser_parse_terminal_expr(parser);
    } else if (parser->curr.type == TT_MINUS) {
        parser_advance(parser);
        node = (ExprNode*)malloc(sizeof(ExprNode));
        node->top_level = false;
        node->type = NT_NEGATIVE;
        node->unary.operand = parser_parse_terminal_expr(parser);
    } else {
        printf("ERROR %d: Expected number or '(' or unary operator\n", parser->curr.line);
        return NULL;
    }
    if (parser->curr.type == TT_NUM || parser->curr.type == TT_LPAREN || parser->curr.type == TT_IDENT) {
        // call
        ExprNode* call = (ExprNode*)malloc(sizeof(ExprNode));
        call->top_level = false;
        call->type = NT_CALL;
        call->binary.left = node;
        call->binary.right = parser_parse_args(parser);
        node = call;
    }
    return node;
}

ExprNode* parser_parse_expr(Parser* parser, Precedence prec) {
    if (parser->curr.type == TT_EOF) {
        printf("ERROR %d: Expected expression\n", parser->curr.line);
        return NULL;
    } else if (parser->curr.type == TT_COMMA) {
        printf("ERROR %d: Unexpected ','\n", parser->curr.line);
        return NULL;
    }
    ExprNode* left = parser_parse_terminal_expr(parser);
    if (!parser->has_first) {
        left->top_level = true;
        parser->has_first = true;
    }
    Token op = parser->curr;
    if (op.type == TT_EOF) {
        return left;
    } else if (op.type == TT_COMMA) {
        return left;
    } else if (op.type == TT_RPAREN) {
        return left;
    }
    Precedence next_prec = precedence[op.type];
    while (next_prec != PREC_MIN) {
        if (prec >= next_prec) {
            break;
        } else {
            parser_advance(parser);
            left = parser_parse_infix_expr(parser, op, left);
            op = parser->curr;
            next_prec = precedence[op.type];
        }
    }
    return left;
}


Parser* parser_create(const char* expr) {
    Parser* parser = malloc(sizeof(Parser));
    parser->lexer = lexer_create(expr);
    parser->has_first = false;
    parser_advance(parser);
    return parser;
}

void parser_free(Parser* parser) {
    lexer_free(parser->lexer);
    free(parser);
}

void __internal_tree_free(ExprNode* tree) {
    if (tree->type == NT_NUMBER) {
        free(tree);
    } else if (tree->type == NT_POSITIVE || tree->type == NT_NEGATIVE) {
        __internal_tree_free(tree->unary.operand);
        free(tree);
    } else {
        __internal_tree_free(tree->binary.left);
        __internal_tree_free(tree->binary.right);
        free(tree);
    }
}

void tree_free(ExprNode* tree) {
    if (tree->top_level == false) {
        printf("ERROR: Cannot free part of a tree!\n");
    }
    __internal_tree_free(tree);
}

void printTree(ExprNode *tree, uint32_t depth) {
    for(uint32_t i = 0; i < depth; i++) {
        if (i == depth - 1) {
            printf(" |-");
        } else {
            printf(" | ");
        }
    }
    switch(tree->type) {
        case NT_NUMBER: {
            printf("Number: %f\n", tree->number);
        } break;
        case NT_POSITIVE: {
            printf("Positive:\n");
            printTree(tree->unary.operand, depth + 1);
        } break;
        case NT_NEGATIVE: {
            printf("Negative:\n");
            printTree(tree->unary.operand, depth + 1);
        } break;
        case NT_ADD: {
            printf("Add:\n");
            printTree(tree->binary.left, depth + 1);
            printTree(tree->binary.right, depth + 1);
        } break;
        case NT_SUB: {
            printf("Sub:\n");
            printTree(tree->binary.left, depth + 1);
            printTree(tree->binary.right, depth + 1);
        } break;
        case NT_MUL: {
            printf("Mul:\n");
            printTree(tree->binary.left, depth + 1);
            printTree(tree->binary.right, depth + 1);
        } break;
        case NT_DIV: {
            printf("Div:\n");
            printTree(tree->binary.left, depth + 1);
            printTree(tree->binary.right, depth + 1);
        } break;
        case NT_IDENT: {
            printf("Identifier: %s\n", tree->ident.identifier);
        } break;
        case NT_CALL: {
            printf("Call:\n");
            printTree(tree->binary.left, depth + 1);
            printTree(tree->binary.right, depth + 1);
        } break;
        case NT_ARGS: {
            printf("Args:\n");
            printTree(tree->binary.left, depth + 1);
            if (tree->binary.right != NULL) {
                printTree(tree->binary.right, depth + 1);
            }
        } break;
        case NT_ASSIGN: {
            printf("Assign:\n");
            printTree(tree->binary.left, depth + 1);
            printTree(tree->binary.right, depth + 1);
        } break;
        default: {
            printf("printTree: ERR %d\n", tree->type);
            printTree(tree->binary.left, depth + 1);
        } break;
    }
}

#endif //_PARSER_H
