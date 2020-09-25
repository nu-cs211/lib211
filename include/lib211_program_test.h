#pragma once

struct proc_spec
{
    int status;
    char const *in, *out, *err;
};

typedef struct proc_spec PROC_SPEC;

#define SPEC_BLANK  ((struct proc_spec) {0, "", "", ""})

#define CHECK_COMMAND(cmd, spec) \
    lib211_do_check_command(__FILE__, __LINE__, cmd, spec)

#define CHECK_EXEC(argc, argv, spec) \
    lib211_do_check_exec(__FILE__, __LINE__, argc, argv, NULL, spec)

#define CHECK_EXEC_ENV(argc, argv, env, spec) \
    lib211_do_check_exec(__FILE__, __LINE__, argc, argv, env, spec)

void lib211_do_check_command(
        char const             *file,
        int                     line,
        char const             *command,
        struct proc_spec const *spec);

void lib211_do_check_exec(
        char const             *file,
        int                     line,
        int                     argc,
        char const             *argv[],
        char const             *env1,
        struct proc_spec const *spec);
