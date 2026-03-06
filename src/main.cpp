#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "executor.h"

#define EXHAUSTIVE_LIMIT 10  // 2^10-1 = 1023 subsets

struct Progress {
    std::string step_label;
    int total = 0;
    int current = 0;
    int bar_width = 40;

    void set(const std::string& label, int total_steps)
    {
        step_label = label;
        total = (total_steps > 0 ? total_steps : 0);
        current = 0;
        print( );
    }

    void tick( )
    {
        ++current;
        print( );
    }

    void done( )
    {
        current = total;
        print( );
        std::cout << std::endl;
    }

    void print( )
    {
        double fraction = (total > 0) ? (double)current / (double)total : 0.0;
        int filled = (int)std::round(fraction * bar_width);
        if (filled < 0)
            filled = 0;
        if (filled > bar_width)
            filled = bar_width;
        std::cout << '\r' << std::left << std::setw(20) << step_label << " [";
        for (int i = 0; i < filled; ++i)
            std::cout << '=';
        for (int i = filled; i < bar_width; ++i)
            std::cout << ' ';
        std::cout << "] " << std::setw(3) << (int)(fraction * 100) << "% (" << current << "/" << total << ")" << std::flush;
    }
};

static Progress progress;

void print_usage(const char* prog) { std::cerr << "Usage: " << prog << " <path-to-executable> <num-args>" << std::endl; }

bool parse_args(int argc, char** argv, std::string& out_target, int& out_num_args)
{
    if (argc < 3) {
        return (false);
    }
    out_target = argv[1];
    out_num_args = std::stoi(argv[2]);
    if (out_num_args < 0) {
        std::cerr << "num-args must be non-negative." << std::endl;
        return (false);
    }
    return (true);
}

ExecResult run_with_size(const std::string& target, int num_args, int size)
{
    std::string buffer(size, 'A');
    if (num_args == 0) {
        std::string stdin_string = buffer + '\n';
        return (Executor::run_with_input(target, { }, stdin_string));
    }
    std::vector<std::string> args(num_args, buffer);
    return (Executor::run(target, args));
}

ExecResult run_with_arg_mask(const std::string& target, int num_args, const std::vector<bool>& mask, int size)
{
    std::string buffer(size, 'A');
    if (num_args == 0) {
        std::string stdin_string = buffer + '\n';
        return (Executor::run_with_input(target, { }, stdin_string));
    }
    std::vector<std::string> args;
    args.reserve(num_args);
    for (int index = 0; index < num_args; ++index) {
        if (index < (int)mask.size( ) && mask[index]) {
            args.push_back(buffer);
        } else {
            args.push_back(std::string("A"));
        }
    }
    return (Executor::run(target, args));
}

std::vector<std::vector<int>> find_crashing_arg_subsets(const std::string& target, int num_args, int min_len)
{
    std::vector<std::vector<int>> results;
    if (num_args <= 0) {
        return (results);
    }
    if (num_args <= EXHAUSTIVE_LIMIT) {
        int total = (1 << num_args) - 1;
        progress.set("Finding crashing argument subsets...", total);
        for (int mask = 1; mask < (1 << num_args); ++mask) {
            std::vector<bool> m(num_args);
            std::vector<int> subset;
            for (int i = 0; i < num_args; ++i) {
                if (mask & (1 << i)) {
                    m[i] = true;
                    subset.push_back(i);
                }
            }
            ExecResult result = run_with_arg_mask(target, num_args, m, min_len);
            if (result.crashed) {
                results.push_back(subset);
            }
            progress.tick( );
        }
        progress.done( );
        return (results);
    }
    progress.set("Finding crashing arguments individually...", num_args);
    for (int i = 0; i < num_args; ++i) {
        std::vector<bool> m(num_args, false);
        m[i] = true;
        ExecResult result = run_with_arg_mask(target, num_args, m, min_len);
        if (result.crashed) {
            results.push_back(std::vector<int>{i});
        }
        progress.tick( );
    }
    progress.done( );
    return (results);
}

int find_crash_exponential(const std::string& target, int num_args, int max_len)
{
    std::vector<int> probe_lengths;
    for (int cur = 1; cur <= max_len; cur *= 2) {
        probe_lengths.push_back(cur);
        if (cur == max_len)
            break;
    }
    progress.set("Finding first crashing length using exponential probing...", (int)probe_lengths.size( ));
    for (size_t i = 0; i < probe_lengths.size( ); ++i) {
        int current_length = probe_lengths[i];
        ExecResult result = run_with_size(target, num_args, current_length);
        progress.tick( );
        if (result.crashed) {
            progress.done( );
            return (current_length);
        }
    }
    progress.done( );
    return (-1);
}

int linear_scan_first_crash(const std::string& target, int num_args, int max_len)
{
    progress.set("Finding first crashing length using linear probing...", max_len);
    for (int current_length = 1; current_length <= max_len; ++current_length) {
        ExecResult result = run_with_size(target, num_args, current_length);
        progress.tick( );
        if (result.crashed) {
            progress.done( );
            return (current_length);
        }
    }
    progress.done( );
    return (-1);
}

int binary_search_min_len(const std::string& target, int num_args, int lo, int hi)
{
    int minimal_crash = hi;
    int range = hi - lo + 1;
    int est_iters = (range > 0) ? (int)std::ceil(std::log2(range)) + 1 : 1;
    progress.set("Finding minimal crashing length using binary search...", est_iters);
    while (lo <= hi) {
        int mid_length = lo + (hi - lo) / 2;
        ExecResult result = run_with_size(target, num_args, mid_length);
        progress.tick( );
        if (result.crashed) {
            minimal_crash = mid_length;
            hi = mid_length - 1;
        } else {
            lo = mid_length + 1;
        }
    }
    progress.done( );
    return (minimal_crash);
}

int main(int argc, char** argv)
{
    std::string target;
    int num_args = 0;
    if (!parse_args(argc, argv, target, num_args)) {
        print_usage(argv[0]);
        return (1);
    }
    std::cout << "Target: " << target << " num_args=" << num_args << std::endl;
    const int MAX_LEN = 65536;
    int crash_len = find_crash_exponential(target, num_args, MAX_LEN);
    if (crash_len < 0) {
        std::cout << "No crash found up to " << MAX_LEN << " bytes using exponential probing. Trying linear scan..." << std::endl;
        crash_len = linear_scan_first_crash(target, num_args, MAX_LEN);
    }
    if (crash_len < 0) {
        std::cout << "No crash found up to " << MAX_LEN << " bytes." << std::endl;
        return (0);
    }
    int prev_noncrash = crash_len / 2;
    int lo = prev_noncrash + 1;
    int minimal = binary_search_min_len(target, num_args, lo, crash_len);
    std::cout << "Crash found. Minimal crash length = " << minimal << std::endl;
    if (num_args == 0) {
        std::cout << "The crash is triggered by input to stdin.\n";
    } else if (num_args > EXHAUSTIVE_LIMIT) {
        std::cout << "Too many arguments to identify specific crashing argument subsets.\n";
    } else if (num_args > 0) {
        std::cout << "Identifying crashing argument subsets (may be slow)..." << std::endl;
        auto subsets = find_crashing_arg_subsets(target, num_args, minimal);
        if (subsets.empty( )) {
            std::cout << "No specific argument subset reproduced the crash at length " << minimal << "\n";
        } else {
            std::cout << "Crashing argument subsets :\n";
            for (const auto& s : subsets) {
                std::cout << " - argv[";
                for (size_t i = 0; i < s.size( ); ++i) {
                    if (i)
                        std::cout << ",";
                    std::cout << s[i];
                }
                std::cout << "]\n";
            }
        }
    }
    return (0);
}
