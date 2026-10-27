# Verification and reproducibility

Run configure, build and CTest as shown in [building](building.md). Full CTest has
ten entries when Python 3.10+ is present. Use `ctest --test-dir build/debug -N` to
inspect discovery and `--output-on-failure` to retain diagnostics. Successful
configuration/build does not substitute for running the tests.

| CTest entry | What it verifies |
| --- | --- |
| baseline_examples | Nine numerical/language witnesses |
| reference_tests | 36 public API, rational boundary, identity and enumeration cases |
| optimized_tests | 28 cases, complete reference comparisons, feature combinations and budgets |
| application_tests | SHA-256 vectors, display/arithmetic boundaries and export publication races |
| baseline_fixtures | 14 historical/numerical groups, matrix, source/workbook projection and manifest |
| reference_differential | Eight independent Fraction/Cartesian groups, fixtures and 6,000 rational operations |
| optimized_differential | Eleven groups including both profiles, large-catalog evaluations and fallback paths |
| cli_end_to_end | Exact imports/evaluations, all scenarios, bounded prefixes, malformed inputs, exports and filesystem safety |
| historical_metadata | Deterministic reconstruction of all 3,190 reviewed ledger cells |
| repository_consistency | Documentation links, README identity, required ledger and archive newline/tampering regressions |

The Python oracles use their own arbitrary-precision `Fraction` calculations and
Cartesian enumeration. They do not derive expected answers through production
search/evaluation functions. Both profiles, equal-return distinct identities,
strict thresholds, incomplete results, exact JSON/CSV fractions and historical
replay are exercised. Ordinary tests use bounded catalogs/prefixes, never an
unrestricted 58-bucket mixed enumeration.

## Historical evidence in a clean checkout

The immutable [archive manifest](../tests/fixtures/historical_manifest.json)
records 33 files, of which **11** are tracked. A clean checkout requires those
eleven, all source/fixtures/examples and `src/historical_cells.inc`. Missing
required evidence fails a test; it is not silently skipped.

Git already stores nine historical text files with LF endings, while the original
archive uses CRLF. `tools/historical_integrity.py` accepts either the exact original
size/SHA-256 or, **only for tracked `.txt`/`.cpp` files without any CR bytes**, the
exact LF form whose CRLF restoration matches that same frozen size/SHA-256. It
writes nothing. Altered text, mixed endings, truncated content and optional files
receive no relaxed comparison. The original manifest is unchanged. Both newline
forms, strict mode and tampering cases have executable regressions.

The 22 untracked/ignored files are optional evidence: saved outputs, screenshots,
IDE files, damaged/misnamed documents and old compiled artifacts. If present they
must match the original bytes; clean-checkout tests state their absence. The
first saved line in optional `www.txt` is checked when available. Its exact
mathematical fixture and authentic matrix replay remain required without that file.
No preserved historical executable is ever run.

For an archive-complete local check:

```powershell
python tools/verify_baseline.py --require-local
```

This requires all 33 files and their **original exact bytes**, with no newline
conversion acceptance. `--independent-archive PATH` also compares the original
archive layout (containing `history`, `most recent version i think`, and `excel`).
That user-owned path is optional and never a build/runtime dependency.

The ledger checker needs only tracked evidence, using the Python standard
library's ZIP/XML parsing of cached workbook data. No formula or historical script
is executed and no spreadsheet package is required:

```powershell
python tools/reconstruct_historical_metadata.py --check
```

To regenerate into a separate reviewed source file, use `--output
build/reconstructed.inc` after creating `build`; compare it before replacing the
committed source. Never generate into `historical/`. The normalized generated
UTF-8/LF ledger SHA-256 remains
`06f0bff048a096d2ea13c85bc28b2fe1ada97b3d7e0964da2c2f8c744e9152e4`.

## Automation and actual coverage

[The workflow](../.github/workflows/verify.yml) runs on pushes and pull requests,
with read-only contents permission and checkout credentials disabled. It defines
Ubuntu 24.04 GCC Debug/Release, GCC Debug ASan+UBSan, and Windows 2022 MSYS2 UCRT64
Debug/Release. Python verification is mandatory in every job. The Linux Release
job compiles the optional benchmark without timing it. Jobs have finite timeouts,
need no secrets and use offline application tests. Only checkout/toolchain setup
requires external networking. The action sources are
[official checkout](https://github.com/actions/checkout/tree/v5) and
[MSYS2 setup](https://github.com/msys2/setup-msys2/tree/v2).

**Hosted execution is pending.** No workflow was pushed/triggered during this work.
Local `actionlint` 1.7.12 validation of the workflow passed with no diagnostics;
the downloaded Windows tool's published SHA-256 was checked before execution.
Windows GCC CMake/CTest and direct builds were executed locally; Linux, Clang,
MSVC and sanitizers remain unverified. Windows link probes failed for missing
`-lasan`/`-lubsan`; that is a blocker, not sanitizer coverage. The workflow will
provide actual execution only after the owner pushes it and reviews the results.
Windows symlink tests explicitly report a privilege skip when the account lacks
symlink creation rights; Linux is expected to execute those cases.

## Recorded local results

Verification on 27 October 2026 used Windows 10 build 19045, MSYS2 UCRT64 GCC
14.2.0 Rev2 and CMake/CTest 3.31.10 with MinGW Makefiles. CMake discovered
UCRT64 Python 3.12.7; direct PowerShell verification used CPython 3.10.11.
The warning/optimization flags and runtime requirements are in [building](building.md).

| Configuration | Observed result |
| --- | --- |
| CMake Debug and Release | Full build and 10/10 CTest entries passed in each configuration |
| Direct GCC Debug and Release | Complete numerical, reference, optimized, historical and application suites passed |
| Isolated source-only Debug and Release | Clean builds and 10/10 CTest entries passed using only the eleven tracked historical files |
| Python discovery disabled, Release | Build and 4/4 C++ CTest entries passed; Python omissions were reported |
| `BUILD_TESTING=OFF`, Release | Three-library/CLI build and authentic historical replay passed |
| Python mandatory but discovery disabled | Configure failed with the intended missing-Python diagnostic |
| Windows ASan/UBSan option | Configure rejected the unsupported runtime/platform configuration |
| Optional Release benchmark target | Compiled; ordinary CTest did not time it |

Source-only runs used isolated exports of the reviewed tracked sources plus the
intended new files, with historical evidence read from the actual Git blobs.
All 22 optional historical files were absent. No `.git`, `.dist` inspection files,
previous objects or user-specific directories were copied. These were pre-commit
checkout simulations, without creating a new commit or altering the index.

The optimized C++ suite compared 29,173 canonical vectors, 105,612 detailed
evaluations and 384 feature searches. Independent Python reference/optimized
suites checked 18/46 Cartesian cases covering 1,249/2,931 vectors, the six fixed
numerical fixtures and generated evaluations. Each exercises the same 6,000
shared rational-operation cases; these are not 12,000 distinct operations.

Application checks passed twelve C++ tests and thirty CLI groups comprising
197 application invocations per configuration, with one explicit Windows
symlink-privilege skip. Junction protection and permitted-junction export ran
successfully. The CLI independently checked all twelve scenarios, 3,190 matrix
prices, 696 input quotes, 1,140 selected outcome quotes and 172 retained results
across the two 1,000-evaluation historical prefixes. Those prefixes returned
incomplete status, retaining 53 results under 5/5 and 119 under 6/4.

Synthetic CLI comparisons covered 32 ranked queries, 1,358 candidate positions,
278 qualifying evaluations and 1,304 outcome comparisons. The equal-return
example retained 405 distinct 5/5 contracts and 390 under 6/4. These counts describe
one configuration and include repeated comparisons across cases/options.

Archive-complete verification matched all 33 original file sizes/SHA-256 hashes
and the independent archive, while all 3,190 cent-rounded workbook prices and
reconstructed ledger cells agreed. Repository checks passed all three groups.
The [historical replay](offline-cli.md#historical-replay) and exact JSON/CSV exports
passed through the real importer/evaluator. [Benchmarks](benchmarks.md) preserve
the separately recorded original and final timing runs.

These are local Windows results. Hosted Linux/Windows workflow results and actual
ASan/UBSan execution remain pending; Clang, MSVC and macOS have not been executed.
