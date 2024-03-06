#include <stdio.h>

#include "asm.h"

typedef struct InstrVM {
    // Takes a pointer to the instruction array, and runs the instructions
    // variables:
    double vars[127]; // 127 variable max (for now)
    // variable names:
    char* var_names[127]; // to find the name of a variable: search through the array for that variable, the index is the var, vars[index] is the value, var_names[index] is the name
    uint32_t var_count;

    // todo: functions
} InstrVM;

double vm_get_var(InstrVM* vm, Data var, uint32_t line) {
    if (var.type == CONSTANT) {
        return var.data.constant;
    } else if (var.type == VARIABLE) {
        return vm->vars[var.data.variable];
    } else if (var.type == IDENTIFIER) {
        for (int i = 0; i < vm->var_count; i++) {
            if (strcmp(vm->var_names[i], var.data.identifier) == 0) {
                return vm->vars[i];
            }
        }
        printf("ERROR %d: unknown identifier '%s'\n", line, var.data.identifier);
        printf("var_count: %d\n", vm->var_count);
    } else {
        printf("ERROR %d: unknown data type\n", line);
    }
    return 0;
}

#define max(a, b) ((a) > (b) ? (a) : (b))

void update_vars(InstrVM* vm, int out) {
    uint32_t prev_var_count = vm->var_count;
    vm->var_count = max(vm->var_count, out + 1);
    if (vm->var_count > prev_var_count) {
        for (int i = prev_var_count; i < vm->var_count; i++) {
            vm->var_names[i] = "\0";
        }
    }
}

void run_instructions(InstructionArr* instructions) {
    InstrVM* vm = malloc(sizeof(InstrVM));
    vm->var_count = 0;
    for (int i = 0; i < instructions->size; i++) {
        Instruction instr = instructions->data[i];
        switch (instr.type) {
            case BINARY:
                switch (instr.data.binary.op) {
                    case ADD:
                        vm->vars[instr.out] = vm_get_var(vm, instr.data.binary.left, i) + vm_get_var(vm, instr.data.binary.right, i);
                        update_vars(vm, instr.out);
                        break;
                    case SUB:
                        vm->vars[instr.out] = vm_get_var(vm, instr.data.binary.left, i) - vm_get_var(vm, instr.data.binary.right, i);
                        update_vars(vm, instr.out);
                        break;
                    case MUL:
                        vm->vars[instr.out] = vm_get_var(vm, instr.data.binary.left, i) * vm_get_var(vm, instr.data.binary.right, i);
                        update_vars(vm, instr.out);
                        break;
                    case DIV:
                        vm->vars[instr.out] = vm_get_var(vm, instr.data.binary.left, i) / vm_get_var(vm, instr.data.binary.right, i);
                        update_vars(vm, instr.out);
                        break;
                    case ASSIGN:
                        if (instr.data.binary.left.type != IDENTIFIER) {
                            printf("ERROR %d: left side of assignment must be an identifier\n", i);
                            return;
                        }
                        // if identifer already exists, delete the old one AFTER creating the new one
                        bool foundold = false;
                        for (int i2 = 0; i2 < vm->var_count; i2++) {
                            if (strcmp(vm->var_names[i2], instr.data.binary.left.data.identifier) == 0) {
                                foundold = true;
                                break;
                            }
                        }
                        switch (instr.data.binary.right.type) {
                            case CONSTANT:
                                // create a new variable with the name of the left side, and the value of the right side
                                if (instr.out != vm->var_count) {
                                    printf("ERROR %d: cannot reassign a variable with const\n", i);
                                    printf("out: %d, var_count: %d\n", instr.out, vm->var_count);
                                    return;
                                }
                                vm->var_names[instr.out] = instr.data.binary.left.data.identifier;
                                vm->vars[instr.out] = instr.data.binary.right.data.constant;
                                vm->var_count++;
                                break;
                            case VARIABLE:
                                vm->var_names[instr.data.binary.right.data.variable] = instr.data.binary.left.data.identifier;
                                break;
                            case IDENTIFIER: {
                                bool found = false;
                                for (int i2 = 0; i2 < vm->var_count; i2++) {
                                    if (strcmp(vm->var_names[i2], instr.data.binary.right.data.identifier) == 0) {
                                        if (instr.out != vm->var_count) {
                                            printf("ERROR %d: cannot reassign a variable with ident\n", i);
                                            printf("out: %d, var_count: %d\n", instr.out, vm->var_count);
                                            return;
                                        }
                                        vm->var_names[instr.out] = instr.data.binary.left.data.identifier;
                                        vm->vars[instr.out] = vm->vars[i2];
                                        vm->var_count++;
                                        found = true;
                                        break;
                                    }
                                }
                                if (!found) {
                                    printf("ERROR %d: unknown copy identifier '%s'\n", i,
                                           instr.data.binary.right.data.identifier);
                                    return;
                                }
                                break;
                            }
                            default:
                                printf("ERROR %d: right side of assignment must be a constant, variable, or identifier\n", i);
                                return;
                        }
                        // delete old if exists
                        if (foundold) {
                            for (int i2 = 0; i2 < vm->var_count; i2++) {
                                if (strcmp(vm->var_names[i2], instr.data.binary.left.data.identifier) == 0) {
                                    vm->var_names[i2] = "\0";
                                    break;
                                }
                            }
                        }

                        break;
                    case CALL:
                        // only print for now
                        if (instr.data.binary.left.type != IDENTIFIER) {
                            printf("ERROR %d: call must be an identifier\n", i);
                            return;
                        }
                        if (strcmp(instr.data.binary.left.data.identifier, "print") == 0) {
                            if (instr.data.binary.right.type != ARGLIST) {
                                printf("ERROR %d: print must have an argument list\n", i);
                                return;
                            }
                            for (int i = 0; i < instr.data.binary.right.data.arglist.len; i++) {
                                printf("%f ", vm_get_var(vm, instr.data.binary.right.data.arglist.args[i], i));
                            }
                            printf("\n");
                            // return 0
                            vm->vars[instr.out] = 0;
                            update_vars(vm, instr.out);
                        } else {
                            printf("ERROR %d: unknown function '%s'\n", i, instr.data.binary.left.data.identifier);
                        }
                        break;
                }
                break;
            case UNARY:
                switch (instr.data.unary.op) {
                    case NEG:
                        vm->vars[instr.out] = -vm_get_var(vm, instr.data.unary.operand, i);
                        break;
                }
                break;
        }
    }
//    for (int i = 0; i < vm->var_count; i++) {
//        if (vm->var_names[i][0] != '\0') {
//            printf("%%%d/'%s' = %f\n", i, vm->var_names[i], vm->vars[i]);
//        } else {
//            printf("%%%d = %f\n", i, vm->vars[i]);
//
//        }
//    }
    free(vm);
}

int main() {
    InstructionArr* instructions = gen_code("x = (y = 10)");
    print_instructions(instructions);

    run_instructions(instructions);

    return 0;

}