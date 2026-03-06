#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <sys/types.h>

struct ExecResult {
    bool crashed = false;
};

namespace Executor {
    ExecResult run(const std::string &path, const std::vector<std::string> &args);
    ExecResult run_with_input(const std::string &path, const std::vector<std::string> &args, const std::string &stdin_data);
}
