#define _XOPEN_SOURCE 700
#define LIB211_RAW_ALLOC
#define LIB211_RAW_EXIT

#include "lib211.h"
#include "buffer.h"
#include "test_reporting.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#define FD_COUNT          4
#define FOR_FD(I)         for (size_t I = 0; I < FD_COUNT; ++I)
#define COULD_NOT_EXEC    252
#define READ_FD_BUF_LEN   256
#define ARRAY_LEN(A)      (sizeof (A) / sizeof *(A))

struct proc_spec null_proc(void)
{
    return (struct proc_spec) { 0, "", "", "" };
}

static char const
temp_template[] = "/tmp/lib211_check_exec.XXXXXX";

static char* read_fd(int fd)
{
    if (lseek(fd, 0, SEEK_SET) < 0)
        goto could_not_lseek;

    struct buffer buf;
    if (!balloc(&buf, READ_FD_BUF_LEN))
        goto could_not_malloc;

    FILE* fstream = fdopen(dup(fd), "r");
    if (!fstream)
        goto could_not_fdopen;

    while(bfread(&buf, 1, fstream))
    { }

    if (!feof(fstream))
        goto could_not_fread;

    fclose(fstream);
    buf.data[buf.fill] = 0;
    return buf.data;

could_not_fread:
    fclose(fstream);

could_not_fdopen:
    free(buf.data);

could_not_malloc:
could_not_lseek:
    perror(NULL);
    return NULL;
}

static int write_range(
        int fd,
        char const* begin,
        char const* end)
{
    while (begin < end) {
        int res = write(fd, begin, end - begin);
        if (res < 0) {
            if (errno != EINTR) return res;
        } else {
            begin += res;
        }
    }

    return 0;
}

static int write_string(int fd, char const* str)
{
    return write_range(fd, str, str + strlen(str));
}

static size_t env_len(char const** begin)
{
    char const** end = begin;
    while (*end) ++end;
    return end - begin;
}

static char const** grow_clone(char const* const* src_ptr,
                               size_t old_size,
                               size_t front_growth,
                               size_t back_growth)
{
    size_t new_size = old_size + front_growth + back_growth;
    char const** dst_ptr = malloc(new_size * sizeof dst_ptr[0]);

    if (dst_ptr)
        memcpy(dst_ptr + front_growth, src_ptr, old_size * sizeof dst_ptr[0]);

    return dst_ptr;
}

static int fput_cstrlit(FILE* fout, const char* str);

static void check_output_string(
        const char* file, int line, const char* descr,
        const char* expected, int fd, bool* saw_error)
{
    if (!expected) return;

    char* out = read_fd(fd);
    if (!out) {
        perror(NULL);
        return;
    }

    if (strcmp(out, expected)) {
        if (*saw_error) {
            eprintf("Additional check failure:\n");
        } else {
            rt211_test_log_check(false, file, line);
            *saw_error = true;
        }

        eprintf("  reason: mismatch in %s\n", descr);

        eprintf("  have: ");
        fput_cstrlit(stderr, out);
        eprintf("\n");

        eprintf("  want: ");
        fput_cstrlit(stderr, expected);
        eprintf("\n");
    }

    free(out);
}

static void eprintf_argv(char const *const* argv)
{
    eprintf("  argv[]: {\n");

    while (*argv) {
        eprintf("    ");
        fput_cstrlit(stderr, *argv);
        if (argv + 1) eprintf(",");
        ++argv;
    }

    eprintf("  }\n");
}
static void do_check_exec(
        char const* whoami,
        char const* file,
        int line,
        int argc,
        char const* orig_argv[],
        char const* env1,
        struct proc_spec const* spec)
{
    extern char const **environ;

    size_t envc = env_len(environ);
    int fd[FD_COUNT] = {-1};
    int res;

    char const** argv = grow_clone(orig_argv, argc, 1, 1);
    char const** envv = grow_clone(environ, envc, 0, 2);

    if (!argv || !envv) goto finish;

    argv[0] = "/usr/bin/env";
    argc++;
    argv[argc++] = NULL;

    envv[envc++] = env1;
    envv[envc++] = NULL;

    FOR_FD(i) {
        char tempfile[sizeof temp_template];
        memcpy(tempfile, temp_template, sizeof tempfile);

        fd[i] = mkstemp(tempfile);
        if (fd[i] < 0) goto finish;

        unlink(tempfile);
    }

    if (spec->in) {
        res = write_string(fd[0], spec->in);
        if (res < 0) goto finish;

        res = lseek(fd[0], 0, SEEK_SET);
        if (res < 0) goto finish;
    }

    pid_t pid = fork();
    if (pid < 0) goto finish;

    if (pid == 0) {
        FOR_FD(i) {
            dup2(fd[i], i);
            close(fd[i]);
        }

        execve(argv[0], (char**)argv, (char**)envv);

        dup2(3, 2);
        eprintf("%d %s\n", errno, strerror(errno));
        exit(COULD_NOT_EXEC);
    }

    // Parent:

    int status;
    res = waitpid(pid, &status, 0);
    if (res < 0) goto finish;

    if (WIFEXITED(status) && WEXITSTATUS(status) == COULD_NOT_EXEC) {
        rt211_test_report_error(file, line);
        char* msg = read_fd(fd[3]);
        eprintf("%s: %s\n", argv[0], msg? msg : "could not exec");
        free(msg);
        whoami = NULL;
        goto finish;
    }

    bool saw_error = false;

    if (WIFSIGNALED(status)) {
        rt211_test_report_error(file, line);
        saw_error = true;
        eprintf("  reason: process killed by signal %d\n", WTERMSIG(status));
        eprintf_argv(argv + 1);
    }

    check_output_string(file, line, "stdout",
            spec->out, fd[1], &saw_error);
    check_output_string(file, line, "stderr",
            spec->err, fd[2], &saw_error);

    if (WEXITSTATUS(status) != spec->status) {
        if (saw_error) {
            eprintf("Additional check failure:\n");
        } else {
            rt211_test_log_check(false, file, line);
            saw_error = true;
        }

        eprintf("  reason: exit code mismatch\n");
        eprintf("  have: %d\n", WEXITSTATUS(status));
        eprintf("  want: %d\n", spec->status);
    }

    if (!saw_error) {
        rt211_test_log_check(true, file, line);
        whoami = NULL; // success == no one to blame
    }

finish:
    if (whoami) perror(whoami);

    free(argv);
    free(envv);

    FOR_FD(i) {
        if (fd[i] < 0) break;
        else close(fd[i]);
    }

}

void lib211_do_check_exec(
        char const             *file,
        int                     line,
        int                     argc,
        char const             *argv[],
        char const             *env1,
        struct proc_spec const *spec)
{
    do_check_exec("check_exec", file, line, argc, argv, env1, spec);
}

void lib211_do_check_command(
        char const             *file,
        int                     line,
        char const             *command,
        struct proc_spec const *spec)
{
    char const* argv[] = {"/bin/sh", "-c", command};
    do_check_exec("check_command", file, line,
            ARRAY_LEN(argv), argv, NULL, spec);
}

#define PUT(C) \
    do { \
        if (fputc(C, fout) < 0) return EOF; \
        else ++count; \
    } while (false)

static int fput_cstrlit(FILE* fout, const char* str)
{
    size_t count = 0;

    PUT('"');

    char c;

    while ( (c = *str++) ) {
        switch (c) {
        case '\\': case '\"':
            PUT('\\'); PUT(c); break;
        case '\a':
            PUT('\\'); PUT('a'); break;
        case '\b':
            PUT('\\'); PUT('b'); break;
        case '\f':
            PUT('\\'); PUT('f'); break;
        case '\n':
            PUT('\\'); PUT('n'); break;
        case '\r':
            PUT('\\'); PUT('r'); break;
        case '\t':
            PUT('\\'); PUT('t'); break;
        case '\v':
            PUT('\\'); PUT('v'); break;
        default:
            if (isgraph(c) || c == ' ') {
                PUT(c);
            } else {
                int res = fprintf(fout, "\\x%02x", c);
                if (res < 0) return EOF;
                else count += res;
            }
        }
    }

    PUT('"');

    return count;
}
