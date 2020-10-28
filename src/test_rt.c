#define LIB211_RAW_ALLOC
#define LIB211_RAW_EXIT

#define _BSD_SOURCE
#define _GNU_SOURCE
#define _SVID_SOURCE
#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 700

#include "211.h"
#include "test_reporting.h"

#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef LIB211_HAS_POSIX
#   include <sys/mman.h>
#   include <sys/types.h>
#   include <sys/wait.h>
#endif

#define NORMAL  "\33[0m"
#define RED     "\33[0;31m"
#define GREEN   "\33[0;32m"
#define RVRED   "\33[0;41;37m"

static bool atexit_installed = false;
static bool tests_enabled    = false;

static unsigned pass_count   = 0;
static unsigned fail_count   = 0;
static unsigned error_count  = 0;

static void print_test_results(void)
{
    unsigned check_count = pass_count + fail_count + error_count;
    FILE* fout = fail_count || error_count ? stderr : stdout;
    bool const use_color = isatty(fileno(fout));
    char const* const label_style = "check";

    fprintf(fout, "\n");

    if (! check_count) {
        fprintf(fout, "No checks.\n");
        return;
    }

    if (error_count) {
        if (use_color) fprintf(fout, RVRED);
        fprintf(fout,
                "*** %d %s%s could not be completed due to errors.",
                error_count,
                label_style,
                error_count == 1 ? "" : "s");
        if (use_color) fprintf(fout, NORMAL);
    }

    if (!error_count && !(pass_count && fail_count)) {
        const char* descr = pass_count
            ? use_color ? GREEN "passed" NORMAL : "passed"
            : use_color ? RED   "failed" NORMAL : "failed";

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
        char const* const file,
        int         const line,
        char const* const context,
        char const* const message)
{
    start_testing();

    ++error_count;
    fprintf(stderr, "\nError in %s (%s:%d)", context, file, line);

    if (message) {
        fprintf(stderr, ":\n  reason: %s\n", message);
    } else {
        fprintf(stderr, "\n");
    }
}

void rt211_test_log_perror(
        char const* const file,
        int         const line,
        char const* const context)
{
    rt211_test_log_error(file, line, context,
                         errno ? strerror(errno) : NULL);
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

static void color_word(const char* color, const char* word)
{
    printf("%s%s%s.\n",
            color ? color : "",
            word,
            color ? NORMAL : "");
    fflush(stdout);
}

struct test_outcome
{
    unsigned pass_count, fail_count, error_count;
};


#ifdef LIB211_HAS_POSIX
# ifndef MAP_ANON
#   ifdef MAP_ANONYMOUS
#     define MAP_ANON MAP_ANONYMOUS
#   else
#     define MAP_ANON 0
#   endif
# endif

static struct test_outcome
call_test_function(void (*test_fn)(void))
{

    struct test_outcome volatile* outcome;

    void* mmapped = mmap(NULL, sizeof *outcome,
                         PROT_READ|PROT_WRITE,
                         MAP_ANON|MAP_SHARED, -1, 0);
    if (mmapped == MAP_FAILED) goto os_error;

    outcome = mmapped;
    outcome->pass_count = outcome->fail_count = outcome->error_count = 0;

    pid_t pid = fork();
    if (pid < 0) goto os_error;

    if (pid == 0) {
        pass_count = fail_count = error_count = 0;

        test_fn();

        outcome->pass_count = pass_count;
        outcome->fail_count = fail_count;
        outcome->error_count = error_count;

        // Don't run our exit handler in here.
        tests_enabled = false;

        exit(0);
    }

    int status;
    int waitpid_res = waitpid(pid, &status, 0);
    struct test_outcome result = *outcome;

    int munmap_res = munmap(mmapped, sizeof *outcome);

    if (waitpid_res < 0 || munmap_res < 0) goto os_error;

    if (! WIFEXITED(status)) {
        ++result.error_count;
    }

    return result;

os_error:
    printf("\nunexpected error:\n");
    fflush(stdout);
    perror("RUN_TEST");
    exit(11);
}
#else // LIB211_HAS_POSIX
static struct test_outcome
call_test_function(void (*test_fn)(void))
{
    unsigned old_pass_count = pass_count,
             old_fail_count = fail_count,
             old_error_count = error_count;

    test_fn();

    return (struct test_outcome) {
        .pass_count = pass_count - old_pass_count,
        .fail_count = fail_count - old_fail_count,
        .error_count = error_count - old_error_count,
    };
}
#endif // LIB211_HAS_POSIX

bool lib211_do_run_test(
        void (*test_fn)(void),
        char const* source_expr,
        char const* file,
        int line)
{
    start_testing();

    bool const use_color = isatty(fileno(stdout));

    printf("%s... ", source_expr);
    fflush(stdout);

    struct test_outcome outcome = call_test_function(test_fn);

    if (outcome.error_count) {
        printf("\n%s ", source_expr);
        color_word(use_color ? RVRED : NULL, "errored");
        return false;
    } else if (outcome.fail_count) {
        printf("\n%s ", source_expr);
        color_word(use_color ? RED : NULL, "failed");
        return false;
    } else {
        color_word(use_color ? GREEN : NULL, "passed");
        return true;
    }
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
