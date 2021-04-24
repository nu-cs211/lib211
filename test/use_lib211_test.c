#define _GNU_SOURCE
#include <211.h>

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>


/// Performs `count` failing checks and 2 passing checks.
static void
some_failing_checks(size_t count);

/// Runs this same program several times with different arguments (to
/// produce failing checks) and then checks the results.
static void
check_myself(void);

/// Runs `argv0` (this program) passing it `n` (stringified) as
/// `argv[1]`, especting `msg_fmt` (suitably interpolated) to be written
/// to `stderr`, and / an exit code of `n`
static void
check_n_failures(int n, char const* msg_fmt);

/// Like GNUs `asprintf`, but returns the string or aborts on failure.
static char*
xasprintf(char const* fmt, ...);

/// Will hold `argv[0]`.
static char const* argv0;

int
main(int argc, char* argv[])
{
    argv0 = argv[0];

    if (argc == 2) {
        some_failing_checks(atoi(argv[1]));
    } else {
        check_myself();
    }
}


static void
some_failing_checks(size_t count)
{
    CHECK( 1 );                 // pass

    size_t n = count / 4;

    // Duff's device
    switch (count % 4) {
    case 0:
        while (n--) {
            CHECK_CHAR( 'A', 'B' );
#           define CHAR_CHAR_MSG CHECK_FAILED_MSG(57, 'A', 'A', 'B', 'B')
        case 3:
            CHECK_INT( 'A', 'B' );
#           define INT_CHAR_MSG CHECK_FAILED_MSG(60, 65, 'A', 66, 'B')
        case 2:
            CHECK_CHAR( 65, 66 );
#           define CHAR_INT_MSG CHECK_FAILED_MSG(63, 'A', 65, 'B', 66)
        case 1:
            CHECK_INT( -3, -5 );
#           define INT_INT_MSG CHECK_FAILED_MSG(66, -3, -3, -5, -5)
        }
    }

    CHECK_POINTER( "a", "a" );  // pass
}


#define CHECK_FAILED_MSG(LINE, HAVE, HAVE_SRC, WANT, WANT_SRC)  \
    "\n"                                                        \
    "Check failed (use_lib211_test.c:" #LINE "):\n"             \
    "  have: " #HAVE "  (from: " #HAVE_SRC ")\n"                \
    "  want: " #WANT "  (from: " #WANT_SRC ")\n"
#define N_OF_M_PASSED_FMT       "\n%u of %u checks passed.\n"

static void
check_3_failures(void)
{
    check_n_failures(3,
            INT_CHAR_MSG CHAR_INT_MSG INT_INT_MSG
            N_OF_M_PASSED_FMT);
}

static void
check_4_failures(void)
{
    check_n_failures(4,
            CHAR_CHAR_MSG INT_CHAR_MSG CHAR_INT_MSG INT_INT_MSG
            N_OF_M_PASSED_FMT);
}

static void
check_5_failures(void)
{
    check_n_failures(5,
            INT_INT_MSG CHAR_CHAR_MSG INT_CHAR_MSG CHAR_INT_MSG INT_INT_MSG
            N_OF_M_PASSED_FMT);
}

static void
check_myself(void)
{
    RUN_TEST( check_3_failures );
    RUN_TEST( check_4_failures );
    RUN_TEST( check_5_failures );
}


static void
check_n_failures(int n, char const* msg_fmt)
{
    char *n_str = xasprintf("%d", n),
         *msg   = xasprintf(msg_fmt, 2, 2 + n);

    char const *argv[] = {argv0, n_str, 0};
    CHECK_EXEC(argv, "", "", msg, n);

    free(msg);
    free(n_str);
}


static char*
xvasprintf(char const* fmt, va_list ap);

static char*
xasprintf(char const* fmt, ...)
{
    char* result;

    va_list ap;
    va_start(ap, fmt);
    result = xvasprintf(fmt, ap);
    va_end(ap);

    return result;
}

static char*
xvasprintf(char const* fmt, va_list ap)
{
    char* result;

    int ok = vasprintf(&result, fmt, ap);
    if (ok == -1) {
        perror("xvasprintf");
        exit(1);
    }

    return result;
}

