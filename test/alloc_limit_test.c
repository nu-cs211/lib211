#define _XOPEN_SOURCE 700

#include "alloc_record.h"

#include <211.h>
#include <211_alloc_limit.h>

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>


#define ARRAY_LEN(A)   (sizeof(A) / sizeof((A)[0]))

static size_t const stress_count  = 10000,
                    stress_chunk  = 10000;

static void test_no_init(void)
{
    rec_t r[20];
    rec_init(r, 20);

    for (size_t i = 1; i < ARRAY_LEN(r); ++i)
        malloc_success(r, 1 << i);

    free_all(r);
}

static void test_no_limit(void)
{
    alloc_limit_set_no_limit();
    test_no_init();
}

static void test_limit_total(void)
{
    rec_t r[10];
    rec_init(r, 10);

    alloc_limit_set_total(10);

    malloc_success(r, 4);
    malloc_failure(r, 8);
    malloc_success(r, 4);
    malloc_success(r, 1);
    malloc_failure(r, 2);
    free_all(r);
}

static void test_limit_peak(void)
{
    rec_t r[10];
    rec_init(r, 10);

    alloc_limit_set_peak(10);

    malloc_success(r, 4);
    malloc_failure(r, 8);
    malloc_success(r, 4);
    malloc_failure(r, 4);
    malloc_success(r, 1);
    malloc_failure(r, 2);

    free_one(r, 4);

    malloc_success(r, 5);
    malloc_failure(r, 1);

    free_all(r);
}

static void test_reset_limit(void)
{
    test_limit_total();
    test_limit_total();
    test_limit_peak();
    test_no_limit();
    test_no_limit();
    test_limit_peak();
}

static void test_env_var_helper(
        const char* total_limit,
        const char* peak_limit,
        size_t size)
{
    bool is_peak_limit = peak_limit != NULL && total_limit == NULL;

    if (peak_limit)
        setenv("RT211_ALLOC_LIMIT_PEAK", peak_limit, 1);

    if (total_limit)
        setenv("RT211_ALLOC_LIMIT_TOTAL", total_limit, 1);

    void* p;

    assert( (p = malloc(size)) );
    assert( !malloc(1) );

    free(p);
    assert( (bool)(p = malloc(size)) == is_peak_limit );
    free(p);
}

static void test_env_limit_total_bytes(void)
{
    test_env_var_helper("128", NULL, 128);
}

static void test_env_limit_total_megabytes(void)
{
    test_env_var_helper("16 MB", NULL, 16 << 20);
}

static void test_env_limit_peak_kilobytes(void)
{
    test_env_var_helper(NULL, "128k", 128 << 10);
}

static void test_env_limit_both(void)
{
    test_env_var_helper("16b", "16kb", 16);
}

static void test_stressful(void)
{
    size_t sizes[stress_count];
    size_t total = 0;

    srand(time(NULL));

    for (size_t i = 0; i < stress_count; ++i) {
        total += sizes[i] = 1 + rand() % stress_chunk;
    }

    alloc_limit_set_peak(total);

    rec_t r[stress_count + 1];
    rec_init(r, stress_count + 1);

    for (size_t i = 0; i < stress_count; ++i) {
        malloc_success(r, sizes[i]);
    }

    malloc_failure(r, 1);

    for (size_t i = stress_count - 1; i > 0; --i) {
        size_t j    = rand() % (i + 1);
        size_t temp = sizes[i];
        sizes[i]    = sizes[j];
        sizes[j]    = temp;
    }

    for (size_t i = 1; i < stress_count; ++i) {
        free_one(r, sizes[i]);
    }

    malloc_failure(r, total - sizes[0] + 1);
    malloc_success(r, total - sizes[0]);

    free_all(r);

    malloc_success(r, total);
    malloc_failure(r, total);

    free_all(r);
}

int main(void)
{
    RUN_TEST( test_no_init );
    RUN_TEST( test_limit_total );
    RUN_TEST( test_limit_peak );
    RUN_TEST( test_reset_limit );
    RUN_TEST( test_stressful );
    RUN_TEST( test_env_limit_total_bytes );
    RUN_TEST( test_env_limit_total_megabytes );
    RUN_TEST( test_env_limit_peak_kilobytes );
    RUN_TEST( test_env_limit_both );
}
