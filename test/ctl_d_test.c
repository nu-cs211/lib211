#include <211.h>

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s COUNT\n", argv[0]);
        exit(1);
    }

    long limit = atol(argv[1]);

    for (long i = 0; i < limit; ++i) {
        char* line = read_line();
        printf("%ld: %s\n", i, line ? line : "(null)");
        free(line);
    }
}
