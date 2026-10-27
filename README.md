# Historical Trade-Up Search Engine

**Author:** Rareș Constantinescu

An entirely offline C++20 application for inspecting historical trade-up prices,
evaluating quantity contracts, and searching restricted combinatorial domains with
exact arithmetic and explicit resource budgets.

The personal prototype began in 2023 and the Sheets/C++ workflow expanded in 2024.
The 2026 implementation adds independently verified reference and optimized
engines, content-derived dataset identities and a practical CLI. Original programs
and spreadsheets remain unaltered evidence, documented separately in the
[historical baseline](docs/historical-baseline.md).

## Build on Windows

The verified direct build uses MSYS2 UCRT64 GCC 14.2.0. There are no third-party
libraries, downloads, accounts or network services. Python 3.10+ is used for
verification, not application execution. From the repository root in PowerShell:

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;' + $env:PATH
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\build_cli.ps1 -Configuration Release
$app = '.\.dist\milestone4\tradeup_Release.exe'
& $app --help
```

Execution-policy bypass applies only to the child process. `-Compiler` and
`-BuildDirectory` accept explicit paths. Keep the compiler's runtime DLL directory
on PATH. Debug builds use `-Configuration Debug` and produce `tradeup_Debug.exe`.
CMake targets are supplied; CMake/CTest are unavailable in this environment, so
the direct build is the verified route.

## Inspect and replay authentic historical data

```powershell
$matrix = '.\historical\2024\most recent version i think\input.txt'
& $app scenarios
& $app profiles
& $app inspect --input $matrix --scenario st-ft-mw-mov
& $app replay --input $matrix --details
& $app eval --input $matrix --scenario 'StatTrak FT MW mov' --contract 'b002:5,b029:5'
```

The preserved 55×58 matrix supplies all 3,190 prices. All twelve scenario mappings
come from source/workbook evidence. Odd/even buckets represent lower/higher input
wear; outcomes use the historical higher-wear quote basket. `inspect --json
--matrix` exposes every original price and selected-scenario metadata.

Replay uses five BREAKOUT higher-category inputs (`b002`) and five BROKEN FANG
lower-category inputs (`b029`). It reproduces cost **4.85**, exact modeled return
**186383/1455% ≈ 128.098282%**, and expected ROI **≈ 28.098282%** through the actual
import/evaluation path. This does not establish exact float feasibility, purchase
availability or guaranteed profit.

## Search, filter and export

```powershell
& $app search --input $matrix --profile '5/5' --max-evaluations 1000 --top 3 `
    --output '.\.dist\milestone4\historical-prefix.json'
$LASTEXITCODE

& $app search --input '.\examples\synthetic-equal-returns.tsv' --format catalog-v1 `
    --min-roi 15 --max-cost 10 --sort profit --top 3 `
    --output '.\.dist\milestone4\synthetic-results.csv'
```

The historical command deliberately bounds a prefix: exit code **3** means an
incomplete budget-limited search. The synthetic example completes with 405
distinct equal-return contracts. Choose new export paths on repeated runs:
existing files are never overwritten. JSON and CSV preserve exact fractions,
identities, policies, provenance, limits and completion status. `--details` adds
the full modeled outcome distribution.

`--min-return` defaults to strict `>115`, equivalent to expected ROI `>15%`.
`--min-roi` is an alternative. `--max-cost` filters retained results inclusively
after search. Ranking uses exact expected ROI or profit, then canonical identity.
`--top` limits human display only; machine output contains every matching retained
result. Equal returns never merge distinct contracts.

Both profiles require ten inputs and two to five active buckets, with 5/5 or 6/4
lower/higher wear quantities. They are restrictions, not unrestricted searches.
The reference enumerator supports twelve catalog buckets; the optimized engine
supports larger catalogs with explicit budgets. A 58-bucket 5/5 domain has
80,723,385 contracts. Loading it does not mean completing its search, and default
commands never launch an unbounded traversal.

The documented `tradeup-catalog-v1` UTF-8 TSV format also supports local declared
catalogs with arbitrary positive modeled outcome counts and explicit provenance.
SHA-256 snapshot identities bind normalized prices and scenario/policy metadata,
independently of filesystem paths.

## Verification and documentation

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\verify_cli.ps1 -Configuration Debug
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\verify_cli.ps1 -Configuration Release
```

These run all Milestone 1–3 regressions, the reconstructed workbook-ledger check,
application C++ tests and independent CLI tests. Earlier direct scripts and
bounded synthetic benchmarks remain available. Historical binaries are never
compiled, linked or run.

The modernization was developed with AI assistance. Mathematical and verification
claims are backed by the checked-in specifications, independent oracles and
recorded results.

- [CLI, local formats, identity, export and budget semantics](docs/offline-cli.md)
- [Milestone 4 integration and verification report](docs/milestone-4-report.md)
- [Optimized engine and completeness proofs](docs/optimized-engine.md)
- [Measured benchmarks and their limits](docs/milestone-3-report.md)
- [Reference engine and arithmetic limits](docs/reference-engine.md)
- [Mathematical model](docs/mathematical-model.md)
- [Historical origins and evidence](docs/historical-baseline.md)

## Limits

Calculations retain `historical-quote-tickets-v1` and the historical proportional
13% deduction. Currency, actual quote timestamps, exact floats, listing supply and
independently verified current game mechanics remain unknown. Composite averages
and ratio estimates are labeled. Missing prices are rejected; explicitly modeled
zero-valued outcomes keep their probability mass.

Unsupported checked signed-64-bit arithmetic fails explicitly. Search results
distinguish complete coverage from evaluation, work, time, storage and arithmetic
interruptions. Exports are bounded and published only after successful
serialization. Sanitizer runtimes remain unavailable; no sanitizer run is claimed.

Release automation, CI deployment and further portability validation remain
outside this milestone. No live-market, account or automated-trading features
are provided.
