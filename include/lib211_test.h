#pragma once
#define _GNU_SOURCE
// TODO ^^?

#include <stdbool.h>
#include <stddef.h>

// CHECK(A) checks that `A` evaluates to true. (Returns value of `A` in
// case you want it.)
//
// Example:
//
//     CHECK( isinf(1.0 / 0.0) );
//     CHECK( isnan(0.0 / 0.0) );

// The parentheses around `A` in the first argument to `lib211_do_check`
// ensure that if `A` contains a comma then it isn’t considered as
// separate arguments to `lib211_do_check`. The `#A` for the next
// argument turns into the code that you write for `A` but turned into a
// string literal. The next two arguments are filled in by the C
// preprocessor with the filename and line number where `CHECK` is used.
// A function can't find out what file it was used it, but a C
// preprocessor macro can, and then it can pass that information to a
// function to do the real work. The function is defined below.
#define CHECK(A)            lib211_do_check((A), #A, __FILE__, __LINE__)

// CHECK_CHAR(A, B)    checks that A and B evaluate to the same  char.
// CHECK_INT(A, B)     checks that A and B evaluate to the same  long.
// CHECK_UINT(A, B)    checks that A and B evaluate to the same  unsigned long.
// CHECK_DOUBLE(A, B)  checks that A and B evaluate to the same  double.
// CHECK_STRING(A, B)  checks that A and B evaluate to the same  string.
//   (CHECK_STRING checks for NULLs and considers them errors.)
//
// Examples:
//
//   char const *s = "hello, world";
//
//   CHECK_CHAR( s[5], ',' );
//   CHECK_UINT( strlen(s), 12 );
//   CHECK_STRING( s + 2, "llo, world" );
//
#define CHECK_CHAR(A,B)         _LIB211_DISPATCH_CHECK(char, A,B)
#define CHECK_INT(A,B)          _LIB211_DISPATCH_CHECK(long, A,B)
#define CHECK_LONG(A,B)         _LIB211_DISPATCH_CHECK(long, A,B)
#define CHECK_UINT(A,B)         _LIB211_DISPATCH_CHECK(size, A,B)
#define CHECK_ULONG(A,B)        _LIB211_DISPATCH_CHECK(size, A,B)
#define CHECK_SIZE(A,B)         _LIB211_DISPATCH_CHECK(size, A,B)
#define CHECK_DOUBLE(A,B)       _LIB211_DISPATCH_CHECK(double, A,B)
#define CHECK_STRING(A,B)       _LIB211_DISPATCH_CHECK(string, A,B)
#define CHECK_POINTER(A,B)      _LIB211_DISPATCH_CHECK(pointer, A,B)

// RUN_TEST takes a function with no arguments and no results, and
// calls it as a test. (This means it prints progress and success or
// failure information.)
#define RUN_TEST(...)           _LIB211_RUN_TEST(__VA_ARGS__)

// Initializes the test system. The first check will call this
// automatically, but calling it yourself will ensure that you see the
// empty test results if your test program exits before getting to the
// its check.
void start_testing(void);


/*
 * IMPLEMENTATION DETAILS. The full API is documented above. Below this
 * point are implementation details that you don't need to understand in
 * order to use this library.
 */

// We're going to override exit(3) with a function that complains if
// it's called in the midst of a test.
#ifndef LIB211_RAW_EXIT
#  define exit(E)  lib211_exit_rt(E)
#endif

_Noreturn void lib211_exit_rt(int);


// Helper for dispatching type-specific checks above to functions below.
// Note that `T` stands for “tag,” not “type,” since it might not be a type.
#define _LIB211_DISPATCH_CHECK(T, A, B) \
    lib211_do_check_##T((A),(B),#A,#B,__FILE__,__LINE__)


// Helper function used by `CHECK` macro above.
bool lib211_do_check(
        bool condition,         // did the check pass?
        char const* assertion,  // source code of condition checked
        char const* file,       // file name where `CHECK` was used
        int line);              // line number in `file`

// Helper function used by `CHECK_CHAR` macro above.
bool lib211_do_check_char(
        char have,
        char want,
        char const* expr_have,
        char const* expr_want,
        char const* file,
        int line);

// Helper function used by `CHECK_INT` and `CHECK_LONG` macros above.
bool lib211_do_check_long(
        long have,              // number we got
        long want,              // number we expected
        char const* expr_have,  // source expression producing `have`
        char const* expr_want,  // source expression producing `want`
        char const* file,
        int line);

// Helper function used by `CHECK_UINT`, `CHECK_ULONG`, and `CHECK_SIZE`
// macros above.
bool lib211_do_check_size(
        size_t have,
        size_t want,
        char const* expr_have,
        char const* expr_want,
        char const* file,
        int line);

// Helper function used by `CHECK_DOUBLE` macro above.
bool lib211_do_check_double(
        double have,
        double want,
        char const* expr_have,
        char const* expr_want,
        char const* file,
        int line);

// Helper function used by `CHECK_STRING` macro above.
bool lib211_do_check_string(
        char const* have,
        char const* want,
        char const* expr_have,
        char const* expr_want,
        char const* file,
        int line);

// Helper function used by `CHECK_POINTER` macro above.
bool lib211_do_check_pointer(
        void const* have,
        void const* want,
        char const* expr_have,
        char const* expr_want,
        char const* file,
        int line);


/////
/////
///// Implementation of `RUN_TEST`
/////
/////

// Returns its 1st argument.
#define _LIB211_VA_ARGS_1(M, ...)  M

// Returns its 6th argument.
#define _LIB211_VA_ARGS_6(_1, _2, _3, _4, _5, M, ...)  M

// // Helper for dispatching varargs macros to monargs implementations.
#define _LIB211_DISPATCH_4(M, ...) \
    _LIB211_VA_ARGS_6( \
            __VA_ARGS__, M##_4, M##_3, M##_2, M##_1, M##_0, \
    )(__VA_ARGS__)

// Shortcut:
#define _LZIIRT(N)    _LIB211_RUN_TEST_##N

// Dispatch RUN_TEST by arity first:
#define _LIB211_RUN_TEST(...) \
    _LIB211_DISPATCH_4(_LIB211_RUN_TEST_ARITY, __VA_ARGS__)


////
//// Helpers for declaring prototypes in the next section.
////

#define _LIB211_RUN_TEST_COMMON \
    char const *source_expr, \
    void (*test_fn)

#define lib211_run_test_0() \
    lib211_run_test(_LZIIRT(COMMON)(void))
#define lib211_run_test_1(A) \
    _LIB211_RUN_TEST_N(_##A, \
            _LZIIRT(TYPE)(A) a)
#define lib211_run_test_2(A,B) \
    _LIB211_RUN_TEST_N(_##A##_##B, \
            _LZIIRT(TYPE)(A) a, \
            _LZIIRT(TYPE)(B) b)
#define lib211_run_test_3(A,B,C) \
    _LIB211_RUN_TEST_N(_##A##_##B##_##C, \
            _LZIIRT(TYPE)(A) a, \
            _LZIIRT(TYPE)(B) b, \
            _LZIIRT(TYPE)(C) c)
#define lib211_run_test_4(A,B,C,D) \
    _LIB211_RUN_TEST_N(_##A##_##B##_##C##_##D, \
            _LZIIRT(TYPE)(A) a, \
            _LZIIRT(TYPE)(B) b, \
            _LZIIRT(TYPE)(C) c, \
            _LZIIRT(TYPE)(D) d)

#define _LIB211_RUN_TEST_N(TAG, ...) \
    lib211_run_test##TAG( \
            _LZIIRT(COMMON)(__VA_ARGS__), \
            __VA_ARGS__)

#define _LIB211_RUN_TEST_TYPE(TAG)       _LZIIRT(TYPE_##TAG)
#define _LIB211_RUN_TEST_TYPE_B          _Bool
#define _LIB211_RUN_TEST_TYPE_C          char
#define _LIB211_RUN_TEST_TYPE_I          int
#define _LIB211_RUN_TEST_TYPE_S          char const*
#define _LIB211_RUN_TEST_TYPE_Z          size_t



////
//// Type dispatch
////

#define _LIB211_RUN_TEST_TYPECASE(A, B, C, I, S, Z) \
    _Generic(A, \
            _Bool:       B, \
            char:        C, \
            int:         I, \
            long:        I, \
            char const*: S, \
            char*:       S, \
            size_t:      Z, \
            unsigned:    Z  \
            )

#define _LIB211_STRINGIFY(X)   #X

#define _LIB211_RUN_TEST_CHAIN_TO(N, F, ...) \
    _LZIIRT( CHAIN_##N ) (lib211_run_test, ##__VA_ARGS__) ( \
            _LIB211_STRINGIFY(F(__VA_ARGS__)), \
            F, ##__VA_ARGS__ )

#define _LIB211_RUN_TEST_ARITY_0(...) \
    _LZIIRT(CHAIN_TO)(0, __VA_ARGS__)
#define _LIB211_RUN_TEST_ARITY_1(...) \
    _LZIIRT(CHAIN_TO)(1, __VA_ARGS__)
#define _LIB211_RUN_TEST_ARITY_2(...) \
    _LZIIRT(CHAIN_TO)(2, __VA_ARGS__)
#define _LIB211_RUN_TEST_ARITY_3(...) \
    _LZIIRT(CHAIN_TO)(3, __VA_ARGS__)
#define _LIB211_RUN_TEST_ARITY_4(...) \
    _LZIIRT(CHAIN_TO)(4, __VA_ARGS__)

#define _LIB211_RUN_TEST_CHAIN_0(T)  T

#define _LIB211_RUN_TEST_CHAIN_1(T, A) \
    _LZIIRT(TYPECASE)(A, \
            _LZIIRT(CHAIN_0)(T##_B), \
            _LZIIRT(CHAIN_0)(T##_C), \
            _LZIIRT(CHAIN_0)(T##_I), \
            _LZIIRT(CHAIN_0)(T##_S), \
            _LZIIRT(CHAIN_0)(T##_Z))

#define _LIB211_RUN_TEST_CHAIN_2(N, A, ...) \
    _LZIIRT(TYPECASE)(A, \
            _LZIIRT(CHAIN_1)(N##_B, __VA_ARGS__), \
            _LZIIRT(CHAIN_1)(N##_C, __VA_ARGS__), \
            _LZIIRT(CHAIN_1)(N##_I, __VA_ARGS__), \
            _LZIIRT(CHAIN_1)(N##_S, __VA_ARGS__), \
            _LZIIRT(CHAIN_1)(N##_Z, __VA_ARGS__))

#define _LIB211_RUN_TEST_CHAIN_3(N, A, ...) \
    _LZIIRT(TYPECASE)(A, \
            _LZIIRT(CHAIN_2)(N##_B, __VA_ARGS__), \
            _LZIIRT(CHAIN_2)(N##_C, __VA_ARGS__), \
            _LZIIRT(CHAIN_2)(N##_I, __VA_ARGS__), \
            _LZIIRT(CHAIN_2)(N##_S, __VA_ARGS__), \
            _LZIIRT(CHAIN_2)(N##_Z, __VA_ARGS__))

#define _LIB211_RUN_TEST_CHAIN_4(N, A, ...) \
    _LZIIRT(TYPECASE)(A, \
            _LZIIRT(CHAIN_3)(N##_B, __VA_ARGS__), \
            _LZIIRT(CHAIN_3)(N##_C, __VA_ARGS__), \
            _LZIIRT(CHAIN_3)(N##_I, __VA_ARGS__), \
            _LZIIRT(CHAIN_3)(N##_S, __VA_ARGS__), \
            _LZIIRT(CHAIN_3)(N##_Z, __VA_ARGS__) )

////
//// Declare the functions used to provide the run-time implementation of
//// the `RUN_TEST` macro above.
////

#define RUN_TEST_BODY(...)      ;

#define _FOR_EACH_TYPE_0(M) \
        M(B) M(C) M(I) M(S) M(Z)
#define _FOR_EACH_TYPE_1(M,a) \
        M(a,B) M(a,C) M(a,I) M(a,S) M(a,Z)
#define _FOR_EACH_TYPE_2(M,a,b) \
        M(a,b,B) M(a,b,C) M(a,b,I) M(a,b,S) M(a,b,Z)
#define _FOR_EACH_TYPE_3(M,a,b,c) \
        M(a,b,c,B) M(a,b,c,C) M(a,b,c,I) M(a,b,c,S) M(a,b,c,Z)

#define _NEXT_ARITY_0() \
        bool lib211_run_test_0() RUN_TEST_BODY() \
        _FOR_EACH_TYPE_0(_NEXT_ARITY_1)

#define _NEXT_ARITY_1(A) \
        bool lib211_run_test_1(A) RUN_TEST_BODY(a) \
        _FOR_EACH_TYPE_1(_NEXT_ARITY_2, A)

#define _NEXT_ARITY_2(A, B) \
        bool lib211_run_test_2(A, B) RUN_TEST_BODY(a, b) \
        _FOR_EACH_TYPE_2(_NEXT_ARITY_3, A, B)

#define _NEXT_ARITY_3(A, B, C) \
        _FOR_EACH_TYPE_3(_NEXT_ARITY_4, A, B, C) \
        bool lib211_run_test_3(A, B, C) RUN_TEST_BODY(a, b, c)

#define _NEXT_ARITY_4(A, B, C, D) \
        bool lib211_run_test_4(A, B, C, D) RUN_TEST_BODY(a, b, c, d)

_NEXT_ARITY_0()

#undef RUN_TEST_BODY

#ifndef LIB211_KEEP_RUN_TEST_MACROS
#  undef _FOR_EACH_TYPE_0
#  undef _FOR_EACH_TYPE_1
#  undef _FOR_EACH_TYPE_2
#  undef _FOR_EACH_TYPE_3
#  undef _NEXT_ARITY_0
#  undef _NEXT_ARITY_1
#  undef _NEXT_ARITY_2
#  undef _NEXT_ARITY_3
#  undef _NEXT_ARITY_4
#  undef lib211_run_test_0
#  undef lib211_run_test_1
#  undef lib211_run_test_2
#  undef lib211_run_test_3
#endif
