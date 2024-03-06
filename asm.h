#pragma once
#ifndef _ASM_H
#define _ASM_H

#include "parser.h"
#include "dynarr.h"

// above is an expression parser, that creates a tree.
// below converts the tree into assembly-like instructions

typedef enum DataType {
    CONSTANT, VARIABLE, IDENTIFIER, ARGLIST
} DataType;

typedef struct Data {
    // can be a constant, a % variable, or a normal variable/id
    DataType type;
    union {
        double constant; // EX: 1
        int variable; // EX: %1
        char *identifier; // EX: x
        struct {
            // constant, variable, or identifier, not arglist, you can't have an arglist of arglists
            struct Data* args;
            int len;
        } arglist;
    } data;
} Data;

typedef enum BinaryOp {
    ADD, SUB, MUL, DIV, CALL, ASSIGN
} BinaryOp;
typedef enum UnaryOp {
    NEG
} UnaryOp;

typedef enum InstructionType {
    BINARY, UNARY, SET
    // SET isn't really needed, but it might be useful later
} InstructionType;

typedef struct Instruction {
    // -1 means no output, normally used for assignments.
    // -1 is not allowed to be used for SET because then the instruction would be useless
    int out; // index of the variable to store the result in. EX: `%2 = 2 / y` -> 2
    // 3 types of operations: binary, unary, and set
    // binary: %1 = %1 * x
    // unary: %1 = -%1
    // set: %1 = x
    InstructionType type;
    union {
        struct {
            Data left;
            Data right;
            BinaryOp op;
        } binary;
        struct {
            Data operand;
            UnaryOp op;
        } unary;
        Data set;
    } data;
} Instruction;

dynarr(InstructionArr, Instruction);
typedef struct InstructionArr InstructionArr;

typedef struct AsmWriter {
    InstructionArr* instructions;
    uint32_t depth;
    uint32_t cur_var;
} AsmWriter;

Data get_data(ExprNode* expr, AsmWriter* aw);

BinaryOp get_expr_bin_op(const ExprNode* expr) {
    switch(expr->type) {
        case NT_ADD: return ADD;
        case NT_SUB: return SUB;
        case NT_MUL: return MUL;
        case NT_DIV: return DIV;
        case NT_ASSIGN: return ASSIGN;
        default:
            printf("ERROR: unknown binary op\n");
            return -1;
    }
}

int parse_expr_tree(ExprNode* expr_tree, AsmWriter* aw) {
    switch(get_node_class(expr_tree)) {
        case NC_BINARY: {
            Data left = get_data(expr_tree->binary.left, aw);
            Data right = get_data(expr_tree->binary.right, aw);
            if (expr_tree->type == NT_ASSIGN && right.type == VARIABLE) {
                Instruction instr = {
                        .out = -1,
                        .type = BINARY,
                        .data.binary = {
                                .left = left,
                                .right = right,
                                .op = get_expr_bin_op(expr_tree)
                        },
                };
                pushInstructionArr(aw->instructions, instr);
                return instr.out;
            } else {
                Instruction instr = {
                        .out = aw->cur_var++,
                        .type = BINARY,
                        .data.binary = {
                                .left = left,
                                .right = right,
                                .op = get_expr_bin_op(expr_tree)
                        },
                };
                pushInstructionArr(aw->instructions, instr);
                return instr.out;
            }
        } break;
        case NC_UNARY: {
            Data operand = get_data(expr_tree->unary.operand, aw);
            Instruction instr = {
                    .out = aw->cur_var++,
                    .type = UNARY,
                    .data.unary = {
                            .operand = operand,
                            .op = NEG
                    }
            };
            pushInstructionArr(aw->instructions, instr);
            return instr.out;
        } break;
        case NC_VALUE: {
            return get_data(expr_tree, aw).data.variable;
        } break;
        case NC_CALL: {
            Data left = get_data(expr_tree->binary.left, aw);
            Data right = get_data(expr_tree->binary.right, aw);
            Instruction instr = {
                    .out = aw->cur_var++,
                    .type = BINARY,
                    .data.binary = {
                            .left = left,
                            .right = right,
                            .op = CALL
                    }
            };
            pushInstructionArr(aw->instructions, instr);
            return instr.out;
        } break;
        default:
            printf("ERROR: unknown node class\n");
            return -1;
    }
    // will return out index
}

Data get_data(ExprNode* expr, AsmWriter* aw) {
    if (expr->type == NT_NUMBER) {
        const Data data = {
                .type = CONSTANT,
                .data.constant = expr->number
        };
        return data;
    } else if (expr->type == NT_IDENT) {
        const Data data = {
                .type = IDENTIFIER,
                .data.identifier = expr->ident.identifier
        };
        return data;
    } else if (expr->type == NT_ARGS) {
        int size = 0;
        const ExprNode* cur = expr;
        while (cur && cur->type == NT_ARGS) {
            size++;
            cur = cur->binary.right;
        }
        Data* args = malloc(sizeof(Data) * size);
        cur = expr;
        for (int i = 0; i < size; i++) {
            args[i] = get_data(cur->binary.left, aw);
            cur = cur->binary.right;
        }
        const Data arglist = {
                .type = ARGLIST,
                .data.arglist = {
                        .args = args,
                        .len = size
                }
        };

        return arglist;
    }
    else {
        // it's a variable
        Data data = {
                .type = VARIABLE,
                .data.variable = parse_expr_tree(expr, aw)
        };
        return data;
    }
}

InstructionArr* generate_instructions(const char* expr, uint32_t var_offset) {
    Parser* parser = parser_create(expr);
    ExprNode* expr_tree = parser_parse_expr(parser, PREC_MIN);
    InstructionArr* instructions = newInstructionArr();
    AsmWriter aw = {
            .instructions = instructions,
            .depth = 0,
            .cur_var = var_offset
    };
    parse_expr_tree(expr_tree, &aw);
    return instructions;
}

AsmWriter* asm_writer_create(const char* expr, uint32_t var_offset) {
    Parser* parser = parser_create(expr);
    ExprNode* expr_tree = parser_parse_expr(parser, PREC_MIN);
    InstructionArr* instructions = newInstructionArr();
    AsmWriter* aw = malloc(sizeof(AsmWriter));
    aw->instructions = instructions;
    aw->depth = 0;
    aw->cur_var = var_offset;
    parse_expr_tree(expr_tree, aw);
    return aw;
}

void free_instructions(InstructionArr* instructions) {
    delInstructionArr(instructions);
}

void asm_writer_free(AsmWriter* aw) {
    free_instructions(aw->instructions);
    free(aw);
}

void print_data(Data d) {
    switch (d.type) {
        case CONSTANT:
            printf("%f", d.data.constant);
            break;
        case VARIABLE:
            printf("%%%d", d.data.variable);
            break;
        case IDENTIFIER:
            printf("'%s'", d.data.identifier);
            break;
        case ARGLIST:
            printf("arglist(");
            for (int i = 0; i < d.data.arglist.len; i++) {
                print_data(d.data.arglist.args[i]);
                if (i != d.data.arglist.len - 1) {
                    printf(", ");
                }
            }
            printf(")");
            break;
    }
}

void print_instruction(Instruction i) {
    printf("%%%d = ", i.out);
    switch (i.type) {
        case BINARY:
            print_data(i.data.binary.left);
            switch (i.data.binary.op) {
                case ADD:
                    printf(" + ");
                    break;
                case SUB:
                    printf(" - ");
                    break;
                case MUL:
                    printf(" * ");
                    break;
                case DIV:
                    printf(" / ");
                    break;
                case CALL:
                    printf(" (call) ");
                    break;
                case ASSIGN:
                    printf(" = ");
                    break;
            }
            print_data(i.data.binary.right);
            break;
        case UNARY:
            switch (i.data.unary.op) {
                case NEG:
                    printf("- ");
                    break;
            }
            print_data(i.data.unary.operand);
            break;
        case SET:
            print_data(i.data.set);
            break;
    }
    printf("\n");
}

void print_instructions(InstructionArr* instructions) {
    for (int i = 0; i < instructions->size; i++) {
        print_instruction(instructions->data[i]);
    }
}

InstructionArr* gen_code(const char* expr) {
    // split by semicolons, and offset variables by previous instructions
    // then return the instructions
    // split string
    char* str = strdup(expr);
    char* token = strtok(str, ";");
    InstructionArr* instructions = newInstructionArr();
    uint32_t offset = 0;
    while (token) {
        AsmWriter* aw = asm_writer_create(token, offset);
        for (int i = 0; i < aw->instructions->size; i++) {
            pushInstructionArr(instructions, aw->instructions->data[i]);
        }
        offset = aw->cur_var;
        asm_writer_free(aw);
        token = strtok(NULL, ";");
    }
    free(str);
    return instructions;
}


#endif //_ASM_H
