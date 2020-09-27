#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define READ_LINE_BUF_SIZE 80

#define surely_realloc  rt211_surely_realloc
#include "read_line.inc"
#undef surely_realloc

#undef _LIB211_ALLOC_H_
#undef _LIB211_IO_H_
#define LIB211_RAW_ALLOC
#include "read_line.inc"
