#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib211_io.h"

#define READ_LINE_BUF_SIZE 80

static char*
xread_line(FILE* inf, char const* who)
{
    if (feof(inf)) return NULL;

    int c = getc(inf);
    if (c == EOF) return NULL;

    size_t capacity = READ_LINE_BUF_SIZE;
    size_t size     = 0;

    char* buffer    = realloc(NULL, capacity);
    if (buffer == NULL) {
        perror(who);
        exit(1);
    }

    for (;;) {
        if (c == EOF || c == '\n') {
            buffer[size] = '\0';
            return buffer;
        } else {
            buffer[size++] = (char) c;
        }

        c = getc(inf);

        if (size + 1 > capacity) {
            capacity *= 2;

            char* newbuf = realloc(buffer, capacity);
            if (newbuf == NULL) {
                free(buffer);
                perror(who);
                exit(1);
            }

            buffer = newbuf;
        }
    }
}

char* read_line(void)
{
    return xread_line(stdin, "read_line");
}

char* fread_line(FILE* inf)
{
    return xread_line(inf, "fread_line");
}

char* prompt_line(const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);

    fflush(stdout);
    return xread_line(stdin, "prompt_line");
}

/* vim: se ft=c: */
