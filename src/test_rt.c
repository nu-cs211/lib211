#define LIB211_KEEP_RUN_TEST_MACROS
#define LIB211_RAW_ALLOC
#define LIB211_RAW_EXIT

#define _XOPEN_SOURCE 700

#include "lib211_test.h"
#include "lib211_io.h"
#include "test_reporting.h"

#include <ctype.h>
#include <errno.h>
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

struct color_style
{
    int (*put_color)(char const*);
    int (*fput_color)(char const*, FILE*);
    int (*fcolor_word)(char const* color, char const* word, FILE*);
    char const* (*if_color)(char const* t, char const* f);
};

typedef struct color_style const* color_style_t;

static int
do_fputs(char const* data, FILE* stream)
{
    return fputs(data, stream);
}

static int
do_not_fputs(char const* data, FILE* stream)
{
    (void) data, (void) stream;
    return 0;
}

static int
do_puts(char const* data)
{
    return do_fputs(data, stdout);
}

static int
do_not_puts(char const* data)
{
    (void) data;
    return 0;
}

static int
do_fcolor_word(const char* color, const char* word, FILE* stream)
{
    return fprintf(stream, "%s%s%s", color, word, NORMAL);
}

static int
do_not_fcolor_word(const char* color, const char* word, FILE* stream)
{
    return fputs(word, stream);
}

static char const*
string_1_of_2(char const* a, char const* b)
{
    (void) b;
    return a;
}

static char const*
string_2_of_2(char const* a, char const* b)
{
    (void) a;
    return b;
}

static struct color_style const
color_color_style = {
    .if_color = string_1_of_2,
    .fput_color = do_fputs,
    .put_color = do_puts,
    .fcolor_word = do_fcolor_word,
};

static struct color_style const
plain_color_style = {
    .if_color = string_2_of_2,
    .fput_color = do_not_fputs,
    .put_color = do_not_puts,
    .fcolor_word = do_not_fcolor_word,
};

static color_style_t
stream_color_style(FILE* stream)
{
    if ( isatty(fileno(stream)) ) {
        return &color_color_style;
    } else {
        return &plain_color_style;
    }
}

static void print_test_results(void)
{
    char const* label_style = has_run_tests ? "test" : "check";
    unsigned check_count = pass_count + fail_count + error_count;
    FILE* fout = fail_count || error_count ? stderr : stdout;
    color_style_t color_style = stream_color_style(fout);

    fprintf(fout, "\n");

    if (! check_count) {
        fprintf(fout, "No checks.\n");
        return;
    }

    if (error_count) {
        color_style->fput_color(RVRED, fout);
        fprintf(fout,
                "*** %d %s%s could not be completed due to errors.",
                error_count,
                label_style,
                error_count == 1 ? "" : "s");
        color_style->fput_color(NORMAL, fout);
    }

    if (!error_count && !(pass_count && fail_count)) {
        const char* descr = pass_count
            ? color_style->if_color(GREEN "passed" NORMAL, "passed")
            : color_style->if_color(RED   "failed" NORMAL, "failed");

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
    if (log_check(have == want || (have && want && strcmp(have, want) == 0),
                  file, line))
        return true;

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


static void
print_run_test_outcome(
        color_style_t  color_style,
        const char*    color,
        const char*    word)
{
    color_style->fcolor_word(color, word, stdout);
    fputs(".\n", stdout);
    fflush(stdout);
}

static void
bad_error(char const* who)
{
    printf("\nunexpected error:\n");
    fflush(stdout);
    perror(who);
    exit(11);
}

typedef enum
{
    TS_PARENT_FAILURE = 0,
    TS_PARENT_SUCCESS = 1,
    TS_YOU_ARE_A_LITTLE_BABY,
} test_status_t;

static test_status_t
run_test_fork(char const* source_expr)
{
    start_testing();
    has_run_tests = true;

    color_style_t color_style = stream_color_style(stdout);

    printf("%s... ", source_expr);
    fflush(stdout);

    pid_t pid = fork();
    if (pid < 0) bad_error("RUN_TEST");

    if (pid == 0) {
        pass_count = fail_count = error_count = 0;
        return TS_YOU_ARE_A_LITTLE_BABY;
    }

    int status;
    int res = waitpid(pid, &status, 0);
    if (res < 0) bad_error("RUN_TEST");

    if (WIFEXITED(status)) {
        switch (WEXITSTATUS(status)) {
        case 0:
            print_run_test_outcome(color_style, GREEN, "passed");
            ++pass_count;
            return TS_PARENT_SUCCESS;

        case 1:
            printf("\n%s ", source_expr);
            print_run_test_outcome(color_style, RED, "failed");
            ++fail_count;
            return TS_PARENT_FAILURE;
        }
    }

    printf("\n%s ", source_expr);
    print_run_test_outcome(color_style, RVRED, "errored");
    ++error_count;
    return TS_PARENT_FAILURE;
}

_Noreturn static void
run_test_exit_child(void)
{
    // Don't run our exit handler in here.
    tests_enabled = false;

    if (error_count) exit(2);
    else if (fail_count) exit(1);
    else exit(0);
}


#define RUN_TEST_BODY(...)                                      \
    {                                                           \
        test_status_t status = run_test_fork(source_expr);      \
        if (status == TS_YOU_ARE_A_LITTLE_BABY) {               \
            test_fn(__VA_ARGS__);                               \
            run_test_exit_child();                              \
        }                                                       \
        return status == TS_PARENT_SUCCESS;                     \
    }

_NEXT_ARITY_0()
