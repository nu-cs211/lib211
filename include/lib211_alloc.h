#pragma once

#include <stddef.h>

#ifndef LIB211_RAW_ALLOC
#  define malloc   rt211_malloc
#  define calloc   rt211_calloc
#  define realloc  rt211_realloc
#  define reallocf rt211_reallocf
#  define free     rt211_free
#endif

void *reallocf(void *ptr, size_t size);
