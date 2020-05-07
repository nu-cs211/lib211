#define _XOPEN_SOURCE 700
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

FILE* trace_out = NULL;
static char const* rt211_trace = NULL;

static void close_trace_out(void)
{
    if (trace_out) {
        fclose(trace_out);
        trace_out = NULL;
    }
}

static void tracing_init_once(void)
{
    atexit(&close_trace_out);

    rt211_trace = getenv("RT211_TRACE");
    if (!rt211_trace) {
        rt211_trace = "";
    } else if (rt211_trace[0] == '&' && rt211_trace[1] != 0) {
        char* endptr;
        long fd = strtol(&rt211_trace[1], &endptr, 10);
        if (*endptr == 0 && 0 <= fd && fd <= (long)INT_MAX) {
            trace_out = fdopen((int)fd, "w");
        }
    } else {
        trace_out = fopen(rt211_trace, "w");
    }
}

static bool trace_enabled(void)
{
    if (!rt211_trace) tracing_init_once();
    return trace_out != NULL;
}

static void tracef(char const* format, ...)
{
    if (!trace_enabled()) return;

    va_list ap;
    va_start(ap, format);
    vfprintf(trace_out, format, ap);
    va_end(ap);
    fprintf(trace_out, "\n");
}

void* rt211_calloc(size_t nmemb, size_t size)
{
    tracef("calloc(%zu, %zu)", nmemb, size);
    return calloc(nmemb, size);
}

/*
static void
waspfill(size_t n, char mem[n], char const* pat)
{
    if (!n) return;

    size_t patlen = strlen(pat);

    for (size_t i = 0; i + 1 < n; ++i) {
        mem[i] = pat[i % patlen];
    }

    mem[n - 1] = 0;
}
*/

void* rt211_malloc(size_t size)
{
    tracef("malloc(%zu)", size);
    return malloc(size);
}

void rt211_free(void *ptr)
{
    free(ptr);
    tracef("free(%p)", ptr);
}

void* rt211_realloc(void *ptr, size_t size)
{
    void* result = realloc(ptr, size);
    tracef("realloc(%p, %zu) => %p", ptr, size, result);
    return result;
}

void* rt211_reallocf(void *ptr, size_t size)
{
    tracef("reallocf(%p, %zu)", ptr, size);
    void* result = realloc(ptr, size);
    if (!result) free(ptr);
    return result;
}
