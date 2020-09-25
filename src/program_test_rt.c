#define LIB211_RAW_ALLOC
#include "lib211.h"
#include "buffer.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
temp_template_template[] = "/tmp/check_command.%zu.XXXXXX";

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
    char tempfile[FD_COUNT][sizeof temp_template_template];
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
        snprintf(tempfile[i], sizeof tempfile[i],
                 temp_template_template, i);
    }

    FOR_FD(i) {
        fd[i] = mkstemp(tempfile[i]);
        if (fd[i] < 0) goto finish;
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
    res = wait(&status);
    if (res < 0) goto finish;

    if (WIFEXITED(status)) {
        int exit_status = WEXITSTATUS(status);
        if (exit_status == COULD_NOT_EXEC)
            eprintf("%s: could not execute\n", argv[0]);

        lib211_do_check_long(
                exit_status, spec->status,
                "actual exit code", "expected exit code",
                file, line);
    } else {
        int term_sig = WTERMSIG(status);
        eprintf("%s: exit with signal: %d\n", argv[0], term_sig);

        lib211_do_check_long(
                term_sig, 0,
                "actual termination signal", "expected termination signal",
                file, line);
    }

    if (spec->out) {
        char* actual_out = read_fd(fd[1]);
        if (!actual_out) goto finish;

        lib211_do_check_string(
                actual_out, spec->out,
                "actual stdout", "expected stdout",
                file, line);

        free(actual_out);
    }

    if (spec->err) {
        char* actual_err = read_fd(fd[2]);
        if (!actual_err) goto finish;

        lib211_do_check_string(
                actual_err, spec->err,
                "actual stderr", "expected stderr",
                file, line);

        free(actual_err);
    }

    // success == no one to blame
    whoami = NULL;

finish:
    free(argv);
    free(envv);

    FOR_FD(i) {
        if (fd[i] < 0) break;
        else close(fd[i]);
    }

    FOR_FD(i) {
        unlink(tempfile[i]);
    }

    if (whoami) perror(whoami);
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

