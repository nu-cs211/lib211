#include "lib211_reflect.h"

typedef union
{
    char a_char;
    int a_int;
    double a_double;
    size_t a_size;
    const_cstr a_const_cstr;
} arg211_repr;

_Static_assert(
        sizeof(arg211_repr) <= sizeof arg211_unit_value.repr,
        "arg211 type too small" );

struct arg211
const arg211_unit_value = {
    .tag = AT_unit,
};

#define DEFINE_MAKE_ARG211(A, TAG) \
    arg211_t \
    arg211_##TAG(A a) \
    { \
        arg211_t result; \
        arg211_repr repr = { .a_##TAG = a }; \
        memcpy(result.repr, &repr, sizeof repr); \
        result.tag = AT_##TAG; \
        return result; \
    }

DEFINE_MAKE_ARG211(char, char)
DEFINE_MAKE_ARG211(int, int)
DEFINE_MAKE_ARG211(double, double)
DEFINE_MAKE_ARG211(size_t, size)
DEFINE_MAKE_ARG211(const_cstr, const_cstr)

#define _LIB211_EXPR_ARG_TAG(A) \
    _Generic((A), \
            char: AT_char, \
            int: AT_int, \
            double: AT_double, \
            size_t: AT_size, \
            const_cstr: AT_const_cstr )

#define DEFINE_ARG211_GET(A, TAG) \
    DECLARE_ARG211_GET(A, TAG) \
    { \
        _Bool ok = src.tag == AT_##TAG; \
        if (ok && dst) { \
            arg211_repr repr; \
            memcpy(&repr, &src.repr, sizeof repr); \
            *dst = repr.a_##TAG; \
        } \
        return ok; \
    }

DEFINE_ARG211_GET(char, char)
DEFINE_ARG211_GET(int, int)
DEFINE_ARG211_GET(double, double)
DEFINE_ARG211_GET(size_t, size)
DEFINE_ARG211_GET(const_cstr, const_cstr)

