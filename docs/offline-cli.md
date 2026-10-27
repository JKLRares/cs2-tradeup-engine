# Offline CLI, local input and export contracts

The application separates input parsing, historical mapping, dataset validation,
content identity, unchanged search/evaluation engines, command parsing and output.
It performs no network operations. See [README](../README.md) for PowerShell build
and usage commands and [the report](milestone-4-report.md) for executed results.

## Commands

`scenarios` lists all twelve stable IDs, original labels and workbook rows.
`profiles` describes the restricted 5/5 and 6/4 domains. `inspect` loads a dataset
and lists input buckets/limitations; `--json` exposes every collection, outcome
quote and provenance. `--matrix` additionally exports all 3,190 original prices
and requires historical input with JSON stdout or an output file.

`eval --contract 'b002:5,b029:5'` evaluates ten positive inputs. Repeated entries
are canonicalized. A saved full `contract-v1|snapshot|scenario|allocations` ID is
also accepted, with snapshot/scenario validation to prevent stale-price reuse.
Evaluation may use contracts outside a search profile and asserts no search
completeness. `replay` uses the actual historical import/evaluation path and
requires all preserved values unchanged.

`search` runs the unchanged optimized engine. `--profile` accepts `5/5`, `6/4`,
or full existing profile IDs. `--min-return` is strict `R>T` (default 115).
`--min-roi` is an alternative, adding 100 exactly; return thresholds must be
nonnegative. `--max-cost` is an inclusive filter **after search**; it does not save
engine evaluation/storage budget or add a pruning rule. Ranking is exact ROI or
profit descending, then canonical identity ascending. Equal-return contracts stay
distinct.

`--top` (default 20, range 0–1,000) limits human rows only. Machine output includes
every matching retained result. `--details` adds full modeled outcome distributions.
An exported result's allocations/full ID can be passed back to `eval --details`.
All commands support `--help`; unknown, duplicate and irrelevant options fail.
Historical input defaults to `st-ft-mw-mov`; exact original scenario labels are
also accepted. Explicit catalogs carry their own scenario. No interactive menus
or source/spreadsheet edits are required.

## Historical matrix

`--format historical-2024` is the default. The UTF-8/ASCII file must contain a
header consisting of `58`, followed by exactly 55 physical data rows of 58
nonnegative integer/decimal tokens each. Horizontal whitespace, LF/CRLF and blank
trailing rows are accepted. Fractions/scientific notation, missing/extra values,
negative/nonfinite/malformed numbers, invalid UTF-8/control bytes and unsupported
numeric literals fail with location diagnostics. The maximum file size is 1 MiB.
All twelve input-cost rows must be positive.

Workbook row `r` corresponds to one-based matrix data row `r-66`. Collection order
comes from the 29-entry preserved C++ label table. Odd bucket `2c-1` is lower wear,
even bucket `2c` higher. For example, `c01` is BREAKOUT (`b001`,`b002`), `c15`
BROKEN FANG (`b029`,`b030`), and `c22` REVOLVER (`b043`,`b044`). Both wear buckets
share one collection outcome set, valued using the **even** bucket's higher-wear
column as in the source. Wear groups do not assert exact output floats or mix
input rarities. Outcome IDs such as `row82` denote spreadsheet slots, not invented
item identities; they sort lexically under the unchanged engine API.

| Scenario | Workbook cost | Workbook outputs |
| --- | ---: | --- |
| normal-ft-mw-mov | 67 | 74,75,76,77,78 |
| normal-ft-mw-roz | 69 | 71,72,73,107,108 |
| st-ft-mw-mov | 68 | 82,83,84,85,86 |
| st-ft-mw-roz | 70 | 79,80,81,107,108 |
| normal-mw-fn-mov | 103 | 90,91,92,93,94 |
| normal-mw-fn-roz | 105 | 87,88,89,107,108 |
| st-mw-fn-mov | 104 | 98,99,100,101,102 |
| st-mw-fn-roz | 106 | 95,96,97,107,108 |
| normal-ft-mw-rosu | 111 | 109,110,121,107,108 |
| st-ft-mw-rosu | 114 | 112,113,121,107,108 |
| normal-mw-fn-rosu | 117 | 115,116,121,107,108 |
| st-mw-fn-rosu | 120 | 118,119,121,107,108 |

`src/historical_cells.inc` is a reviewed reconstructed ledger for all 3,190
projected cells: original cent-rounded export value, workbook address, padding
role, quote kind and expanded formula/reference lineage. Shared OOXML formulas
are translated relative to their anchors; formulas are never executed. Check it
with `python tools/reconstruct_historical_metadata.py --check`. Normal builds/runs
use the checked-in ledger and do not need Python or a workbook parser.

The frozen projection has 214 formula-free zero padding cells. **Runtime zero
prices never determine padding.** Nonzero replacement values in declared padding
are rejected. A modeled slot changed to zero remains an outcome. Missing prices
are rejected, not changed to zero or silently removed. Kilowatt and earlier
six-vector/fragment formats are not added to the declared 58-column schema.

Unchanged quotes preserve reconstructed minima, composite averages, ratio estimates
or cached observations with workbook lineage. `recorded` means a cached numeric
observation, with acquisition/time still unknown. Averaged Revolver slots remain
one explicit historical quote-basket slot each. Thirteen workbook cached values
need cent rounding; the importer preserves actual exported prices without
recalculation. Changed prices get unknown replacement provenance plus an edited-
data limitation and different identity. Currency, quote time, floats and listing
quantities are never fabricated. `replay` refuses edited matrices; `eval` supports
them within the fixed mapping.

## Explicit tradeup-catalog-v1 interchange

Select `--format catalog-v1` for one explicitly declared scenario, with arbitrary
positive outcome counts within resource limits. The first line must be exactly
`tradeup-catalog-v1`. Other records use **literal tabs** between fields. Blank
lines and lines beginning `#` are ignored. Entity records may be reordered. IDs
use the engines' restricted ASCII token grammar; text can contain Unicode, quotes
and commas, but tabs/newlines are delimiters, not embedded escapes. Files are
limited to 8 MiB and noncomment records to 16 KiB.

| Record | Fields after its name |
| --- | --- |
| scenario | ID, display label (exactly one) |
| valuation | output valuation policy ID (exactly one) |
| unit | declared common price-unit label (exactly one) |
| collection | ID, display label |
| outcome | collection ID, outcome ID, exact gross price, quote kind, source text |
| bucket | ID, collection ID, `L`/`H`, exact unit cost, quote kind, source text, literal `representative`, display label |
| limitation | limitation text |

Prices accept exact integers, decimals or `numerator/positive-denominator`.
Quote kinds are `recorded`, `minimum_derived`, `composite_estimate`,
`ratio_estimate`, `synthetic`, `unknown`; derived kinds require source provenance.
Collections require at least one unique priced modeled outcome. Duplicate IDs,
unknown references/records, missing prices, negative outcomes, nonpositive costs
and invalid metadata fail validation. Snapshot IDs cannot be supplied/overridden.
The declared basket is not independently verified game coverage. There is no
implicit padding rule: explicit zero-price outcomes keep probability mass.

`examples/synthetic-equal-returns.tsv` and `synthetic-boundary.tsv` are visibly
synthetic examples demonstrating 405 distinct equal-return 5/5 contracts and
strict exclusion at return 115, respectively. No historical prices are invented.

## SHA-256 identity specification

Exports distinguish raw-file `source_sha256`, normalized `content_sha256` and
`snapshot_id`. Paths, raw numeric formatting and raw whitespace are not identity
inputs; normalized exact prices, scenario, metadata, policies and provenance are.
Changing even an unused historical price changes the snapshot. Raw file hashes
may differ while normalized identities agree.

Canonical bytes start with UTF-8 `tradeup-dataset-canonical-v1\n`. Append each
field as decimal UTF-8 **byte** length, colon, field bytes, with no separator:

1. Format, mapping version, source binding, scenario ID/label, probability policy,
   fee policy, valuation policy, price unit.
2. Collection count; each sorted collection's ID, label, outcome count; each sorted
   outcome's ID, normalized `n/d`, quote kind, source.
3. Bucket count; each sorted bucket's ID, label, collection ID, `L`/`H`, normalized
   cost `n/d`, quote kind, source.
4. Limitation count and lexically sorted limitation strings.

`content_sha256=SHA256(canonical_bytes)` and
`snapshot_id=SHA256(UTF8("tradeup-snapshot-v1\n")+canonical_bytes)`.
The identity version and source binding are exported. The explicit catalog binding
is empty. Historical binding is `matrix_digest:ledger_digest`, computed as:

- Matrix bytes: `historical-matrix-values-v1\n58\n55\n`, then each row-major
  normalized `n/d` and newline.
- Ledger bytes: `historical-cell-evidence-v1\n`, then each projected cell's address,
  original normalized `n/d`, kind, `padding`/`modeled`, lineage, tab-separated and
  newline-terminated. Frozen ledger fields contain no ambiguous separators.

Selected scenario/outcome IDs, labels, prices and sources additionally bind its
mapping in the dataset payload. SHA-256 is checked against empty/abc/multi-block/
million-a vectors, Python `hashlib`, and independent Python canonicalization.
Existing canonical contract/evaluation serialization is unchanged. Archive/Git
dates never stand in for quote timestamps.

## Budgets, completion and output

| Search budget | Default | CLI maximum |
| --- | ---: | ---: |
| Evaluation attempts | 100,000 | 10,000,000 |
| Work units | 500,000 | 50,000,000 |
| Retained qualifiers | 10,000 | 100,000 |
| Hit backing capacity | 16 MiB | 64 MiB |
| Cooperative time | 2,000 ms | 60,000 ms |

Preparation caps are 4,096 buckets/collections and 100,000 total modeled outcomes.
Positive budgets are required; backing bytes must hold a hit. Input byte limits,
bounded display, index-only ranking, per-result detail expansion and bounded export
avoid unrestricted accumulation. They are not whole-process RSS or real-time caps.
Preparation and output lie outside the cooperative search timer.

Only `complete` asserts domain coverage. Evaluation/work/time/storage exhaustion
and arithmetic interruption have distinct statuses. Metadata records complete
flag, domain count (or null), counters, limits, query, retained/matching/excluded/
shown counts. Storage exhaustion exposes the first unretained qualifier through
`qualifying_found`; it never silently truncates a completed set. Single evaluation
has no qualification query and reports `retained_qualifying=null`.

Exit codes: 0 success including complete empty search; 2 invalid input/options;
3 budget interruption; 4 I/O/export failure; 5 unsupported arithmetic; 6 allocation
failure. Required arithmetic failures cannot become false complete results.

`--json` writes machine JSON on stdout. `--output` writes a new JSON file, or CSV
for `.csv`; `--output-format` overrides explicitly. Inspection exports JSON only.
All money/probability/return/ROI values are normalized **fraction strings**, never
lossy decimal substitutes. Counters are exact decimal JSON integers; consumers
must parse large integers exactly. Human six-place approximations use integer
long division and never participate in decisions.

JSON contains context, policy/limitations, query, status, budgets/counters,
selection and ranked results. Each result includes IDs, selected buckets,
quantities, quote provenance and seven exact fields. Represented quote kinds and
estimate warnings appear even without `--details`; details add full outcomes.
CSV is quoted UTF-8 with CRLF records. Its first data row is `record_type=metadata`
with report context in `metadata_json`; subsequent result rows contain exact
fields and nested JSON allocations, optional outcomes and limitations. Read the
metadata row even for zero-result files to distinguish empty completion from
interruption. `--top` never reduces machine output.

Existing files are never overwritten. Output parents must exist; protected
`historical`/`.git` components and resolved protected paths are rejected. Writes
use exclusive temporary files, a default 32 MiB byte cap (maximum 256 MiB), and
atomic publication as a new hard link only after serialization/close succeed.
Competing existing destinations cannot be replaced. Failure cleans the temporary
file; filesystems must support hard links or report a clear error. Input files
are never opened for writing.

Identities, exact values and ranking are deterministic. Actual input path/raw
digest and measured elapsed time are audit diagnostics that can differ. Time-
budget prefixes depend on scheduling; count/work prefixes are deterministic on
the same arithmetic/library configuration. Choose fresh export paths on repeats.
In PowerShell 5 read saved Unicode JSON with `Get-Content -Encoding UTF8 -Raw`.
Windows wide arguments preserve spaces and Unicode in input/output paths.
