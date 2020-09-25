#pragma once

#include <stddef.h>

// See malloc(3), calloc(3), realloc(3), reallocf(3), and free(3).
void* rt211_malloc(size_t size);
void* rt211_calloc(size_t count, size_t size);
void* rt211_realloc(void* ptr, size_t size);
void* rt211_reallocf(void* ptr, size_t size);
void rt211_free(void* ptr);

#ifndef LIB211_RAW_ALLOC
#  define malloc   rt211_malloc
#  define calloc   rt211_calloc
#  define realloc  rt211_realloc
#  define reallocf rt211_reallocf
#  define free     rt211_free
#endif

