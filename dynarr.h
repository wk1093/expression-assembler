#pragma once
#ifndef _DYNARR_H
#define _DYNARR_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define fail(msg) printf("ERROR (line %d in '%s'): %s\n", __LINE__, __FILE__, msg);

#define dynarr(name, type)                                                                                                                           \
    struct name {                                                                                                                                    \
        type* data;                                                                                                                                  \
        int size;                                                                                                                                    \
        int capacity;                                                                                                                                \
        void (*push)(struct name*, type);                                                                                                            \
        type (*pop)(struct name*);                                                                                                                   \
        type (*get)(struct name*, int);                                                                                                              \
        void (*set)(struct name*, int, type);                                                                                                        \
        void (*clear)(struct name*);                                                                                                                 \
        void (*del)(struct name*);                                                                                                                   \
        type (*remove)(struct name*, int);                                                                                                           \
        struct name* (*copy)(struct name*);                                                                                                          \
    };                                                                                                                                               \
    void push##name(struct name* t, type i) {                                                                                                 \
        if (t->size == t->capacity) {                                                                                                                \
            t->capacity *= 2;                                                                                                                        \
            t->data = (type*)realloc(t->data, t->capacity * sizeof(type));                                                                           \
            if (t->data == NULL) {                                                                                                                   \
                fail("Out of memory!");                                                                                                              \
            }                                                                                                                                        \
        }                                                                                                                                            \
        t->data[t->size] = i;                                                                                                                        \
        t->size++;                                                                                                                                   \
    }                                                                                                                                              \
    type pop##name(struct name* t) {                                                                                                          \
        if (t->size == 0) {                                                                                                                          \
            fail("Cannot pop from an empty vector!");                                                                                                \
        }                                                                                                                                            \
        t->size--;                                                                                                                                   \
        return t->data[t->size];                                                                                                                     \
    }                                                                                                                                              \
    type get##name(struct name* t, int i) {                                                                                                   \
        if (i < 0 || i >= t->size) {                                                                                                                 \
            fail("Index out of bounds!");                                                                                                            \
        }                                                                                                                                            \
        return t->data[i];                                                                                                                           \
    }                                                                                                                                              \
    void set##name(struct name* t, int i, type v) {                                                                                           \
        if (i < 0 || i >= t->size) {                                                                                                                 \
            fail("Index out of bounds!");                                                                                                            \
        }                                                                                                                                            \
        t->data[i] = v;                                                                                                                              \
    }                                                                                                                                              \
    void clear##name(struct name* t) { t->size = 0; }                                                                                       \
    void del##name(struct name* t) {                                                                                                          \
        free(t->data);                                                                                                                               \
        free(t);                                                                                                                                     \
    }                                                                                                                                              \
    type remove##name(struct name* t, int i) {                                                                                                \
        if (i < 0 || i >= t->size) {                                                                                                                 \
            fail("Index out of bounds!");                                                                                                            \
        }                                                                                                                                            \
        type v = t->data[i];                                                                                                                         \
        for (int j = i; j < t->size - 1; j++) {                                                                                                      \
            t->data[j] = t->data[j + 1];                                                                                                             \
        }                                                                                                                                            \
        t->size--;                                                                                                                                   \
        return v;                                                                                                                                    \
    }                                                                                                                                              \
    struct name* new##name();                                                                                                                        \
    struct name* copy##name(struct name* in) {                                                                                                \
        struct name* out = new##name();                                                                                                              \
        for (int i = 0; i < in->size; i++) {                                                                                                         \
            out->push(out, in->get(in, i));                                                                                                          \
        }                                                                                                                                            \
        return out;                                                                                                                                  \
    }                                                                                                                                              \
    struct name* new##name() {                                                                                                                \
        struct name* t = (struct name*)malloc(sizeof(struct name));                                                                                  \
        t->data = (type*)malloc(10 * sizeof(type));                                                                                                  \
        t->size = 0;                                                                                                                                 \
        t->capacity = 10;                                                                                                                            \
        t->push = &push##name;                                                                                                                       \
        t->pop = &pop##name;                                                                                                                         \
        t->get = &get##name;                                                                                                                         \
        t->set = &set##name;                                                                                                                         \
        t->clear = &clear##name;                                                                                                                     \
        t->del = &del##name;                                                                                                                         \
        t->remove = &remove##name;                                                                                                                   \
        return t;                                                                                                                                    \
    }


#endif //_DYNARR_H
