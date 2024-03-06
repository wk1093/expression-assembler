cmake_minimum_required(VERSION 3.26)
project(expr_asm)

add_executable(expr_asm main.c
        lexer.h
        dynarr.h
        parser.h
        asm.h)
