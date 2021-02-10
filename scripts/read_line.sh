#!/bin/sh

input=${1:?need to specify read_line.inc}

cat <<'END'
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define READ_LINE_BUF_SIZE 80

#define surely_realloc  rt211_surely_realloc

#include "lib211_io.h"

END

cat "$input"

cat <<'END'
#undef surely_realloc

#undef _LIB211_ALLOC_H_
#undef _LIB211_IO_H_
#define LIB211_RAW_ALLOC
#include "lib211_io.h"

END

cat "$input"
