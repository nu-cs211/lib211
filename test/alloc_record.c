#include "alloc_record.h"

#include <211.h>

#include <assert.h>
#include <stdlib.h>

static size_t
find_record(rec_t recs[], size_t size);

void rec_init(rec_t recs[], size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        recs[i].size = 0;
        recs[i].ptr  = NULL;
    }
}

void
malloc_success_fun(rec_t recs[], size_t size, const char* file, int line)
{
    void* ptr = malloc(size);
    lib211_do_check(ptr, "", file, line);
    if (! ptr) return;

    size_t i = find_record(recs, 0);
    assert( recs[i + 1].size == 0 );

    recs[i].size = size;
    recs[i].ptr  = ptr;
}

void
malloc_failure_fun(rec_t recs[], size_t size, const char* file, int line)
{
    (void) recs;

    void* nothing = malloc(size);
    lib211_do_check_pointer(nothing, NULL, "", "", file, line);
    free(nothing);
}

void
free_one(rec_t recs[], size_t size)
{
    size_t i = find_record(recs, size);
    size_t j = find_record(recs, 0) - 1;
    free(recs[i].ptr);
    if (i != j) recs[i] = recs[j];
    recs[j].size = 0;
    recs[j].ptr  = NULL;
}

void
free_all(rec_t recs[])
{
    for (size_t i = 0; recs[i].size; ++i) {
        free(recs[i].ptr);
        recs[i].size = 0;
        recs[i].ptr  = NULL;
    }
}

static size_t
find_record(rec_t recs[], size_t size)
{
    size_t i = 0;
    while (recs[i].size != size) ++i;
    return i;
}
