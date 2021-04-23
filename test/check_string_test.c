#define _XOPEN_SOURCE 700

#include <211.h>

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NON_NEG(E) non_neg_rt((E), #E, __FILE__, __LINE__)

static int non_neg_rt(int val, char const* expr, char const* file, int line)
{
    if (val >= 0) return val;

    eprintf("Error: %s returned %d at %s:%d\n", expr, val, file, line);
    perror("  reason");
    _exit(255);
}

static void do_test(char const* a, char const* b)
{
    pid_t pid = NON_NEG( fork() );

    // child
    if (pid == 0) {
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);

        CHECK_STRING(a, b);
        CHECK_STRING(a, b);

        exit(0);
    }

    // parent
    int status;
    NON_NEG( waitpid(pid, &status, 0) );

    CHECK( WIFEXITED(status) );
    if (a == b || (a && b && !strcmp(a, b)))
        CHECK_INT( WEXITSTATUS(status), 0 );
    else
        CHECK_INT( WEXITSTATUS(status), 2 );
}

int main(int argc, char* argv[])
{
    start_testing();
    do_test("A", "A");
    do_test("A", "B");
    do_test(NULL, "B");
    do_test("A", NULL);
    do_test(NULL, NULL);
}
