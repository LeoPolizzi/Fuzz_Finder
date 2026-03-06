#include "executor.h"

#include <fcntl.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>

static void apply_resource_limits( )
{
    struct rlimit as_limit;
    as_limit.rlim_cur = as_limit.rlim_max = 64 * 1024 * 1024;  // 64 MiB address space
    setrlimit(RLIMIT_AS, &as_limit);

    struct rlimit cpu_limit;
    cpu_limit.rlim_cur = cpu_limit.rlim_max = 2;  // 2 seconds CPU
    setrlimit(RLIMIT_CPU, &cpu_limit);
}

static char** build_exec_argv(const std::string& program_path, const std::vector<std::string>& args)
{
    size_t count = 1 + args.size( );
    char** argv = (char**)calloc(count + 1, sizeof(char*));
    argv[0] = strdup(program_path.c_str( ));
    for (size_t i = 0; i < args.size( ); ++i) {
        argv[i + 1] = strdup(args[i].c_str( ));
    }
    argv[count] = nullptr;
    return (argv);
}

ExecResult Executor::run_with_input(const std::string& program_path, const std::vector<std::string>& args, const std::string& stdin_data)
{
    int stdin_pipe[2];
    if (pipe(stdin_pipe) != 0) {
        perror("pipe");
    }
    pid_t child_pid = fork( );
    if (child_pid == 0) {
        dup2(stdin_pipe[0], STDIN_FILENO);
        close(stdin_pipe[1]);
        apply_resource_limits( );
        int dev_null_fd = open("/dev/null", O_WRONLY);
        if (dev_null_fd >= 0) {
            dup2(dev_null_fd, STDOUT_FILENO);
            dup2(dev_null_fd, STDERR_FILENO);
            if (dev_null_fd > STDERR_FILENO) {
                close(dev_null_fd);
            }
        }
        char** child_argv = build_exec_argv(program_path, args);
        execv(program_path.c_str( ), child_argv);
        perror("execv");
        _exit(127);
    }
    close(stdin_pipe[0]);
    (void)write(stdin_pipe[1], stdin_data.data( ), stdin_data.size( ));
    close(stdin_pipe[1]);
    ExecResult result;
    int status = 0;
    pid_t wait_ret = waitpid(child_pid, &status, 0);
    if (wait_ret == child_pid) {
        if (WIFSIGNALED(status)) {
            result.crashed = true;
        }
    }
    return (result);
}

ExecResult Executor::run(const std::string& program_path, const std::vector<std::string>& args)
{
    pid_t child_pid = fork( );
    if (child_pid == 0) {
        apply_resource_limits( );

        int dev_null_fd = open("/dev/null", O_WRONLY);
        if (dev_null_fd >= 0) {
            dup2(dev_null_fd, STDOUT_FILENO);
            dup2(dev_null_fd, STDERR_FILENO);
            if (dev_null_fd > STDERR_FILENO) {
                close(dev_null_fd);
            }
        }

        char** child_argv = build_exec_argv(program_path, args);
        execv(program_path.c_str( ), child_argv);
        perror("execv");
        _exit(127);
    }

    ExecResult result;

    int status = 0;
    pid_t wait_ret = waitpid(child_pid, &status, 0);
    if (wait_ret == child_pid) {
        if (WIFSIGNALED(status)) {
            result.crashed = true;
        }
    }

    return (result);
}
