Fuzz Finder
===========

Overview
--------
Fuzz Finder is a minimal single-threaded fuzzer written in C++17. It probes a target program by increasing input sizes and, once a crash is detected, performs a binary search to determine the minimal input length that triggers the crash. The fuzzer supports passing data via command-line arguments (one or more argv entries) or via stdin.

Requirements
------------
- A POSIX-compatible environment (Linux, macOS, WSL)
- A C++17-capable compiler (e.g. `g++` / `clang++`)
- `make`

Building from source
--------------------
1. Clone the repository:

```
git clone https://github.com/LeoPolizzi/Fuzz_Finder.git
cd Fuzz_Finder
```

2. Build with `make`:

```
make
```

The `make` target produces `bin/fuzzer`.

Compiling an example vulnerable program
---------------------------------------
For testing, you can build programs like this:

```
gcc -o vuln vuln_program.c -g -O0 -fno-stack-protector -z execstack
```

Usage
-----
Basic usage:

```
./bin/fuzzer <path-to-executable> <num-args>
```

- `path-to-executable` : path to the target binary to fuzz
- `num-args` : number of argv entries to populate with test data. Use `0` to fuzz `stdin` instead.

Examples
--------
1) Fuzz `stdin` of the example target (num-args = 0):

```
./bin/fuzzer ./examples/vuln_overflow 0
```

2) Pass fuzzed data as a single argument (num-args = 1):

```
./bin/fuzzer ./examples/vuln_overflow 1
```

Notes
-----
- The fuzzer starts with exponential probing to find a length that causes a crash, then performs a binary search to find the minimal crashing length.
- The program prints progress bars and reports the minimal crashing length when found.
- Extra command-line flags can be passed after these two positional arguments; they will be ignored by the basic argument parser but can be extended in the future.

Interpreting results
--------------------
When a crash is found the fuzzer prints the minimal input length that produced the crash and (for multi-argument cases) attempts to identify which argv entries reproduce the crash.

Continuous integration and releases
-----------------------------------
This repository includes a GitHub Actions workflow that builds the project and uploads `bin/fuzzer` as a build artifact on every push and pull-request. When a tag matching `v*.*.*` is pushed, the workflow produces a release and attaches the built artifact.

See `.github/workflows/ci.yml` for the exact CI configuration.

Contributing
------------
Contributions are welcome. Open a PR with changes and the CI will run the build. For large changes, include tests or simple reproduction steps.

License
-------
This project is licensed under the MIT License. See the `LICENSE` file at the repository root for full terms.
