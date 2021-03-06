#define _XOPEN_SOURCE 700
#define LIB211_RAW_ALLOC
#define LIB211_RAW_EXIT

#include "211_alloc_limit.h"
#include "211.h"

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

#define EV_PEAK   "RT211_ALLOC_LIMIT_PEAK"
#define EV_TOTAL  "RT211_ALLOC_LIMIT_TOTAL"

// DEPRECATED:
#define EV_PEAK2  "RT211_HEAP_LIMIT"
#define EV_TOTAL2 "RT211_ALLOC_LIMIT"

///
/// TRACING
///

static bool trace_is_init = false;
static FILE* trace_out    = NULL;

static void
close_trace_out(void)
{
    if (trace_out) {
        fclose(trace_out);
        trace_out = NULL;
    }
}

static void
tracing_init_once(void)
{
    if (trace_is_init) return;

    atexit(&close_trace_out);

    const char* trace_dst = getenv("RT211_TRACE");
    if (! trace_dst) {
        // all set
    } else if (trace_dst[0] == '&' && trace_dst[1] != 0) {
        char* endptr;
        long fd = strtol(&trace_dst[1], &endptr, 10);
        if (*endptr == 0 && 0 <= fd && fd <= (long)INT_MAX) {
            trace_out = fdopen((int)fd, "w");
        }
    } else {
        trace_out = fopen(trace_dst, "w");
    }

    trace_is_init = true;
}

static bool
alloc_trace_is_enabled(void)
{
    tracing_init_once();
    return trace_out != NULL;
}

static void
alloc_tracef(char const* format, ...)
{
    if (!alloc_trace_is_enabled()) return;

    va_list ap;
    va_start(ap, format);
    vfprintf(trace_out, format, ap);
    va_end(ap);
    fprintf(trace_out, "\n");
}


///
/// ALLOCATION INSTRUMENTATION
///

// A linked list mapping pointers to allocation sizes.
typedef struct alloc_record
{
    void*  pointer;
    size_t size;
    struct alloc_record* next;
}       *alloc_list_t;

// The state of the allocation limit system:
static enum {
    UNINITIALIZED,
    NO_LIMIT,
    LIMIT_TOTAL, // limit total bytes allocated ever (free irrelevant)
    LIMIT_PEAK   // limit total bytes allocated at once (free helps)
}       alloc_limit_state = UNINITIALIZED;

// Remaining bytes allowed to allocate. If the state is LIMIT_PEAK then
// free() adds to this, whereas with LIMIT_TOTAL this number is monotone
// decreasing (unless you reset it explicitly).
static size_t bytes_remaining;

// A map from every allocated pointer to its size.
static alloc_list_t allocation_list = NULL;

static noreturn void
bad_env_var(char const* name, char const* value)
{
    fprintf(stderr, "rt211_alloc: could not understand %s value: ‘%s’",
            name, value);
    exit(254);
}

static bool
get_limit(char const* const name, size_t* const out)
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

static void
alloc_limit_init_once(void)
{
    size_t n;

    if (get_limit(EV_TOTAL, &n) || get_limit(EV_TOTAL2, &n))
        alloc_limit_set_total(n);

    else if (get_limit(EV_PEAK, &n) || get_limit(EV_PEAK2, &n))
        alloc_limit_set_peak(n);

    else
        alloc_limit_set_no_limit();
}

#define ENSURE_ALLOC_DEBUG_INIT() \
    if (alloc_limit_state == UNINITIALIZED) alloc_limit_init_once()

static alloc_list_t
find_alloc_record(void* p)
{
    for (alloc_list_t cur = allocation_list; cur; cur = cur->next) {
        if (cur->pointer == p) {
            return cur;
        }
    }

    return NULL;
}

static size_t
lookup_and_forget_size(void* p)
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

static void forget_everything(void)
{
    alloc_list_t list = allocation_list;
    allocation_list = NULL;

    while (list) {
        alloc_list_t victim = list;
        list = list->next;
        free(victim);
    }
}

static void remember_allocation(void* p, size_t n)
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

static void forget_allocation(void* p)
{
    bytes_remaining += lookup_and_forget_size(p);
}

static bool alloc_limit_may_alloc(size_t n)
{
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

static void* alloc_limit_did_alloc(void* p, size_t n)
{
    if (!p) return NULL;

    if (alloc_limit_state == LIMIT_PEAK)
        remember_allocation(p, n);

    if (alloc_limit_state == LIMIT_PEAK ||
            alloc_limit_state == LIMIT_TOTAL)
        bytes_remaining -= n;

    return p;
}

static void alloc_limit_will_free(void* p)
{
    if (!p) return;

    if (alloc_limit_state == LIMIT_PEAK)
        forget_allocation(p);
}


///
/// WRAPPERS FOR MALLOC/FREE API
///

static inline void*
realloc_with_total_limit(void *ptr, size_t new_size)
{
    if (alloc_limit_may_alloc(new_size))
        return alloc_limit_did_alloc(realloc(ptr, new_size), new_size);
    else
        return NULL;
}

static inline void*
realloc_with_peak_limit(void *ptr, size_t new_size)
{
    alloc_list_t node = find_alloc_record(ptr);
    size_t old_size   = node ? node->size : 0;

    size_t needed = new_size > old_size ? new_size - old_size : 0;
    if (!alloc_limit_may_alloc(needed))
        return NULL;

    ptr = realloc(ptr, new_size);
    if (!ptr)
        return NULL;

    // Unsigned arithmetic, so this works even when new_size < old_size:
    size_t change_in_size = new_size - old_size;

    bytes_remaining -= change_in_size;
    if (node) {
        node->size  += change_in_size;
    } else {
        remember_allocation(ptr, new_size);
    }

    return ptr;
}

#define DO_CALLOC(NMEMB, SIZE) \
    (SIZE <= SIZE_MAX / NMEMB && \
         alloc_limit_may_alloc(NMEMB * SIZE) \
     ? alloc_limit_did_alloc(calloc(NMEMB, SIZE), NMEMB * SIZE) \
     : NULL)

#define DO_MALLOC(SIZE) \
    (alloc_limit_may_alloc(SIZE) \
     ? alloc_limit_did_alloc(malloc(SIZE), SIZE) \
     : NULL)

#define DO_FREE(PTR) \
     (alloc_limit_will_free(PTR), free(PTR))

#define DO_REALLOC(PTR, NEW_SIZE) \
    (!PTR \
     ? DO_MALLOC(NEW_SIZE) \
     : alloc_limit_state == NO_LIMIT \
     ? realloc(PTR, NEW_SIZE) \
     : alloc_limit_state == LIMIT_TOTAL \
     ? realloc_with_total_limit(PTR, NEW_SIZE) \
     : alloc_limit_state == LIMIT_PEAK \
     ? realloc_with_peak_limit(PTR, NEW_SIZE) \
     : NULL)


/////
///// PUBLIC API FUNCTIONS
/////


///
/// TRACING WRAPPERS
///

void* rt211_calloc(size_t nmemb, size_t size)
{
    ENSURE_ALLOC_DEBUG_INIT();
    alloc_tracef("calloc(%zu, %zu)", nmemb, size);

    return DO_CALLOC(nmemb, size);
}

void* rt211_malloc(size_t size)
{
    ENSURE_ALLOC_DEBUG_INIT();
    alloc_tracef("malloc(%zu)", size);

    return DO_MALLOC(size);
}

void rt211_free(void *ptr)
{
    ENSURE_ALLOC_DEBUG_INIT();
    alloc_tracef("free(%p)", ptr);

    DO_FREE(ptr);
}

void* rt211_realloc(void *ptr, size_t size)
{
    ENSURE_ALLOC_DEBUG_INIT();
    alloc_tracef("realloc(%p, %zu)", ptr, size);

    return DO_REALLOC(ptr, size);
}

void* rt211_reallocf(void *ptr, size_t size)
{
    ENSURE_ALLOC_DEBUG_INIT();
    alloc_tracef("reallocf(%p, %zu)", ptr, size);

    void* result = DO_REALLOC(ptr, size);
    if (!result) DO_FREE(ptr);
    return result;
}


///
/// SIMULATING ALLOCATION FAILURE
///

void alloc_limit_set_no_limit(void)
{
    alloc_limit_state = NO_LIMIT;
    forget_everything();
    bytes_remaining = 0;
}

void alloc_limit_set_total(size_t n)
{
    alloc_limit_state = LIMIT_TOTAL;
    forget_everything();
    bytes_remaining = n;
}

void alloc_limit_set_peak(size_t n)
{
    alloc_limit_state = LIMIT_PEAK;
    forget_everything();
    bytes_remaining = n;
}
