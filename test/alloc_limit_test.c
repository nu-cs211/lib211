#define _XOPEN_SOURCE 700
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


#define NORMAL  "\33[0m"
#define RED     "\33[0;31m"
#define GREEN   "\33[0;32m"
#define RVRED   "\33[0;41;37m"

static size_t stress_count  = 10000;
static size_t stress_chunk  = 10000;

static void test_no_init(void)
{
    for (char i = 0; i < 20; ++i)
        assert( malloc(1 << i) );
}

static void test_no_limit(void)
{
    alloc_limit_set_no_limit();
    test_no_init();
}

static void test_limit_total(void)
{
    alloc_limit_set_total(10);

    assert( malloc(4) );
    assert( ! malloc(8) );
    assert( malloc(4) );
    assert( ! malloc(4) );
    assert( malloc(1) );
    assert( ! malloc(2) );
}

static void test_limit_peak(void)
{
    alloc_limit_set_peak(10);

    void *p1, *p2, *p3;

    assert( (p1 = malloc(4)) );
    assert( ! malloc(8) );
    assert( (p2 = malloc(4)) );
    assert( ! malloc(4) );
    assert( (p3 = malloc(1)) );
    assert( ! malloc(2) );

    free(p1);
    assert( (p1 = malloc(5)) );
    assert( ! malloc(1) );
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

struct allocation
{
    void* ptr;
    size_t size;
};

static void swap(struct allocation a[], size_t i, size_t j)
{
    struct allocation t;

    t    = a[i];
    a[i] = a[j];
    a[j] = t;
}

static void test_env_var_helper(
        const char* alloc_limit,
        const char* heap_limit,
        size_t size,
        bool is_heap_limit)
{
    if (alloc_limit)
        setenv("RT211_ALLOC_LIMIT", alloc_limit, 1);

    if (heap_limit)
        setenv("RT211_HEAP_LIMIT", heap_limit, 1);

    void* p;

    assert( (p = malloc(size)) );
    assert( !malloc(1) );

    free(p);
    assert( (bool)malloc(size) == is_heap_limit );
}

static void test_env_limit_alloc_bytes(void)
{
    test_env_var_helper("128", NULL, 128, false);
}

static void test_env_limit_alloc_megabytes(void)
{
    test_env_var_helper("16 MB", NULL, 16 << 20, false);
}

static void test_env_limit_heap_kilobytes(void)
{
    test_env_var_helper(NULL, "128k", 128 << 10, true);
}

static void test_env_limit_both(void)
{
    test_env_var_helper("16b", "16kb", 16, false);
}

static void test_stressful(void) {
    struct allocation allocations[stress_count];
    size_t total = 0;
    void* p;

    srand(time(NULL));

    for (size_t i = 0; i < stress_count; ++i)
        total += allocations[i].size = rand() % stress_chunk + 1;

    alloc_limit_set_peak(total);

    for (size_t i = 0; i < stress_count; ++i)
        assert( (allocations[i].ptr = malloc(allocations[i].size)) );

    assert( ! malloc(1) );

    for (size_t i = stress_count - 1; i > 0; --i)
        swap(allocations, i, rand() % (i + 1));

    for (size_t i = 1; i < stress_count; ++i)
        free(allocations[i].ptr);

    assert( ! malloc(total - allocations[0].size + 1) );
    assert( (p = malloc(total - allocations[0].size)) );

    free(p);
    free(allocations[0].ptr);

    assert( malloc(total) );
    assert( ! malloc(total) );
}

static int attempted_tests = 0;
static int passed_tests    = 0;

void run_test(char const* name, void (*test_fn)(void))
{
    ++attempted_tests;

    pid_t pid = fork();

    if (pid < 0) {
        perror(name);
        exit(1);
    }

    if (pid == 0) {
        eprintf("%s... ", name);
        fflush(stdout);

        int fd = open("/dev/null", O_RDONLY);
        if (fd >= 0) {
            dup2(fd, 1);
            dup2(fd, 2);
        }

        test_fn();
        exit(0);
    }

    int status;

    if (wait(&status) < 0) {
        perror(name);
        exit(2);
    }

    if (WIFEXITED(status)) {
        if (WEXITSTATUS(status) == 0) {
            eprintf(GREEN "passed" NORMAL "\n");
            ++passed_tests;
        } else {
            eprintf(RVRED "unexpected exit %d" NORMAL "\n",
                    WEXITSTATUS(status));
        }
    } else if (WIFSIGNALED(status)) {
        if (WTERMSIG(status) == SIGABRT) {
            eprintf(RED "failed" NORMAL "\n");
        } else {
            eprintf(RED "crashed from signal %d" NORMAL "\n",
                    WTERMSIG(status));
        }
    }
}

#define TEST(N)  run_test(#N, &N)

int main(void)
{
    TEST(test_no_init);
    TEST(test_limit_total);
    TEST(test_limit_peak);
    TEST(test_reset_limit);
    TEST(test_stressful);
    TEST(test_env_limit_alloc_bytes);
    TEST(test_env_limit_alloc_megabytes);
    TEST(test_env_limit_heap_kilobytes);
    TEST(test_env_limit_both);

    return attempted_tests - passed_tests;
}
