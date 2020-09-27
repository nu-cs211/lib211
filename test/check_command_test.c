#include <lib211.h>

static void test_true_cmd(void)
{
    PROC_SPEC spec = SPEC_BLANK;
    CHECK_COMMAND("true", &spec);
}

static void test_false_cmd(void)
{
    PROC_SPEC spec = SPEC_BLANK;
    spec.status = 1;
    CHECK_COMMAND("false", &spec);
}

static void test_exit_5_cmd(void)
{
    PROC_SPEC spec = SPEC_BLANK;
    spec.status = 5;
    CHECK_COMMAND("exit 5", &spec);
}

static void test_echo_out_cmd(void)
{
    PROC_SPEC spec = SPEC_BLANK;
    spec.out = "hello world\n";
    CHECK_COMMAND("echo hello world", &spec);
}

static void test_echo_err_cmd(void)
{
    PROC_SPEC spec = SPEC_BLANK;
    spec.err    = "hello world\n";
    CHECK_COMMAND("echo hello world >&2", &spec);
}

static void test_echo_both_cmd(void)
{
    PROC_SPEC spec = SPEC_BLANK;
    spec.err    = "goodbye world\n";
    spec.out    = "hello world\n";
    CHECK_COMMAND("echo hello world; echo goodbye world >&2", &spec);
}


static void test_grep_exec(void)
{
    PROC_SPEC spec = {
        .status = 0,
        .in     = "spaceless\nspace full\n   \nnope\n",
        .err    = "",
        .out    = "space full\n   \n"
    };

    char const* argv[] = { "grep", "-E", " " };
    CHECK_EXEC(3, argv, &spec);
}

static void test_cat_exec(void)
{
    PROC_SPEC spec = SPEC_BLANK;
    spec.status = 1;
    spec.in     = "blah blah blah\n";
    spec.err    = "cat: meow: No such file or directory\n";

    char const* argv[] = { "cat", "meow" };
    CHECK_EXEC(2, argv, &spec);
}

static void test_env_exit_code(const char* env1, int expect_status)
{
    PROC_SPEC spec = SPEC_BLANK;
    spec.status = expect_status;
    spec.err    = NULL;

    char const* argv[] = { "build/alloc_limit_test" };
    CHECK_EXEC_ENV(1, argv, env1, &spec);
}

static void test_env_alloc_limit_50_B(void)
{
    test_env_exit_code("RT211_ALLOC_LIMIT=50", 2);
}

static void test_env_alloc_limit_50_MB(void)
{
    test_env_exit_code("RT211_ALLOC_LIMIT=50M", 1);
}

static void test_env_heap_limit_50_MB(void)
{
    test_env_exit_code("RT211_HEAP_LIMIT=50M", 0);
}

static void test_env_heap_limit_50_GB(void)
{
    test_env_exit_code("RT211_HEAP_LIMIT=50G", 0);
}

static void run_test(char const* name, void (*test_fn)(void))
{
    fprintf(stderr, "%s... ", name);
    test_fn();
    fprintf(stderr, " done\n");
}

#define TEST(N)  run_test(#N, &N)

int main(void)
{
    TEST(test_true_cmd);
    TEST(test_false_cmd);
    TEST(test_exit_5_cmd);
    TEST(test_echo_out_cmd);
    TEST(test_echo_err_cmd);
    TEST(test_echo_both_cmd);

    TEST(test_grep_exec);
    TEST(test_cat_exec);

    TEST(test_env_alloc_limit_50_B);
    TEST(test_env_alloc_limit_50_MB);
    TEST(test_env_heap_limit_50_MB);
    TEST(test_env_heap_limit_50_GB);
}
