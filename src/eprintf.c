#define LIB211_RAW_ALLOC
#define LIB211_RAW_EXIT

#include "lib211_io.h"

#include <stdarg.h>
#include <stdio.h>

void eprintf(const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);

    fflush(stderr);
}
