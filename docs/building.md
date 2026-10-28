# Building and running

The application uses C++20 and the standard library. Python 3.10+ is optional for
building/running and required for the full verification configuration. No pip
packages, workbook libraries or generated downloads are runtime dependencies.
`src/historical_cells.inc` is a required reviewed source resource, already in Git.
Historical programs and binaries are never compiled, linked or executed.

## Windows: verified GCC route

Use MSYS2 UCRT64 GCC, with its `bin` directory on the current process PATH.
The local verified compiler is GCC 14.2.0, MSYS2 UCRT64 Rev2, on Windows 10
build 19045. CMake 3.31.10 and MinGW Makefiles were exercised in Debug and Release.
MSVC and Clang have not been executed; their coverage must not be inferred from
CMake recognizing their names.

If the tools are missing, install MSYS2 from its official site, then run this in
the **MSYS2 UCRT64** terminal (these are manual setup instructions):

```sh
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-python
```

[MSYS2's CMake guidance](https://www.msys2.org/docs/cmake/) recommends its native
MinGW CMake and Ninja packages. No project script installs software or changes
machine settings. For an existing GCC installation without CMake, the direct
PowerShell scripts need no additional build tool.

Alternatively, obtain a Windows x64 ZIP from
[CMake's official downloads](https://cmake.org/download/), extract the complete
distribution into a user-owned directory, and add that distribution's `bin` to
the current PowerShell process PATH. Keep its sibling `share` directory.
The following project-local alternative was also verified:

```powershell
python -m pip install --disable-pip-version-check --no-cache-dir --no-deps --only-binary=:all: --target .dist/tooling cmake==3.31.10
$env:PATH = (Join-Path $PWD '.dist/tooling/cmake/data/bin') + ';C:\msys64\ucrt64\bin;' + $env:PATH
cmake --version
ctest --version
```

This downloads build tooling into ignored project storage without a system-wide
installation. Network access is needed for that setup step only.

From the repository root in PowerShell:

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;' + $env:PATH
cmake -S . -B build/debug -G 'MinGW Makefiles' -DCMAKE_BUILD_TYPE=Debug -DTRADEUP_REQUIRE_PYTHON=ON
cmake --build build/debug --parallel 4
ctest --test-dir build/debug --output-on-failure
```

Use a separate `build/release` directory with `-DCMAKE_BUILD_TYPE=Release` for
Release. Ninja can replace the generator when installed. Multi-configuration
generators need `cmake --build build --config Debug` and `ctest --test-dir build
-C Debug`; that generator path is supplied by CMake but not locally verified.
In-source builds are rejected. After any command fails, stop and investigate its
exit code before executing later commands; a failed configure is not a test pass.

Without CMake:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\build_cli.ps1 -Configuration Release -BuildDirectory build/direct
& '.\build\direct\tradeup_Release.exe' --help
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\verify_cli.ps1 -Configuration Debug -BuildDirectory build/direct-debug
```

The complete verifier builds and checks the numerical, reference, optimized and
application suites.
`-Compiler` accepts an explicit GCC path. `-HistoricalRoot` on the verifier accepts
an alternate read-only evidence directory. Execution-policy bypass affects the
child process only. Choose a fresh build directory to demonstrate a clean build.

GCC runtime DLLs must be available on PATH. The verified application directly
imports `libgcc_s_seh-1.dll` and `libstdc++-6.dll`; those runtimes also depend on
`libwinpthread-1.dll`. Windows/UCRT system DLLs are separate. These files are
provided by the selected UCRT64 toolchain. No standalone binary, redistributable
bundle or runtime-DLL license clearance is claimed.

Paths with spaces and Unicode are supported through Windows wide arguments and
native filesystem paths. Saved JSON/CSV is UTF-8; in PowerShell 5 use
`Get-Content -Encoding UTF8 -Raw` to read JSON. Console glyph rendering additionally
depends on the terminal's font/encoding. Inputs accept LF and CRLF; embedded bare
carriage returns are rejected.

## Linux: verified hosted builds and sanitizers

With GCC, CMake 3.20+, Ninja and Python 3.10+ installed:

```sh
cmake -S . -B build/debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DTRADEUP_REQUIRE_PYTHON=ON
cmake --build build/debug --parallel 2
ctest --test-dir build/debug --output-on-failure
cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release -DTRADEUP_REQUIRE_PYTHON=ON
cmake --build build/release --parallel 2
ctest --test-dir build/release --output-on-failure
./build/release/tradeup replay --input 'historical/2024/most recent version i think/input.txt'
```

For a GCC/Clang environment with actual ASan/UBSan runtimes:

```sh
cmake -S . -B build/sanitizers -G Ninja -DCMAKE_BUILD_TYPE=Debug -DTRADEUP_REQUIRE_PYTHON=ON -DTRADEUP_ENABLE_SANITIZERS=ON
cmake --build build/sanitizers --parallel 2
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 ctest --test-dir build/sanitizers --output-on-failure
```

The sanitizer option instruments all modern libraries, CLI, C++ tests and private
drivers, including binaries invoked by Python. It checks runtime linkage and
rejects unsupported Windows/compiler configurations; successful linkage alone
does not establish execution coverage. Linux builds and ASan/UBSan test execution
passed in both [confirmed hosted runs](verification.md#automation-and-actual-coverage).
Earlier local Windows GCC probes could not link `libasan` or `libubsan`; those
local limitations are separate from the successful hosted Linux verification.

## Optional targets and dependency-free build

`BUILD_TESTING=OFF` builds just the three libraries and CLI (and optional benchmark).
With testing enabled, Python is discovered automatically. Without Python, CTest
still registers four C++ suites and explicitly reports omitted Python checks.
`TRADEUP_REQUIRE_PYTHON=ON` makes missing Python a configuration failure, as in CI.

`-DTRADEUP_BUILD_BENCHMARKS=ON` adds `search_benchmark`; ordinary CTest never runs
it. Use a Release direct build with `verify_optimized.ps1 -Configuration Release
-Benchmarks` to reproduce the original documented benchmark flags and workload.
CMake's GCC Release default is `-O3 -DNDEBUG`, whereas both recorded
[benchmark runs](benchmarks.md) use `-O2 -DNDEBUG`. Attribute timings to their
documented build configuration.

All targets use strict warnings as errors. GCC Debug enables `_GLIBCXX_ASSERTIONS`;
tests retain active checks in Release. Build trees, binaries, temporary exports,
local tooling and benchmark output belong in ignored directories.
