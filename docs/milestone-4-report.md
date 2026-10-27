# Milestone 4 — historical integration and offline CLI

Completed on `modernization`, with verification recorded on 27 October 2026.
The application now loads authentic preserved data, evaluates/replays contracts,
runs bounded optimized searches, filters/ranks results, and exports exact JSON/CSV.
Milestone 5 was not started. No historical file, independent archive, staged index,
commit, branch or Git history was changed. No network services or dependencies
were used or installed.

## 1. Supported historical and local formats

The application directly supports the original 2024 `input.txt`: header 58, then
55 rows × 58 prices, exactly **3,190** values. It supports all **twelve** original
2024 scenarios. All prices, including original structural padding and unused
scenario rows, remain available through `inspect --json --matrix`.

Parsing validates dimensions, exact numeric syntax, file size, UTF-8, extra or
missing values and strictly positive input-cost rows. Diagnostic locations refer
to matrix data rows/columns or workbook rows/buckets. No absent quote becomes
zero and no missing outcome is silently renormalized away.

The explicit local `tradeup-catalog-v1` TSV format additionally accepts declared
collections, representative buckets, prices, provenance and arbitrary positive
modeled outcome counts within resource limits. Its two examples are explicitly
synthetic. Earlier six-vector 2023 formats, partial saved output fragments and a
general-purpose XLSX application importer are not implemented; no semantic mapping
for them is guessed. The existing 6/4 profile remains available for imported data.

## 2. Exact scenario and collection mapping

These rows were reconstructed from the actual twelve preserved 2024 C++ calls,
checked against earlier fixtures and workbook/Apps Script evidence, then verified
through the application. All numbers in the output column are **workbook rows**;
the one-based matrix data row is `workbook row - 66`.

| CLI ID | Original label | Workbook cost row | Matrix cost row | Workbook output rows |
| --- | --- | ---: | ---: | --- |
| normal-ft-mw-mov | NORMAL FT MW mov | 67 | 1 | 74,75,76,77,78 |
| normal-ft-mw-roz | NORMAL FT MW roz | 69 | 3 | 71,72,73,107,108 |
| st-ft-mw-mov | StatTrak FT MW mov | 68 | 2 | 82,83,84,85,86 |
| st-ft-mw-roz | StatTrak FT MW roz | 70 | 4 | 79,80,81,107,108 |
| normal-mw-fn-mov | NORMAL MW FN mov | 103 | 37 | 90,91,92,93,94 |
| normal-mw-fn-roz | NORMAL MW FN roz | 105 | 39 | 87,88,89,107,108 |
| st-mw-fn-mov | StatTrak MW FN mov | 104 | 38 | 98,99,100,101,102 |
| st-mw-fn-roz | StatTrak MW FN roz | 106 | 40 | 95,96,97,107,108 |
| normal-ft-mw-rosu | NORMAL FT MW rosu | 111 | 45 | 109,110,121,107,108 |
| st-ft-mw-rosu | StatTrak FT MW rosu | 114 | 48 | 112,113,121,107,108 |
| normal-mw-fn-rosu | NORMAL MW FN rosu | 117 | 51 | 115,116,121,107,108 |
| st-mw-fn-rosu | StatTrak MW FN rosu | 120 | 54 | 118,119,121,107,108 |

The 29 collections retain the C++ table order. Stable IDs are `c01`–`c29` and
`b001`–`b058`; odd `2c-1` is lower input wear, even `2c` higher. Both wear buckets
share a single collection outcome set. Outcomes use the following even bucket's
selected higher-wear column, as in the original source. Outcome IDs such as
`row82` identify modeled worksheet slots without inventing missing item metadata.
Their order is lexical, consistent with the unchanged engine API.

FT/MW and MW/FN are wear categories, not mixed input rarities. The later workbook
and script's Kilowatt block is deliberately absent from this 58-column mapping.
The Apps Scripts establish organization and cell transfers, not verified market
acquisition or quote time. The preserved historical workflow is described
separately in [the historical baseline](historical-baseline.md).

## 3. Identity and provenance strategy

Imported snapshot IDs now use a portable C++ SHA-256 implementation over a
versioned, length-prefixed canonical byte representation. It binds normalized
exact prices, sorted entities, labels, sources, limitations, scenario and
calculation-policy metadata. Historical binding also includes all matrix values
and the reviewed cell ledger. Paths, raw whitespace and numeric spelling do not
affect identity. Changing an unused historical value still changes the snapshot.

Exports distinguish raw-file digest, effective content digest and snapshot ID.
The [CLI documentation](offline-cli.md) specifies exact canonical field order and
byte encoding; independent Python canonicalization and `hashlib` check it.
Canonical `contract-v1`/`evaluation-v1` serialization is unchanged. Full contract
IDs passed back to `eval` must match the imported snapshot and scenario.

Actual replay identities:

```text
source_sha256:
b7962f66689f395cb6da0858f1d69ff0035752d11aacebc4df458d786f489d41
content_sha256:
eb585dd73d0495a08d584d9a2dc093bd37f7135666b6432b14d5bb24c5ab5267
snapshot_id:
957e7e50af90409e4153cb769c9dc00778cd872905b7ca3823cf5fbf307df37c
```

The new cell ledger reconstructs every projected address, exported rounded value,
formula/reference lineage, role and quote kind from the preserved workbook.
Shared formulas are expanded relative to their anchors; formulas are not executed.
Its explicit padding role map is independent of current prices: rows 107,108,121,
and fifth `mov` slots at rows 78,86,94,102 for BREAKOUT, CHROMA, CHROMA 2, SHADOW,
WINTER OFFENSIVE. Changed values cannot turn an absent slot into an outcome.
A genuine modeled value changed to zero retains its outcome and probability.

Ledger totals across all 3,190 source cells:

| Classification | Cells |
| --- | ---: |
| Cached recorded observations with acquisition/time unknown | 2,260 |
| Spreadsheet minima | 696 |
| Composite averages | 8 |
| Ratio-derived estimates | 12 |
| Explicit structural padding | 214 |

The checked-in ledger is reconstructed metadata required by the application, not
a modification of the archive. Its byte SHA-256 is
`06f0bff048a096d2ea13c85bc28b2fe1ada97b3d7e0964da2c2f8c744e9152e4`.
The reconstruction tool checks the preserved evidence-file hashes before using
the fixed mapping, and `--check` verifies the exact committed ledger.

Matrix prices are preserved without replacing them with unrounded workbook
values. Thirteen cached values need cent rounding to match the export. Unchanged
prices retain known derivations; replacement prices get unknown provenance,
original-lineage context and an edited-data limitation. `replay` refuses edited
matrices. Currency, actual quote timestamp, exact floats and listing quantities
remain unknown/null. Git/archive dates never stand in for market timestamps.

## 4. CLI architecture and commands

| Layer | Implementation |
| --- | --- |
| Exact value types and engine validation | Unchanged reference library |
| Optimized search/full evaluation | Unchanged optimized library |
| SHA-256 | `src/sha256.cpp` |
| Bounded file parsing, explicit catalog, identity | `src/catalog_io.cpp` |
| Historical mapping and reconstructed provenance | `src/historical.cpp`, cell ledger |
| Allocation parsing, result selection/ranking, bounded exports | `src/application.cpp` |
| Text/JSON/CSV representation | `src/serialization.cpp` |
| Arguments, command dispatch, exit statuses, Windows wide arguments | `src/cli.cpp` |

`scenarios`, `profiles`, `inspect`, `eval`, `replay`, and `search` form the command
surface. `--help` documents defaults, limits and exact numerical syntax. Unknown,
duplicate, irrelevant, malformed or unsupported options receive diagnostics.
There are no interactive menus. Windows paths with spaces and Unicode work through
wide argument conversion and native filesystem paths.

No spreadsheet positions enter the generic search engine. There are no external
frameworks, worker threads, accounts, network calls, databases or GUI components.
Normal building/running does not need a workbook parser or Python; verification
uses only the existing Python standard library.

## 5. Historical replay

The actual import/evaluation path reproduces the first saved 2024 result in
`StatTrak FT MW mov`:

- `b002`: BREAKOUT higher MW, five inputs at `41/100` each.
- `b029`: BROKEN FANG lower FT, five inputs at `14/25` each.
- Four BREAKOUT slots and five BROKEN FANG slots; 45 tickets, probability `1/9`
  for each of the nine declared outcomes.

| Field | Exact value | Approximate human display |
| --- | --- | ---: |
| Input cost | 97/20 | 4.850000 |
| Gross EV | 6427/900 | 7.141111 |
| Net EV | 186383/30000 | 6.212767 |
| Expected profit | 40883/30000 | 1.362767 |
| Capital-inclusive return % | 186383/1455 | 128.098282 |
| Expected ROI % | 40883/1455 | 28.098282 |
| Worst modeled net receipt | 348/125 | 2.784000 |

The modeled worst receipt is below input cost, so this is not profit under every
modeled outcome. It also does not establish exact float feasibility, listing
availability, real-game basket completeness or guaranteed actual profit.
`historical-quote-tickets-v1`, the historical proportional 13% fee policy, and
`historical-higher-wear-quote-basket-v1` remain explicit.

## 6. Filtering, ranking and result inspection

Return/ROI thresholds use strict exact comparison. Default return `>115` means
expected ROI `>15`; equality is excluded. `--min-return` and `--min-roi` are
mutually exclusive. `--max-cost` inclusively filters retained results **after**
search; no unproved early cost/float filter was added.

Ranking uses exact ROI or expected profit descending, then canonical identity
ascending. Equal-return results remain distinct. `--top` limits human display
only; all matching retained results remain available in machine output. Metadata
distinguishes retained, matching, excluded and shown counts. A full canonical ID
or its allocation list can be replayed through `eval --details` for probabilities
and provenance. A single evaluation does not assert qualification without a query.

## 7. Export formats and numerical representation

JSON and CSV are UTF-8 and retain exact normalized `numerator/denominator` strings
for monetary values, probabilities, return and ROI. Only human presentation uses
six-place rounded approximations, calculated by integer long division without
overflowing a remainder-times-ten intermediate. No float affects qualification,
ranking, identity or exported numerical precision.

JSON contains dataset/source identities, scenario/policies/limitations, query,
completion, limits, observed counters, selection and ranked results. Results
contain canonical IDs, inputs/quantities/provenance and all seven exact value
fields. Quote kinds/estimate warnings are exposed without requiring full outcomes;
`--details` adds every modeled outcome's identity, probability, quote and net
receipt. Single evaluation's `retained_qualifying` is null.

CSV has a fixed rectangular header and quoted CRLF records. Its first data record
is `metadata` with the same report metadata encoded as JSON; subsequent `result`
rows contain exact values and nested JSON allocations, optional outcomes and
limitations. This supports complete-empty and interrupted-empty exports without
losing status. CSV readers must retain the metadata row.

Output files must be new and their parent must exist. Existing files are never
overwritten; historical and `.git` path components and resolved protected paths
are rejected. Serialization is byte-bounded, written to an exclusive temporary
file, closed, then published as a new same-directory hard link. Failed writes or
budget overruns remove the temporary and leave no partial published report.
Filesystem hard-link support is required; failures are reported explicitly.

Exact result identities/order are reproducible. Actual source paths, raw digests
for reformatted files and elapsed diagnostics may differ. Time-limited prefixes
depend on scheduling; deterministic count/work limits are preferable when a
repeatable prefix is required. No fabricated timestamp is added to exports.

## 8. Limits and incomplete results

| CLI budget | Default | Maximum |
| --- | ---: | ---: |
| Evaluation attempts | 100,000 | 10,000,000 |
| Work units | 500,000 | 50,000,000 |
| Retained hits | 10,000 | 100,000 |
| Hit backing bytes | 16 MiB | 64 MiB |
| Cooperative search time | 2 seconds | 60 seconds |
| Output bytes | 32 MiB | 256 MiB |

Input limits are 1 MiB for the original matrix, 8 MiB for explicit catalogs, 16 KiB
per noncomment catalog record, 4,096 buckets/collections and 100,000 modeled
outcomes. Ranking stores indices; full outcomes are materialized one result at a
time. These are bounded resource policies, not hard process-RSS or real-time caps.
Preparation and output lie outside the cooperative search timer.

Only `complete` asserts full coverage of the declared restricted domain. Other
statuses distinguish evaluation, work, time, storage and arithmetic limits, with
the original engine counters preserved. An unrepresentable domain count is null.
Storage exhaustion explicitly exposes the first unretained qualifier through
`qualifying_found`. A filtered subset cannot make an interrupted run complete.

Exit codes are 0 success (including complete empty search), 2 invalid input/options,
3 budget interruption, 4 I/O/export failure, 5 unsupported arithmetic, and 6
allocation failure. Preparation/evaluation arithmetic failures are explicit.
The current 58-bucket domains have 80,723,385 and 78,280,280 vectors for 5/5 and
6/4; this milestone does not claim exhaustive mixed historical searches.

## 9. Verification and adversarial results

Direct builds passed in **Debug and Release**, using MSYS2 UCRT64 GCC 14.2.0 Rev2,
C++20, `-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror`. Debug adds
`-O0 -g -D_GLIBCXX_ASSERTIONS`; Release adds `-O2 -DNDEBUG`. Python is 3.10.11 on
Windows 10 build 19045. Counts below describe one configuration, not doubled runs.

| Suite | Actual result |
| --- | --- |
| Existing C++ baseline examples | 9 checks passed |
| Existing C++ reference suite | 36 named tests passed |
| Existing C++ optimized suite | 28 named tests passed; 29,173 canonical-vector comparisons; 105,612 detailed evaluations; 384 feature searches |
| Existing Python preservation suite | 14 groups passed; 33 historical hashes and all 3,190 rounded workbook values matched |
| Existing Python reference suite | 8 groups; 18 Cartesian cases / 1,249 vectors; 6 fixtures, 40 generated evaluations and 6,000 rational operations passed |
| Existing Python optimized suite | 11 groups; 46 Cartesian cases / 2,931 vectors, 24 large-catalog evaluations, existing fixtures/generated cases and rational operations passed |
| New application C++ suite | 10 named tests passed, including four SHA-256 vectors and extreme decimal-display checks |
| Workbook metadata check | All 3,190 cell records matched reconstructed preserved evidence |
| Final independent CLI suite | **25 groups passed; 181 application invocations** |

The CLI suite independently checks all 3,190 imported prices, all twelve scenario
maps, all 696 scenario input quotes and 1,140 selected scenario outcome quotes.
It compares twelve historical contract evaluations against Python `Fraction`,
and both historical 1,000-evaluation prefixes: **172 retained contracts** (53 under
5/5, 119 under 6/4), with all exact values and full probability distributions.
These are checked prefix results, not a proof of complete historical coverage.

Synthetic independent Cartesian tests cover **32 ranked queries**, **1,358 candidate
positions**, **278 qualifying evaluations**, and **1,304 outcome probability/price/
receipt comparisons**. Totals include both ranking modes, not globally distinct
contracts. The explicit equal-return case retains all 405 5/5 contracts and 390
6/4 contracts. Other cases cover a seven-outcome collection including legitimate
zero, missing values, composite/ratio/minimum provenance, strict threshold
neighbors, all completion statuses, stale identities, post-search filters,
JSON/CSV round trips, output failures/cleanup, and spaces/Unicode in Windows paths.

Adversarial review checked row offsets, even output columns, collection aggregation,
padding independent of current values, snapshot changes for unused edited rows,
raw/effective hash distinction, delimiter-safe UTF-8 canonicalization, identical
returns, incomplete exports and storage counters. No engine/reference regression
or unresolved input-to-output discrepancy remained.

A provenance test initially assumed row102 sorted last; inspecting the actual
shared formula and ledger showed the annotation was correct and lexical ID order
caused the test's positional mistake. The test now selects the evidenced slot ID,
without changing the expected composite classification or old fixtures. Review
also corrected a misleading single-evaluation qualifier counter to null and
added its boundary regression. GCC caught deprecated `u8path`; conversion now
uses C++20 `u8string` native-path construction.

CMake/CTest were checked again in PATH and standard CMake/MSYS2/Visual Studio
locations and remain unavailable. Nine CTest entries and separate application
targets are supplied, but **configuration/CTest execution is unverified**. Actual
AddressSanitizer/UndefinedBehaviorSanitizer link probes still fail for missing
`-lasan`/`-lubsan`; **no sanitizer execution is claimed**. No software was installed.
The unchanged Milestone 3 benchmarks remain the performance evidence; no new CLI
speedup claim is made from demonstration runtimes.

Executed complete verification commands:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\verify_cli.ps1 -Configuration Debug
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\verify_cli.ps1 -Configuration Release
```

After the final adversarial prefix test was added, the final 25-group CLI suite
was rerun against both built configurations. The updated metadata reconstruction
check also passed unchanged. Existing expected fixtures and engine sources were
not modified. Execution-policy bypass affected child processes only.

## 10. Actual end-to-end demonstrations

The following PowerShell-compatible commands were executed with the Release
application. Generated files/logs live under ignored `.dist/milestone4/`.
Choose fresh output names if repeating them, since overwrite is intentionally
refused.

```powershell
$app = '.\.dist\milestone4\tradeup_Release.exe'
$matrix = '.\historical\2024\most recent version i think\input.txt'
& $app replay --input $matrix --details --json `
    --output '.\.dist\milestone4\demo-replay.json'
```

Observed exit 0, with exact values listed in section 5 and nine probability `1/9`
outcomes. Selected buckets were BREAKOUT higher MW and BROKEN FANG lower FT.

```powershell
& $app search --input $matrix --profile '5/5' --max-evaluations 1000 --top 3 `
    --output '.\.dist\milestone4\demo-historical-prefix.json'
```

Observed exit **3**, with actual output:

```text
Search status: evaluation-budget (INCOMPLETE; retained results are partial)
Domain: 80723385; considered: 1000; evaluated: 1000; proved pruned: 794
Qualifiers found: 53; retained: 53; matching post-search filter: 53; shown: 3
Work: 3799; result capacity bytes: 12800
Rank  Expected ROI%  Expected profit  Input cost  Bucket quantities
1  45.938545  1.263310  2.750000  b024:5,b043:5
2  45.662857  1.438380  3.150000  b040:5,b043:5
3  37.602899  1.297300  3.450000  b002:5,b043:5
```

All 53 matching retained contracts were exported, not just the three displayed.
Estimate/float/supply limitations accompanied the output; these are historical
modeled prefix results, not global rankings or actual purchasing opportunities.

```powershell
& $app search --input '.\examples\synthetic-equal-returns.tsv' --format catalog-v1 `
    --min-roi 15 --max-cost 10 --sort profit --top 3 `
    --output '.\.dist\milestone4\demo-synthetic-results.csv'
```

Observed exit 0: domain/evaluated/retained/matching **405**, pruned **0**, shown **3**.
CSV contained one metadata record and all **405** distinct result rows. Every
synthetic result had expected ROI `770/1`, expected profit `77/1`, cost `10/1`;
canonical identity broke ties deterministically. These invented values are labeled
synthetic and do not represent historical quotes.

```powershell
& $app search --input '.\examples\synthetic-boundary.tsv' --format catalog-v1
```

Observed exit 0:

```text
Search status: complete (complete for selected profile)
Domain: 1; considered: 0; evaluated: 0; proved pruned: 1
Qualifiers found: 0; retained: 0; matching post-search filter: 0; shown: 0
No qualifying contracts in the complete selected search domain.
```

Its exact return is 115, correctly excluded by the strict default threshold.

## 11. Exact modified/new files

Modified existing files:

```text
CMakeLists.txt
README.md
```

New files:

```text
include/tradeup/application.hpp
include/tradeup/catalog_io.hpp
include/tradeup/historical.hpp
include/tradeup/serialization.hpp
src/application.cpp
src/catalog_io.cpp
src/cli.cpp
src/historical.cpp
src/historical_cells.inc
src/serialization.cpp
src/sha256.cpp
tests/application_tests.cpp
tools/build_cli.ps1
tools/reconstruct_historical_metadata.py
tools/verify_cli.ps1
tools/verify_cli.py
examples/synthetic-boundary.tsv
examples/synthetic-equal-returns.tsv
docs/offline-cli.md
docs/milestone-4-report.md
```

There are **22 intended source/documentation changes**. The reconstructed `.inc`
is a reviewed required source resource, with its generator/checker; transient
conversions, objects, executables, logs, demos and probe artifacts remain ignored
under `.dist/milestone4/`. Python bytecode is already ignored. Reference and
optimized source/header/tests, previous reports/fixtures and historical materials
remain unchanged.

All 33 historical files, including untracked and ignored artifacts, retain their
initial SHA-256 hashes, file set and modification timestamps. The independent
archive was never a write target. Branch/HEAD remain `modernization` /
`d3c46c7fcfb07e7ffc7a54142a54576493a203bc`; the staged diff remains empty.

## 12. Selective manual staging command

Provided for manual review only; **not executed**. This lists only the 22 intended
files and excludes historical materials and transient generated artifacts:

```powershell
$milestone4Files = @(
    'CMakeLists.txt',
    'README.md',
    'include/tradeup/application.hpp',
    'include/tradeup/catalog_io.hpp',
    'include/tradeup/historical.hpp',
    'include/tradeup/serialization.hpp',
    'src/application.cpp',
    'src/catalog_io.cpp',
    'src/cli.cpp',
    'src/historical.cpp',
    'src/historical_cells.inc',
    'src/serialization.cpp',
    'src/sha256.cpp',
    'tests/application_tests.cpp',
    'tools/build_cli.ps1',
    'tools/reconstruct_historical_metadata.py',
    'tools/verify_cli.ps1',
    'tools/verify_cli.py',
    'examples/synthetic-boundary.tsv',
    'examples/synthetic-equal-returns.tsv',
    'docs/offline-cli.md',
    'docs/milestone-4-report.md'
)
git add -- $milestone4Files
```

## 13. Suggested commit message

```text
feat: integrate historical datasets and exact bounded offline CLI
```

No staging, commit, push, merge, amend or history rewrite was performed.

## 14. Five interview decisions

1. **Freeze semantics separately from numeric values.** Explicit source-derived
   slot roles keep zero-valued outcomes distinct from padding. Mapping adapters
   handle historical row positions while the engines remain generic and unchanged.
2. **Bind identity to effective data, not file location or labels.** Versioned,
   length-prefixed UTF-8 canonicalization prevents delimiter ambiguity; SHA-256
   binds prices, source evidence, scenario and policies. Raw and effective hashes
   serve different audit purposes.
3. **Preserve exact computation through presentation.** Threshold/ranking remain
   rational, machine output keeps fractions, and decimal formatting uses checked
   integer long division. Return, ROI and guaranteed profit are not conflated.
4. **Make incompleteness unavoidable in the interface.** Status/exit codes,
   counters, budgets and retained-versus-shown counts accompany exports, including
   empty files. Filtering or top display cannot upgrade a partial search.
5. **Test the whole boundary without weakening the oracle.** Source/workbook
   mappings, input-to-engine evaluations, SHA-256 canonical bytes, budget states,
   CSV/JSON round trips, no-overwrite output and Unicode paths are independently
   challenged while all earlier engine comparisons remain intact.

## 15. Remaining limitations

The application analyzes the approved historical quote-basket abstraction only.
It does not verify current CS2 mechanics, authentic market capture dates/currency,
exact floats, item eligibility outside declared metadata, actual supply or
guaranteed profit. Composite/ratio estimates and unknown acquisition remain
limitations. Explicit catalogs are user declarations, not externally certified
datasets. Fixed historical mapping does not extend to Kilowatt or arbitrary
older six-vector files without additional evidence/schema work.

There is no general runtime XLSX importer, unrestricted search, resumable cursor,
streaming search-result sink, arbitrary-precision arithmetic, multithreading,
database, network service or trading integration. Required signed-64-bit
intermediate overflows fail explicitly. Whole-process RSS and hard real-time
limits are not promised. Full outcome formatting occurs outside the engine timer;
the export byte budget still bounds serialized output. New-file publication
requires same-filesystem hard-link support and never replaces an existing file.
Time-limited prefixes may vary with scheduling.

CMake/CTest, sanitizer runs and non-Windows compiler validation remain unavailable
or unexecuted. This is reported rather than represented as coverage. No new
benchmark result is extrapolated from the historical prefix demos.

## 16. Concrete prerequisites for Milestone 5

Explicit authorization is required before Milestone 5. Its next review should use
these exact inputs, policy versions, immutable archive manifest, complete engine
oracles, identity encoding and application regression corpus.

Obtain working CMake/CTest and sanitizer-capable environments; validate compiler
portability, Windows packaging/runtime DLL assumptions and output publication on
target filesystems. Decide supported compiler/platform versions and release
artifact layout before packaging. Review public documentation/AI-assistance
disclosure, historical-data distribution permissions/provenance, and which local
archives belong in a clean checkout. Only then finalize release documentation,
automation/CI, reproducible distribution and any additional benchmark evidence.
No release automation, deployment or unrelated feature work started here.
