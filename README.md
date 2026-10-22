# Historical Trade-Up Search Engine

**Author:** Rareș Constantinescu

An offline C++ project for studying trade-up search, expected returns, and algorithmic correctness using preserved local price data. The original prototype dates from 2023, its Sheets/C++ workflow was extended in 2024, and modernization began in 2026.

Milestone 1 provides a historical baseline, a mathematical specification, and bounded verification examples. The reference search engine is the next milestone and has not been implemented.

## Baseline and model

- [Historical workflow, formats, and evidence](docs/historical-baseline.md)
- [Probability, search domain, identity, and numerical examples](docs/mathematical-model.md)
- [Milestone 1 report, verification, and next step](docs/milestone-1-report.md)

The specified first model assigns each modeled collection outcome one ticket per contributing input, then applies the historical 13% proportional sale deduction. The planned historical search restriction uses ten inputs, a 5/5 split between the selected wear buckets, and at most five active buckets. The earlier 6/4 strategy is documented separately.

Historical prices include both quoted values and spreadsheet estimates. Some outcome prices were averaged into a five-slot representation. A positive expected return is conditional on these assumptions; it does not establish exact float feasibility, available listings, or profit under every outcome. Current CS2 compatibility is outside the initial model.

The historical collection grouping, minimum-price preprocessing, ordered selection, quantity representation, and common-data scenarios remain the starting points for modernization. No optimized engine or performance improvement is claimed yet.

## Verify locally on Windows

Python 3.10 or newer runs the exact rational fixtures and read-only archive checks without third-party packages:

```powershell
python .\tools\verify_baseline.py
```

Expected result: `Ran 14 tests` and `OK`. Optional untracked archive files may be absent in a clean checkout; any that are present are checked. To require the entire locally preserved archive:

```powershell
python .\tools\verify_baseline.py --require-local
```

A minimal CMake project builds only the isolated C++ numerical examples. With CMake 3.20+, a suitable C++20 compiler, and a matching generator installed:

```powershell
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

For the installed MSYS2 UCRT64 GCC toolchain, from PowerShell with its `bin` directory on `PATH`:

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe -DCMAKE_MAKE_PROGRAM=C:/msys64/ucrt64/bin/mingw32-make.exe
cmake --build build
ctest --test-dir build --output-on-failure
```

CMake was unavailable in the Milestone 1 environment, so the CMake/CTest path remains unverified. The examples were compiled directly with GCC 14.2.0 using strict diagnostics:

```powershell
New-Item -ItemType Directory -Force -Path build | Out-Null
& 'C:\msys64\ucrt64\bin\g++.exe' -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -O0 -g .\tests\baseline_examples.cpp -o .\build\baseline_examples.exe
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed' }
& .\build\baseline_examples.exe
```

Expected output: `PASS: 9 numerical and language-semantics checks`.

## Coverage and preservation

The checks cover six fixed numerical cases, twelve source scenario mappings, all 3,190 exported prices, archive integrity, and bounded witnesses for arithmetic, loop omission, and result identity. They do not establish full legacy enumeration or a usable modern search application.

Everything under `historical/` is immutable, including untracked files. Historical binaries are not executed or included in build targets. No data downloads, market integrations, or external accounts are required. Input pricing timestamps and exact float values are unavailable.

## Roadmap

1. Historical baseline and mathematical specification — delivered for review.
2. Independent reference engine — next, after baseline approval.
3. Optimized engine with differential verification.
4. Local data conversion and practical command-line workflow.
5. Reproducible benchmarks, CI, and portfolio release.

Search commands, result export, optimization measurements, and benchmarks will be documented when those milestones supply working implementations. There are no performance claims in this baseline.
