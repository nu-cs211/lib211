#include <211.h>
#include <stdio.h>
#include <stdlib.h>

void alloc_test(void);
void fread_test(char const *filename);

void *alloc1(size_t n);
void  free1(void *p);
void  init1(int *p);
char *fread1(FILE *p);

void *alloc0(size_t n, unsigned i);
void  free0(void *p, unsigned i);
void  init0(int *p, unsigned i);
char *fread0(FILE *f, unsigned i);

int main(int argc, char *argv[])
{
    switch (argc) {
    case 1:
        alloc_test();
        break;

    case 2:
        fread_test(argv[1]);
        break;

    default:
        fprintf(stderr, "Usage: %s [INPUT_FILE]\n", argv[0]);
        return 3;
    }
}

void alloc_test(void)
{
    int *p = alloc1(sizeof *p);
    if (p == NULL) {
        perror("alloc_test");
        exit(1);
    }

    free1(p);

    // Write to dangling pointer:
    init1(p);
}

void fread_test(char const *filename)
{
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        perror("fread_test");
        exit(2);
    }

    // Leaks:
    while (fread1(f))
    { }

    fclose(f);
}

void *alloc1(size_t n)
{
    return alloc0(n, 4);
}

void free1(void *p)
{
    free0(p, 5);
}

void init1(int *p)
{
    init0(p, 6);
}

char *fread1(FILE *f)
{
    return fread0(f, 3);
}

void *alloc0(size_t n, unsigned i)
{
    if (i == 0)
        return malloc(n);
    else
        return alloc0(n, i - 1);
}

void free0(void *p, unsigned i) {
    if (i == 0)
        free(p);
    else
        free0(p, i - 1);
}

void init0(int *p, unsigned i)
{
    if (i == 0)
        *p = 10;
    else
        init0(p, i - 1);
}

char *fread0(FILE *f, unsigned i)
{
    if (i == 0)
        return fread_line(f);
    else
        return fread0(f, i - 1);
}
