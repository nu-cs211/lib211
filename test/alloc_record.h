#pragma once

#include <stddef.h>

typedef struct allocation
{
    void*  ptr;
    size_t size;
} rec_t;

void rec_init(rec_t[], size_t);
void malloc_success_fun(rec_t[], size_t, const char*, int);
void malloc_failure_fun(rec_t[], size_t, const char*, int);
void free_one(rec_t[], size_t);
void free_all(rec_t[]);

#define malloc_success(Recs, Size) \
    malloc_success_fun(Recs, Size, __FILE__, __LINE__)

#define malloc_failure(Recs, Size) \
    malloc_failure_fun(Recs, Size, __FILE__, __LINE__)
