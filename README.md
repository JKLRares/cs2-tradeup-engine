# CS2 Trade-Up Engine
**Author:** Rareș Constantinescu

A [trade-up contract](https://help.steampowered.com/en/faqs/view/0151-608B-BCAA-D7D5)
exchanges Counter-Strike weapon skins for a higher-rarity result. The possible
results have different values, so comparing their expected return with the cost
of the inputs matters.

This offline C++20 application analyzes historical ten-skin combinations. It lets
you evaluate a contract or search combinations, compare modeled returns, and
inspect the results with their probabilities and assumptions.

You can:

- Load the preserved 2024 price matrix or a custom offline UTF-8 catalog.
- Calculate input cost, modeled outcome probabilities, expected return on investment
  (ROI) and the worst modeled receipt.
- Search the supported **5/5** and **6/4** input-wear profiles.
- Rank results by expected ROI or profit and filter by return threshold or cost.
- Export exact JSON/CSV values with identities, provenance and completion status.
- Set evaluation, work, time and storage budgets, with explicit incomplete results.

## Quick start

From the repository root in Windows PowerShell, with MSYS2 UCRT64 GCC installed:

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;' + $env:PATH
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\build_cli.ps1 -Configuration Release -BuildDirectory build/direct
$app = '.\build\direct\tradeup_Release.exe'
$matrix = '.\historical\2024\most recent version i think\input.txt'
& $app replay --input $matrix --details
```

This builds the application and replays an authentic saved historical contract.
See [toolchain setup and alternative builds](docs/building.md) if GCC is missing
or installed elsewhere. Running the application needs no Python or spreadsheet
packages. Use `& $app --help` for all commands and options.

## A historical example

The replay evaluates **`b002:5,b029:5`** in **`st-ft-mw-mov`**: five BREAKOUT
higher-wear inputs and five BROKEN FANG lower-wear inputs, using preserved prices.

| Metric | Modeled value |
| --- | ---: |
| Input cost | 4.85 |
| Return, including capital | 128.098282% |
| Expected ROI | 28.098282% |
| Worst net receipt | 2.784 |

Return and ROI are rounded here; the exact return is `186383/1455` percent.
The worst receipt is below the input cost: positive expected ROI does **not**
guarantee profit. Currency and the original quote timestamps are unknown.

To evaluate the same allocation explicitly:

```powershell
& $app eval --input $matrix --scenario st-ft-mw-mov --contract 'b002:5,b029:5'
```

## How it works

Local prices and scenario metadata → validated data model → exact evaluation
and optimized search → ranked results and exports.

The importer checks prices, collection mappings and provenance. Evaluation combines
each input collection's modeled outcomes and compares expected receipts with cost.
Search groups interchangeable inputs into quantities rather than testing every
ordering of ten skins. Exact fractions keep rounding out of qualification and
ranking; outcome details remain available for inspection.

## Search, filter and export

These examples use a clearly labeled **synthetic** catalog, completing with 405
distinct contracts under 5/5 or 390 under 6/4:

```powershell
$catalog = '.\examples\synthetic-equal-returns.tsv'
& $app search --input $catalog --format catalog-v1 --profile '5/5' `
    --min-roi 15 --max-cost 10 --sort profit --top 3
& $app search --input $catalog --format catalog-v1 --profile '6/4' --top 3
```

Both profiles use ten inputs and two to five active buckets. The split specifies
lower/higher input wear quantities; it does not establish exact float eligibility.
Default qualification is strictly return **>115%**, equivalent to expected ROI
**>15%**. Cost filtering happens after search. `--top` limits human rows only.

A deliberately incomplete historical prefix:

```powershell
& $app search --input $matrix --max-evaluations 1000 --time-ms 60000 --top 3
$LASTEXITCODE # 3: incomplete search; retained results are partial
```

Only `complete` asserts coverage of the selected domain. Budget and arithmetic
interruptions are explicit; a bounded prefix does not establish global rankings.

```powershell
& $app replay --input $matrix --details --output '.\build\replay.json'
& $app search --input $catalog --format catalog-v1 --output '.\build\synthetic.csv'
```

Choose fresh output names on repeats: existing files are never overwritten.
JSON/CSV include exact fraction strings, every matching retained result, policies,
provenance, budgets and completion status. Publication requires same-directory
hard-link support. See the [CLI and input/export formats](docs/offline-cli.md).

## Architecture

- **Reference engine:** a small exhaustive correctness baseline for catalogs of
  up to twelve buckets, with checked rational arithmetic and full evaluations.
- **Optimized engine:** generates quantity combinations directly, reuses collection
  summaries, applies exact scaled-integer decisions and optional proved bounds,
  and retains compact results with full outcome expansion on demand.
- **Offline CLI:** validates datasets, reconstructs historical metadata, derives
  SHA-256 identities, ranks results and writes JSON/CSV.

Distinct contracts remain distinct even when their expected returns match.
See the [reference completeness proof](docs/reference-engine.md) and
[optimized search specification](docs/optimized-engine.md).

## Measured performance

On selected, identical **synthetic 12-bucket mixed workloads**, the final recorded
run measured **7.10× / 7.79×** faster expanded searches (5/5 / 6/4), and
**20.18× / 20.29×** faster compact searches. Expanded mode materializes the same
full qualifying outcomes as the reference; compact mode defers those details.

These are ratios of median search times over five samples on Windows 10,
i7-9700, GCC 14.2.0, `-O2 -DNDEBUG`, seed 20261026. Pruning reduced candidates
but slowed one integer workload by about 12% (17% in the original run).
The [benchmark evidence](docs/benchmarks.md) separates the original measurements
from the final rerun and documents workloads, output costs and variability.
These ratios do not predict every search. The full 58-bucket historical mixed
domain has not been exhaustively searched.

## Build and verification

Local builds/tests passed on Windows 10 with MSYS2 UCRT64 GCC 14.2.0 and
CMake 3.31.10. Python 3.10+ and its standard library are used for verification.
Keep the compiler's runtime DLL directory on PATH; the executable is not a
standalone distribution package.

With CMake available, a Release build and the full test suite are:

```powershell
cmake -S . -B build/release -G 'MinGW Makefiles' -DCMAKE_BUILD_TYPE=Release -DTRADEUP_REQUIRE_PYTHON=ON
cmake --build build/release --parallel 4
ctest --test-dir build/release --output-on-failure
```

Tests cover exact fixtures, exhaustive small searches, independent Python
Fraction/Cartesian comparisons, historical reconstruction and adversarial CLI
and export behavior. [Verification details](docs/verification.md) distinguish
executed local checks from pending hosted CI and sanitizer runs.
[GitHub Actions](.github/workflows/verify.yml) configures Linux Debug/Release,
Linux ASan+UBSan and Windows UCRT64 Debug/Release; hosted success is not claimed.
See [building](docs/building.md) for Debug, direct GCC and Linux commands.

## Background

The project began as an independently developed personal high-school trade-up
finder in 2023–2024. Google Sheets organized prices and C++ searched selected wear
splits. Original code, workbooks and data remain preserved in `historical/`,
separate from the modern implementation. The [historical baseline](docs/historical-baseline.md)
documents the surviving evidence and its provenance.

## Model and limitations

The application uses `historical-quote-tickets-v1`, a proportional 13% deduction
and a higher-wear quote basket with labeled composite/estimated values.
The [mathematical model](docs/mathematical-model.md) states those assumptions.
It does not verify current CS2 mechanics, exact floats, output wear feasibility
or actual listing availability. There are no live prices, account connections
or automated trading, and no guarantee of profit.

The historical 5/5 domain contains 80,723,385 contracts. Supporting that dataset
does not imply completing its search. Unsupported signed-64-bit intermediates
fail explicitly; storage/work budgets and cooperative time checks are not
whole-process memory caps or hard deadlines.

No license has been selected. Public distribution still requires the author's
[license choice and historical-data provenance review](docs/distribution.md).
