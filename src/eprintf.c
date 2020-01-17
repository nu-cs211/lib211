#include "lib211_io.h"

#include <stdarg.h>
#include <stdio.h>

void eprintf(const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    fflush(stderr);
}
