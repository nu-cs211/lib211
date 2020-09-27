#ifndef _LIB211_ALLOC_H_
#define _LIB211_ALLOC_H_

#include <stdlib.h>

#undef malloc
#undef calloc
#undef realloc
#undef reallocf
#undef free
#undef read_line
#undef fread_line
#undef prompt_line

#ifndef LIB211_RAW_ALLOC
#  define malloc       rt211_malloc
#  define calloc       rt211_calloc
#  define realloc      rt211_realloc
#  define reallocf     rt211_reallocf
#  define free         rt211_free
#else
#  define read_line    read_line_raw_alloc
#  define fread_line   fread_line_raw_alloc
#  define prompt_line  prompt_line_raw_alloc
#endif

// See malloc(3), calloc(3), realloc(3), reallocf(3), and free(3).
void* malloc(size_t size);
void* calloc(size_t count, size_t size);
void* realloc(void* ptr, size_t size);
void* reallocf(void* ptr, size_t size);
void free(void* ptr);

#endif // _LIB211_ALLOC_H_
