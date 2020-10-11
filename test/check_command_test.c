#include <lib211.h>

static void test_true_cmd(void)
{
    CHECK_COMMAND("true", "", "", "", 0);
}

static void test_false_cmd(void)
{
    CHECK_COMMAND("false", "", "", "", 1);
}

static void test_exit_5_cmd(void)
{
    CHECK_COMMAND("exit 5",
            "", "", "", 5);
}

static void test_echo_out_cmd(void)
{
    CHECK_COMMAND("echo hello world",
            "", "hello world\n", "", 0);
}

static void test_echo_err_cmd(void)
{
    CHECK_COMMAND("echo hello world >&2",
            "", "", "hello world\n", 0);
}

static void test_echo_both_cmd(void)
{
    CHECK_COMMAND("echo hello world; echo goodbye world >&2",
            "", "hello world\n", "goodbye world\n", 0);
}


static void test_grep_exec(void)
{
    char const* argv[] = { "grep", "-E", " ", NULL };
    CHECK_EXEC(argv,
            "spaceless\nspace full\n   \nnope\n",
            "space full\n   \n",
            "",
            0);
}

static void test_cat_exec(void)
{

    char const* argv[] = { "cat", "meow", NULL };
    CHECK_EXEC(argv,
            "blah blah blah\n",
            "",
            "cat: meow: No such file or directory\n",
            1);
}

static void test_env_exit_code(const char* env1, int expect_status)
{
    char const* argv[] = {
        "/usr/bin/env",
        env1,
        "build/alloc_limit_test",
        NULL,
    };

    CHECK_EXEC(argv, "", "", ANY_OUTPUT, expect_status);
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

int main(void)
{
    RUN_TEST(test_true_cmd);
    RUN_TEST(test_false_cmd);
    RUN_TEST(test_exit_5_cmd);
    RUN_TEST(test_echo_out_cmd);
    RUN_TEST(test_echo_err_cmd);
    RUN_TEST(test_echo_both_cmd);

    RUN_TEST(test_grep_exec);
    RUN_TEST(test_cat_exec);

    RUN_TEST(test_env_alloc_limit_50_B);
    RUN_TEST(test_env_alloc_limit_50_MB);
    RUN_TEST(test_env_heap_limit_50_MB);
    RUN_TEST(test_env_heap_limit_50_GB);
}
