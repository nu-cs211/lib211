#pragma once

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef const char* const_cstr;

#ifndef ARG211_REPR_BYTES
#  define ARG211_REPR_BYTES  (sizeof(void*))
#endif

typedef struct arg211 {
    enum {
        AT_unit,
        AT_char,
        AT_int,
        AT_size,
        AT_double,
        AT_const_cstr,
    } tag;
    union {
        unsigned char repr[ARG211_REPR_BYTES];
    };
} arg211_t;


extern arg211_t
const arg211_unit_value;

#define DECLARE_MAKE_ARG211(A, TAG) \
    arg211_t arg211_##TAG(A a)

DECLARE_MAKE_ARG211(char, char);
DECLARE_MAKE_ARG211(int, int);
DECLARE_MAKE_ARG211(double, double);
DECLARE_MAKE_ARG211(size_t, size);
DECLARE_MAKE_ARG211(const_cstr, const_cstr);

#define make_arg211(A) \
        _Generic((A), \
                char: make_arg211_char, \
                int: make_arg211_int, \
                size: make_arg211_size, \
                double: make_arg211_double, \
                const_cstr: make_arg211_const_cstr, \
                )((A))

#define DECLARE_ARG211_GET(A, TAG) \
    _Bool arg211_get_##TAG(A* dst, arg211_t src)

DECLARE_ARG211_GET(char, char);
DECLARE_ARG211_GET(int, int);
DECLARE_ARG211_GET(double, double);
DECLARE_ARG211_GET(size_t, size);
DECLARE_ARG211_GET(const_cstr, const_cstr);

#define arg211_get(A, TAG) \
        _Generic(*(A), \
                char: arg211_get_char, \
                int: arg211_get_int, \
                size: arg211_get_size, \
                double: arg211_get_double, \
                const_cstr: arg211_get_const_cstr, \
                )((A), (TAG))

