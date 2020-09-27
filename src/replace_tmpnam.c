#define _XOPEN_SOURCE 700
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_LEN(A)      (sizeof (A) / sizeof (*(A)))
#define TEMPNAM_TEMPLATE  "%s/%s.XXXXXX"
#define TMPNAM_TEMPLATE   (P_tmpdir "/tmpnam.XXXXXX")

static_assert( sizeof TMPNAM_TEMPLATE < L_tmpnam,
               "This shim won't work here" );

static char*
mkstemp_close_unlink(char *buf, char const* who)
{
    int fd = mkstemp(buf);
    if (fd < 0) return NULL;

    if (close(fd) < 0) perror(who);
    if (unlink(buf) < 0) perror(who);

    return buf;
}

static char*
real_tmpnam(char* buf) {
    static char my_buf[L_tmpnam];
    if (!buf) buf = my_buf;

    strcpy(buf, TMPNAM_TEMPLATE);
    return mkstemp_close_unlink(buf, "tmpnam");
}

static char*
try_tempnam(char const* dir, char const* pfx)
{
    if (!dir) return NULL;
    if (!pfx) pfx = "tempnam";

    char dummy;
    size_t buf_size = snprintf(&dummy, sizeof dummy,
            TEMPNAM_TEMPLATE, dir, pfx);

    char* buf = malloc(buf_size);
    if (!buf) return NULL;

    snprintf(buf, buf_size, TEMPNAM_TEMPLATE, dir, pfx);
    return mkstemp_close_unlink(buf, "tempnam");
}

char*
tempnam(char const* dir, char const* pfx)
{
    char const* candidates[] = {
        dir,
        P_tmpdir,
        getenv("TMPDIR"),
        "/tmp",
        ".",
    };

    for (size_t i = 0; i < ARRAY_LEN(candidates); ++i) {
        char* result = try_tempnam(candidates[i], pfx);
        if (result) return result;
    }

    return NULL;
}

char*
tmpnam(char* buf)
{
    return real_tmpnam(buf);
}

char*
tmpnam_r(char* s)
{
    return s ? real_tmpnam(s) : NULL;
}
