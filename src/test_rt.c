#define LIB211_RAW_ALLOC
#define LIB211_RAW_EXIT

#include "lib211_test.h"
#include "lib211_io.h"
#include "test_reporting.h"

#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>

#define NORMAL  "\33[0m"
#define RED     "\33[0;31m"
#define GREEN   "\33[0;32m"
#define RVRED   "\33[0;41;37m"

static bool atexit_installed = false;
static bool tests_enabled    = false;
static bool has_run_tests    = false;

static unsigned pass_count   = 0;
static unsigned fail_count   = 0;
static unsigned error_count  = 0;

static void print_test_results(void)
{
    unsigned check_count = pass_count + fail_count + error_count;

    FILE* fout = fail_count || error_count ? stderr : stdout;

    char const* label_style = has_run_tests ? "test" : "check";

    fprintf(fout, "\n");

    if (! check_count) {
        fprintf(fout, "No checks.\n");
        return;
    }

    if (error_count) {
        fprintf(fout,
                RVRED
                "*** %d %s%s could not be completed due to errors."
                NORMAL,
                error_count,
                label_style,
                error_count == 1 ? "" : "s");
    }

    if (!error_count && !(pass_count && fail_count)) {
        const char* descr = pass_count
            ? GREEN "passed" NORMAL
            : RED   "failed" NORMAL;

        switch (check_count) {
            case 1:
                fprintf(fout, "The only %s %s.\n", label_style, descr);
                break;
            case 2:
                fprintf(fout, "Both %ss %s.\n", label_style, descr);
                break;
            default:
                fprintf(fout, "All %d %ss %s.\n",
                        check_count, label_style, descr);
                break;
        }
    } else {
        fprintf(fout, "%d of %d %ss passed.\n",
                pass_count, check_count, label_style);
    }
}

static void exit_hook_function(void)
{
    if (tests_enabled) {
        print_test_results();

        unsigned failures = fail_count + error_count;
        if (failures) {
            _exit(failures);
        }
    }
}

static void install_atexit(void)
{
    if (atexit_installed) return;

    if (atexit(&exit_hook_function)) {
        perror("atexit");
        exit(10);
    }

    atexit_installed = true;
}

void start_testing(void)
{
    install_atexit();
    tests_enabled = true;
}

#define log_check rt211_test_log_check

bool rt211_test_log_check(bool condition, const char* file, int line)
{
    start_testing();

    if (condition) {
        ++pass_count;
    } else {
        ++fail_count;
        eprintf("\nCheck failed (%s:%d):\n", file, line);
    }

    return condition;
}

void rt211_test_log_error(
        char const* context,
        char const* file,
        int line)
{
    start_testing();

    ++error_count;
    eprintf("\nError in %s (%s:%d):\n", context, file, line);
}

static const char* c_escape_of_char(char c) {
    switch (c) {
        case '\a': return "a";
        case '\b': return "b";
        case '\f': return "f";
        case '\n': return "n";
        case '\r': return "r";
        case '\t': return "t";
        case '\v': return "v";
        case '\\': return "\\";
        default:   return NULL;
    }
}

static void eprintf_escaped_char(char c, char quote)
{
    const char* escaped = c_escape_of_char(c);

    if (escaped)
        eprintf("\\%s", escaped);
    else if (c == quote)
        eprintf("\\%c", c);
    else if (isgraph(c) || c == ' ')
        eprintf("%c", c);
    else
        eprintf("\\x%02x", (unsigned char) c);
}

static void eprintf_char_literal(char c) {
    if (c) {
        eprintf("'");
        eprintf_escaped_char(c, '\'');
        eprintf("'");
    } else {
        eprintf("0");
    }
}

static void eprintf_string_literal(const char* s) {
    if (s) {
        eprintf("\"");
        for ( ; *s; ++s) eprintf_escaped_char(*s, '"');
        eprintf("\"");
    } else {
        eprintf("(null)");
    }
}

bool lib211_do_run_test(
        void (*test_fn)(void),
        char const* source_expr,
        char const* file,
        int line)
{
    start_testing();
    has_run_tests = true;

    printf("%s... ", source_expr);
    fflush(stdout);

    pid_t pid = fork();
    if (pid < 0) goto bad_error;

    if (pid == 0) {
        pass_count = fail_count = error_count = 0;

        test_fn();

        // Don't run our exit handler in here.
        tests_enabled = false;

        if (error_count) exit(2);
        else if (fail_count) exit(1);
        else exit(0);
    }

    int status;
    int res = waitpid(pid, &status, 0);
    if (res < 0) goto bad_error;

    if (WIFEXITED(status)) {
        switch (WEXITSTATUS(status)) {
        case 0:
            printf(GREEN "passed" NORMAL ".\n");
            fflush(stdout);
            ++pass_count;
            return true;

        case 1:
            printf("\n%s " RED "failed" NORMAL ".\n", source_expr);
            fflush(stdout);
            ++fail_count;
            return false;
        }
    }

    printf("\n%s " RVRED "errored" NORMAL ".\n", source_expr);
    fflush(stdout);
    ++error_count;
    return false;

bad_error:
    printf("\nunexpected error:\n");
    fflush(stdout);
    perror("RUN_TEST");
    exit(11);
}

bool lib211_do_check(
        bool condition,
        const char* assertion,
        const char* file,
        int line)
{
    if (log_check(condition, file, line)) return true;
    eprintf("  assertion: %s\n", assertion);
    return false;
}

bool lib211_do_check_char(
        char have,
        char want,
        const char* expr_have,
        const char* expr_want,
        const char* file,
        int line)
{
    if (log_check(have == want, file, line)) return true;

    eprintf("  have: ");
    eprintf_char_literal(have);
    eprintf("  (from: %s)\n", expr_have);

    eprintf("  want: ");
    eprintf_char_literal(want);
    eprintf("  (from: %s)\n", expr_want);

    return false;
}

bool lib211_do_check_long(
        long have,
        long want,
        const char* expr_have,
        const char* expr_want,
        const char* file,
        int line)
{
    if (log_check(have == want, file, line)) return true;
    eprintf("  have: %ld  (from: %s)\n", have, expr_have);
    eprintf("  want: %ld  (from: %s)\n", want, expr_want);
    return false;
}

bool lib211_do_check_size(
        size_t have,
        size_t want,
        const char* expr_have,
        const char* expr_want,
        const char* file,
        int line)
{
    if (log_check(have == want, file, line)) return true;
    eprintf("  have: %zu  (from: %s)\n", have, expr_have);
    eprintf("  want: %zu  (from: %s)\n", want, expr_want);
    return false;
}

bool lib211_do_check_double(
        double have,
        double want,
        const char* expr_have,
        const char* expr_want,
        const char* file,
        int line)
{
    if (log_check(have == want, file, line)) return true;
    eprintf("  have: %f  (from: %s)\n", have, expr_have);
    eprintf("  want: %f  (from: %s)\n", want, expr_want);
    return false;
}

bool lib211_do_check_string(
        const char* have,
        const char* want,
        const char* expr_have,
        const char* expr_want,
        const char* file,
        int line)
{
    if (log_check(have && want && strcmp(have, want) == 0, file, line))
        return true;

    if (!want) {
        eprintf("Fatal Error:\n");
        eprintf("The second argument to CHECK_POINTER cannot be NULL.\n");
        eprintf("If NULL is correct, use CHECK_POINTER. See the manual\n");
        eprintf("page for CHECK(3) for additional information.");
        abort();
    }

    eprintf("  have: ");
    eprintf_string_literal(have);
    eprintf("  (from: %s)\n", expr_have);

    eprintf("  want: ");
    eprintf_string_literal(want);
    eprintf("  (from: %s)\n", expr_want);

    return false;
}

bool lib211_do_check_pointer(
        const void* have,
        const void* want,
        const char* expr_have,
        const char* expr_want,
        const char* file,
        int line)
{
    if (log_check(have == want, file, line)) return true;
    eprintf("  have: %p  (from: %s)\n", have, expr_have);
    eprintf("  want: %p  (from: %s)\n", want, expr_want);
    return false;
}

_Noreturn void lib211_exit_rt(int result)
{
    if (tests_enabled) {
        print_test_results();
        tests_enabled = false;
        eprintf("lib211: exit(%d) while testing\n", result);
        if (result == 0) result = fail_count;
    }

    exit(result);
}
