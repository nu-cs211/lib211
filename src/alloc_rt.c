#define _XOPEN_SOURCE 700
#define LIB211_RAW_ALLOC
#define LIB211_RAW_EXIT

#include "lib211_alloc_limit.h"
#include "lib211.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <unistd.h>

// A linked list mapping pointers to allocation sizes.
struct alloc_record
{
    void*  pointer;
    size_t size;
    struct alloc_record* next;
};
typedef struct alloc_record* alloc_list_t;

// A map from every allocated pointer to its size.
static alloc_list_t allocation_list = NULL;

// The state of the allocation limit system:
static enum {
    UNINITIALIZED,
    NO_LIMIT,
    LIMIT_TOTAL, // limit total bytes allocated ever (free irrelevant)
    LIMIT_PEAK   // limit total bytes allocated at once (free helps)
} alloc_limit_state = UNINITIALIZED;

// Remaining bytes allowed to allocate. If the state is LIMIT_PEAK then
// free() adds to this, whereas with LIMIT_TOTAL this number is monotone
// decreasing (unless you reset it explicitly).
static size_t bytes_remaining;

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

static void alloc_tracef(char const* format, ...)
{
    if (!trace_enabled()) return;

    va_list ap;
    va_start(ap, format);
    vfprintf(trace_out, format, ap);
    va_end(ap);
    fprintf(trace_out, "\n");
}

void alloc_limit_set_no_limit(void)
{
    alloc_limit_state = NO_LIMIT;
}

void alloc_limit_set_total(size_t n)
{
    alloc_limit_state = LIMIT_TOTAL;
    bytes_remaining = n;
}

void alloc_limit_set_peak(size_t n)
{
    alloc_limit_state = LIMIT_PEAK;
    bytes_remaining = n;
}

static noreturn void
bad_env_var(char const* name, char const* value)
{
    fprintf(stderr, "rt211_alloc: could not understand %s value: ‘%s’",
            name, value);
    exit(254);
}

static bool get_limit_from_env(char const* const name,
                               size_t* const out)
{
    char *const original = getenv(name),
         *begin = original,
         *end;
    unsigned long size;

    if (!begin) return false;

    while (isspace(*begin)) ++begin;
    if (!*begin) return false;

    size = strtoul(begin, &end, 10);
    if (begin == end)
        bad_env_var(name, original);

    while (isspace(*end)) ++end;

    switch (*end) {
    case 'B': case 'b': case 0:
        *out = size;
        return true;

    case 'K': case 'k':
        *out = size << 10;
        return true;

    case 'M': case 'm':
        *out = size << 20;
        return true;

    case 'G': case 'g':
        *out = size << 30;
        return true;

    default:
        bad_env_var(name, original);
    }
}

static void alloc_debug_init_once(void)
{
    if (get_limit_from_env("RT211_ALLOC_LIMIT", &bytes_remaining))
        alloc_limit_state = LIMIT_TOTAL;
    else if (get_limit_from_env("RT211_HEAP_LIMIT", &bytes_remaining))
        alloc_limit_state = LIMIT_PEAK;
    else
        alloc_limit_state = NO_LIMIT;
}

#define ENSURE_ALLOC_DEBUG_INIT() \
    if (alloc_limit_state == UNINITIALIZED) alloc_debug_init_once()

static void alloc_debug_gave_back(size_t size)
{
    if (alloc_limit_state == LIMIT_PEAK)
        bytes_remaining += size;
}

static void adjust_record(alloc_list_t node, void* new_pointer,
                          size_t old_size, size_t new_size)
{
    node->pointer = new_pointer;

    if (new_size > old_size) {
        bytes_remaining -= new_size - old_size;
        node->size += new_size - old_size;
    } else if (new_size < old_size) {
        alloc_debug_gave_back(old_size - new_size);
        node->size -= old_size - new_size;
    }
}

static void remember_size(void* p, size_t n)
{
    alloc_list_t node = malloc(sizeof *node);
    if (!node) {
        perror("lib211_alloc");
        exit(255);
    }

    node->pointer = p;
    node->size = n;
    node->next = allocation_list;
    allocation_list = node;
}

static alloc_list_t* find_alloc_record(void* p)
{
    ENSURE_ALLOC_DEBUG_INIT();

    for (alloc_list_t* cur = &allocation_list; *cur; cur = &(*cur)->next) {
        if ((*cur)->pointer == p) {
            return cur;
        }
    }

    return NULL;
}

static size_t lookup_and_forget_size(void* p)
{
    size_t size = 0;

    for (alloc_list_t* cur = &allocation_list; *cur; cur = &(*cur)->next) {
        if ((*cur)->pointer == p) {
            alloc_list_t victim = *cur;
            *cur = victim->next;
            size = victim->size;
            free(victim);
            break;
        }
    }

    return size;
}

static bool alloc_debug_may_alloc(size_t n)
{
    ENSURE_ALLOC_DEBUG_INIT();

    if ((alloc_limit_state == LIMIT_TOTAL || alloc_limit_state == LIMIT_PEAK)
            && n > bytes_remaining)
    {
        alloc_tracef(
                "lib211_alloc: preventing allocation of %zu bytes "
                "because\nremaining limit is %zu",
                n, bytes_remaining);
        errno = ENOMEM;
        return false;
    }

    return true;
}


static void* alloc_debug_did_alloc(void* p, size_t n)
{
    if (!p) return NULL;

    if (alloc_limit_state == LIMIT_TOTAL || alloc_limit_state == LIMIT_PEAK)
        bytes_remaining -= n;

    remember_size(p, n);

    return p;
}

static void alloc_debug_will_free(void* p)
{
    if (!p) return;

    ENSURE_ALLOC_DEBUG_INIT();

    size_t size = lookup_and_forget_size(p);
    alloc_debug_gave_back(size);
}

void* rt211_calloc(size_t nmemb, size_t size)
{
    alloc_tracef("calloc(%zu, %zu)", nmemb, size);

    if (size <= SIZE_MAX / nmemb &&
            alloc_debug_may_alloc(nmemb * size))
        return alloc_debug_did_alloc(calloc(nmemb, size), nmemb * size);
    else
        return NULL;
}

static void* rt211_malloc_quiet(size_t size)
{
    if (alloc_debug_may_alloc(size))
        return alloc_debug_did_alloc(malloc(size), size);
    else
        return NULL;
}

void* rt211_malloc(size_t size)
{
    alloc_tracef("malloc(%zu)", size);
    return rt211_malloc_quiet(size);
}

void rt211_free(void *ptr)
{
    alloc_tracef("free(%p)", ptr);

    alloc_debug_will_free(ptr);
    free(ptr);
}

void* rt211_realloc(void *ptr, size_t new_size)
{
    alloc_tracef("realloc(%p, %zu)", ptr, new_size);

    if (!ptr) return rt211_malloc_quiet(new_size);

    alloc_list_t* nodep = find_alloc_record(ptr);

    // No record! What to do? Just be realloc.
    if (!nodep) return realloc(ptr, new_size);

    size_t old_size = (*nodep)->size;
    size_t needed = new_size > old_size ? new_size - old_size : 0;
    if (!alloc_debug_may_alloc(needed)) goto failure;

    void* new_ptr = realloc(ptr, new_size);
    if (!new_ptr) goto failure;

    adjust_record(*nodep, new_ptr, old_size, new_size);
    return new_ptr;

failure:
    return NULL;
}

void* rt211_reallocf(void *ptr, size_t new_size)
{
    alloc_tracef("reallocf(%p, %zu)", ptr, new_size);

    if (!ptr) return rt211_malloc_quiet(new_size);

    alloc_list_t* nodep = find_alloc_record(ptr);

    // No record! What to do? Just be reallocf.
    if (!nodep) {
        void* new_ptr = realloc(ptr, new_size);
        if (!new_ptr) free(ptr);
        return new_ptr;
    }

    size_t old_size = (*nodep)->size;
    size_t needed = new_size > old_size ? new_size - old_size : 0;
    if (!alloc_debug_may_alloc(needed)) goto failure;

    void* new_ptr = realloc(ptr, new_size);
    if (!new_ptr) goto failure;

    adjust_record(*nodep, new_ptr, old_size, new_size);
    return new_ptr;

failure:
    alloc_debug_will_free(ptr);
    free(ptr);
    return NULL;
}
