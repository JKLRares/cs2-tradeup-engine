# Historical Trade-Up Search Engine

**Author:** Rareș Constantinescu

An offline C++20 reference engine for studying bounded trade-up search and expected returns using explicitly modeled local price data. The original personal prototype dates from 2023, the Sheets/C++ workflow was extended in 2024, and the reference implementation was developed in 2026.

Milestone 2 provides exact arithmetic, validated collection/bucket/outcome models, canonical contract identities, and exhaustive enumeration for bounded historical wear profiles. It is a library with verification tools; a practical application and optimized search are later milestones.

## Model and supported search

The `historical-quote-tickets-v1` policy gives each declared outcome one ticket per contributing collection input. Net expected value uses the historical 13% proportional deduction. Qualification compares exact return percentages strictly against a threshold; the default `>115` means expected ROI above 15%.

The named search profiles use ten inputs, either a 5/5 or 6/4 lower/higher wear split, and two to five active buckets. Enumeration supports at most 12 catalog buckets, with explicit node and result budgets. It emits each eligible vector once and retains distinct contracts with equal returns. The evaluator accepts general ten-input contracts and arbitrary positive modeled outcome counts.

Historical values include quotes, composite averages, and ratio-derived estimates. Their provenance and limitations remain explicit. A mathematically positive expected return does not prove exact output wear, purchasable listings, or profit under every real outcome. Current CS2 compatibility is outside this model.

- [Reference API, enumeration proof, identity, and arithmetic limits](docs/reference-engine.md)
- [Milestone 2 results, exact changed files, and review notes](docs/milestone-2-report.md)
- [Approved mathematical specification](docs/mathematical-model.md)
- [Historical evidence and data formats](docs/historical-baseline.md)
- [Milestone 1 baseline report](docs/milestone-1-report.md)

## Build and verify on Windows

Requires a C++20 compiler and Python 3.10+ for independent verification. There are no third-party libraries, downloads, accounts, or network services.

The tested PowerShell route uses the available MSYS2 UCRT64 GCC compiler. The execution-policy option applies only to that child process and does not change persistent policy:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\verify_reference.ps1 -Compiler 'C:\msys64\ucrt64\bin\g++.exe' -Configuration Debug
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\verify_reference.ps1 -Compiler 'C:\msys64\ucrt64\bin\g++.exe' -Configuration Release
```

The helper compiles the reference implementation and three test executables, then runs the full available suite. Build output defaults to `build/reference-direct`; `-BuildDirectory` and `-HistoricalRoot` accept explicit paths. A failed compile or check terminates the script.

Expected summaries per configuration:

```text
PASS: 9 numerical and language-semantics checks
Reference unit tests: 36 passed, 0 failed
Ran 14 tests ... OK
Ran 8 tests ... OK
PASS: complete available Debug verification suite
```

The last line uses the selected configuration. Independent comparisons include 6,000 rational operations, six baseline fixtures, 40 generated evaluations, and 18 bounded enumeration cases containing 1,249 eligible vectors.

With CMake 3.20+ and a matching installed generator:

```powershell
cmake -S . -B build/cmake -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe -DCMAKE_MAKE_PROGRAM=C:/msys64/ucrt64/bin/mingw32-make.exe -DCMAKE_BUILD_TYPE=Debug
cmake --build build/cmake
ctest --test-dir build/cmake --output-on-failure
```

CMake builds `tradeup_reference`, `baseline_examples`, `reference_tests`, and the private `reference_fixture_driver`. With Python available, CTest registers four suites. CMake/CTest were not found in the current environment, so that route remains unverified. Direct Debug and Release builds passed with GCC 14.2.0 and strict warnings. No sanitizer coverage is claimed.

The preserved baseline can also be checked without compiling:

```powershell
python .\tools\verify_baseline.py
```

Add `--require-local` to require all 33 locally preserved files. A clean checkout may omit untracked/ignored archival files; present files must match their recorded hashes.

## Design and limitations

The engine retains the original collection grouping, representative-price concept, quantity contracts, canonical option order, and shared scenario model. It replaces unsafe fixed arrays and integer-truncated weights with validated values and exact checked rational calculations.

The reference enumerator favors a transparent completeness argument. It provides no optimized pruning, parallel search, Top-K, compact result grouping, spreadsheet converter, or application CLI. The private fixture driver is a verification interface rather than a user-facing import format. There are no speedup or benchmark claims.

All files under `historical/` remain immutable, including untracked artifacts. Historical binaries are never executed or linked. Missing prices are rejected; explicit zero-valued outcomes remain possible. Exact arithmetic has documented signed-64-bit intermediate limits and fails explicitly on overflow. See the reference documentation for the complete supported domain.

## Roadmap

1. Historical baseline and mathematical specification — reviewed and approved.
2. Independent C++ reference engine — delivered for review.
3. Optimized engine with differential verification — requires authorization.
4. Local data conversion and practical command-line workflow.
5. Reproducible benchmarks, CI, and portfolio release.
