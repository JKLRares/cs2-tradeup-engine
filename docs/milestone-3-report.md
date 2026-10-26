# Milestone 3 — verified C++ search optimization and performance engineering

Completed locally on the existing `modernization` branch. This milestone adds a
separate exact optimized search, differential verification, and reproducible
bounded performance experiments. It does not implement Milestone 4 or 5. No
historical file was modified, no historical binary was executed, no network was
used, and no Git staging, commit, push, merge, or history operation was performed.

## 1. Optimized engine architecture

`tradeup_optimized` owns a validated immutable catalog, dense bucket/collection
indices, wear-side index arrays, reusable collection summaries, and optional
common-scale integer values. It streams direct support/composition generation
through exact qualification and retains fixed-size compact hits. Full evaluation
and persistent identity are reconstructed on demand.

The reference implementation remains unchanged. Shared code is limited to input
validation, canonicalization, public value types, and checked rational arithmetic;
optimized search and evaluation never call their reference counterparts. This
keeps enumeration/evaluation comparisons meaningful while avoiding diverging input
validation policies. Python `Fraction` independently checks the shared numerical
foundation and both evaluators.

The [engine documentation](optimized-engine.md) gives the API, algorithms,
invariants, formulas, completeness proof, and resource semantics. There are no
external libraries, worker threads, databases, application GUI, market features,
or end-user CLI. The stream drivers and benchmark executable are internal tools.

## 2. Original ideas preserved

- Interchangeable purchases remain grouped into explicit representative buckets.
- Contracts use quantities rather than permutations of ten individual inputs.
- Increasing bucket selection removes equivalent representations and zero padding.
- Collection outcome information is reused instead of reconstructed per candidate.
- The original 5/5 and 6/4 wear profiles remain separate named restrictions.
- Historical quote-basket valuation, estimated/composite quotes, unknown supply,
  and unverified float assumptions remain visible, rather than being replaced
  with new game-rule claims.

The preserved 2023 quantity experiments and 2024 ordered-index search were read
alongside their notes and the prior reconstruction. Their unsafe loops, integer
normalization, 99-result capacity, and equal-return suppression are not reproduced.
No additional representative-price dominance deletion is applied: bucket identity
and the complete declared quantity-vector domain are preserved.

## 3. Implemented optimizations and correctness arguments

| Optimization | Why it preserves the declared result set |
| --- | --- |
| Direct positive compositions after ordered support selection | Every valid vector has one wear support on each side and one positive composition per support. Forcing the last quantity preserves all positive compositions and excludes invalid sums. |
| Streaming candidate traversal | Evaluation occurs at the same unique vector that would have been emitted; storing the entire enumeration is unnecessary. |
| Dense indices and fixed five-entry candidate storage | Indices refer to immutable sorted bucket identities. Canonical serialization still uses stable IDs and positive quantities. |
| Collection sums/counts and minimum receipts | Distributivity gives `G=sum(q*s_c)/sum(q*m_c)`; the minimum over represented collection minima equals the outcome minimum. Shared wear buckets aggregate into one collection's outcome mass. |
| Common-scale integer accumulation and reduced threshold coefficient | Exact positive scaling preserves rational values. The strict inequality `x*S_i > y*D*C_i` is algebraically identical to `R>T`. Every product is checked. |
| Rational fallbacks | Unrepresentable LCMs, scaled totals, or fast comparison products do not wrap or approximate. Required unsupported rational operations fail explicitly. |
| Root and support profitability bounds | Gross EV is a ticket-weighted average of collection means, bounded by their maximum. Positive quantities imply a proved minimum cost. `87*max_mean/min_cost <= T` excludes only contracts that cannot pass strict qualification. |
| Deferred full distributions and compact result storage | Summary qualification computes the same seven exact fields; details are reconstructed without dropping contract identities or changing probabilities. Equal-return contracts occupy separate hits. |

Pruned supports are disjoint, and their omitted composition counts are exact.
Arithmetic failure in an optional bound disables that bound; it never establishes
unprofitability. No heuristic, approximate mode, floating-point decision, bucket
dominance filter, or return-keyed deduplication was added.

The failed common-scale accumulation fallback and oversized chrono-duration guard
were added during adversarial review, before final verification. They prevent an
optional acceleration from unnecessarily rejecting representable contracts and
prevent time-budget conversion overflow, respectively.

## 4. Supported catalog sizes and profiles

Default preparation budgets allow 4,096 buckets, 4,096 collections, and 100,000
declared outcomes in total. These are configurable resource limits, not a promise
to exhaust their search domains. Outcome count is not capped at five. The general
evaluator also supports ten-input contracts outside the search profiles.

Search still requires exactly ten inputs, 2–5 active buckets, and either:

- `historical-5-5-up-to-5-buckets-v1`: five lower and five higher inputs.
- `historical-6-4-up-to-5-buckets-v1`: six lower and four higher inputs.

For `l,h` available buckets and wear totals `L,H`, the domain count is
`sum choose(l,a)*choose(L-1,a-1)*choose(h,b)*choose(H-1,b-1)` over positive `a,b`
with `a+b<=5`. Impossible terms are zero. The 58-bucket paired structure has
**80,723,385** contracts under 5/5 and **78,280,280** under 6/4. These counts were
also checked with Python's independent `math.comb` calculation. The historical
price matrix was not imported or searched.

## 5. Completeness guarantees and limits

Only status `complete` asserts that all domain contracts were evaluated or
eliminated by proved bounds, and that every qualifier was retained. Completion
checks `evaluated+pruned=domain`. A completed zero-result proof differs from an
interrupted traversal with no stored results.

Interruption statuses identify evaluation, work, time, storage, or arithmetic
limits. Partial retained hits are explicitly incomplete; there is no resumable
cursor. An unsupported domain count is marked unavailable, rather than reported
as a known zero. Preparation arithmetic failures throw before search.

These are exact guarantees within the historical declared basket, profiles, and
supported arithmetic. They establish neither unrestricted ten-input completeness
nor real-game outcome coverage, float eligibility, listing availability, or actual
profitability. Unknown quote time/currency and composite outcome assumptions remain.

## 6. Independent differential methodology

The C++ suite obtains complete canonical sets from the unchanged reference
enumerator and compares them with optimized all-contract mode. It compares every
hit's cost, gross/net EV, expected profit, return, ROI, and worst net receipt.
Full evaluations additionally compare evaluation identity, outcome identities,
exact probabilities, quote prices/kinds/sources, net receipts, and limitations.

Each bounded instance is also searched under all four combinations of pruning
and scaled arithmetic, comparing complete qualifying sets. Input permutation tests
check deterministic optimized order, independently of reference traversal order.
Generated prices are rational and collections have unequal outcome counts; wear
buckets share collection identities.

The Python oracle independently enumerates full Cartesian quantity vectors,
including zeros, filters the approved domain, and calculates with arbitrary-
precision `Fraction`. The existing numerical fixtures, including the preserved
first 2024 result `186383/1455` percent, pass through the optimized evaluator.
Additional tests exercise threshold equality/neighbors, all-zero outcomes, equal
returns, and evaluations on 58-bucket catalogs.

Budget tests cover exact completion boundaries and one below, count/byte storage
exhaustion, a cooperative elapsed-time interruption, a 58-bucket prefix, and domain
count overflow without enumeration. A zero-valued 58-bucket synthetic catalog is
independently checkable: every return is zero, so root pruning proves no qualifiers.

## 7. Actual verification numbers

The following passed in **both final Debug and Release builds**. Counts describe
one configuration; they are not doubled to inflate coverage.

| Check | Actual result per configuration |
| --- | --- |
| Existing C++ numerical/language examples | 9 passed |
| Unchanged C++ reference tests | 36 named tests passed |
| New C++ optimized tests | 28 named tests passed, 0 failed |
| Full optimized/reference canonical-vector comparisons | 29,173 contracts across bounded cases |
| Full detailed C++ evaluation comparisons | 105,612, including repeated qualifiers across feature combinations and 100 large-catalog prefix evaluations |
| Optimized feature searches compared | 384 across 96 bounded case invocations |
| Existing Python preservation suite | 14 groups passed; all 33 historical file hashes matched; all 3,190 rounded workbook prices matched |
| Existing Python reference suite | 8 groups; 18 Cartesian comparisons / 1,249 vectors; 6 fixtures; 40 seeded evaluations; 6,000 rational operations |
| Python optimized suite | 11 groups; 46 Cartesian comparisons / 2,931 vectors; 6 fixtures; 40 seeded evaluations; 24 large-catalog evaluations; 6,000 rational operations |

The 6,000 rational operations in each Python run exercise the same shared rational
implementation; they are not 12,000 distinct arithmetic cases. Contract/evaluation
totals are summed comparisons across cases, thresholds, and feature configurations,
not claims of globally distinct dataset identities. The 405 equal-return contracts
in the explicit regression are all retained.

Benchmark prechecks additionally compared complete qualifying identities and all
seven exact fields for eight identical reference/optimized workloads spanning
87,240 candidate positions. Three ablation prechecks repeated the same 20,916-
contract workload with different feature options. Timing repetitions are separate
from correctness-test counts.

## 8. Reproducible benchmarks and measured performance

Measurements used Windows 10 build 19045, Intel Core i7-9700 CPU at the reported
3.00 GHz, MSYS2 UCRT64 GCC 14.2.0 Rev2, Python 3.10.11, and direct Release flags:

```text
-std=c++20 -O2 -DNDEBUG -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror
```

Synthetic generation uses standard `mt19937` raw outputs with seed **20261026**.
Buckets alternate wear and share one collection per adjacent pair; collection
outcome counts cycle through 1–5. The generator is in
`benchmarks/search_benchmark.cpp`. Mixed prices are exact hundredths; all-qualified
and zero-output cases are explicitly different synthetic workloads. Threshold is
exactly 115 in every benchmark. No historical market observations are synthesized.

Each measurement uses five repetitions in a separate process per engine/case.
`steady_clock` measures actual elapsed search time. Dataset generation, full-set
prechecks, and printing are excluded from search timing. Constructor preparation
is measured separately. There is no CPU affinity, isolated machine, or hardware
counter claim. All five raw times and counters are saved locally in ignored
`.dist/milestone3/benchmark-results.json`; source tools reproduce them.

Both optimized output modes search the identical domain and retain the same
qualifying identities and exact values. **Compact** keeps fixed-size summaries;
**expanded** additionally constructs and retains every full qualifying outcome
distribution inside the timed region, matching the reference's eager output
responsibility. Reporting both avoids attributing all savings to traversal.

### Complete identical workloads

Times are medians in milliseconds. Candidate columns are optimized evaluated /
proved pruned; the reference evaluates the entire domain and prunes zero.

| Buckets | Profile / data | Domain | Evaluated / pruned | Qualifiers | Reference ms | Compact ms | Expanded ms | Ref/compact | Ref/expanded |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 4 | 5/5 mixed | 36 | 0 / 36 | 0 | 0.2684 | 0.0019 | 0.0021 | — | — |
| 4 | 6/4 mixed | 35 | 0 / 35 | 0 | 0.2170 | 0.0019 | 0.0019 | — | — |
| 8 | 5/5 mixed | 2,160 | 1,440 / 720 | 2 | 20.7096 | 0.7609 | 0.7565 | 27.22× | 27.38× |
| 8 | 6/4 mixed | 2,080 | 1,629 / 451 | 6 | 21.1778 | 0.7720 | 0.8148 | 27.43× | 25.99× |
| 12 | 5/5 mixed | 20,916 | 18,160 / 2,756 | 2,121 | 243.2352 | 12.5029 | 32.3427 | 19.45× | 7.52× |
| 12 | 6/4 mixed | 20,181 | 17,442 / 2,739 | 1,992 | 235.1589 | 11.4910 | 30.2783 | 20.46× | 7.77× |
| 12 | 5/5 zero outputs | 20,916 | 0 / 20,916 | 0 | 202.6027 | 0.0033 | 0.0033 | — | — |
| 12 | 5/5 all qualify | 20,916 | 20,916 / 0 | 20,916 | 236.3007 | 30.6498 | 222.9389 | 7.71× | 1.06× |

Ratios are search-time ratios of measured medians for the same mathematical
workload, not unrestricted historical performance. Trivial root-proof cases are
reported without large microsecond-scale speedup claims. In the all-qualified
case full expansion gains only about 6%; that is a small local observation, not
a broad performance promise. Building detailed result objects dominates there.

Observed search-time ranges across the five repetitions:

| Case | Reference min–max ms | Compact min–max ms | Expanded min–max ms |
| --- | ---: | ---: | ---: |
| 8, 5/5 mixed | 20.0208–22.5465 | 0.6784–0.9148 | 0.7091–0.9056 |
| 8, 6/4 mixed | 19.6449–22.7783 | 0.7485–0.8603 | 0.7768–0.8989 |
| 12, 5/5 mixed | 237.3574–246.4183 | 11.8644–24.9533 | 31.5904–32.5787 |
| 12, 6/4 mixed | 233.8395–236.2311 | 11.1593–11.8574 | 29.7092–30.9082 |
| 12, 5/5 all qualify | 235.4928–238.1419 | 30.5430–31.2006 | 222.2427–226.0435 |

The compact 12-bucket 5/5 run has a visible outlier; medians do not remove that
measurement from the raw record. Expanded and compact timings from different
processes can differ slightly even when expansion has almost no work.

Median preparation overhead, including validation and engine construction:

| Case | Reference ms | Compact engine ms | Expanded engine ms |
| --- | ---: | ---: | ---: |
| 4, 5/5 mixed | 0.0038 | 0.0055 | 0.0057 |
| 4, 6/4 mixed | 0.0026 | 0.0054 | 0.0056 |
| 8, 5/5 mixed | 0.0063 | 0.0124 | 0.0127 |
| 8, 6/4 mixed | 0.0071 | 0.0124 | 0.0119 |
| 12, 5/5 mixed | 0.0091 | 0.0196 | 0.0228 |
| 12, 6/4 mixed | 0.0103 | 0.0195 | 0.0237 |
| 12, 5/5 zero outputs | 0.0100 | 0.0110 | 0.0107 |
| 12, 5/5 all qualify | 0.0080 | 0.0166 | 0.0186 |

### Ablation: a useful optimization can still lose time

All four rows below complete the same 12-bucket 5/5 mixed domain and retain the
same 2,121 results. Constructor preparation is unchanged by per-search options.

| Pruning | Scaled arithmetic | Evaluated / pruned | Median search ms |
| --- | --- | ---: | ---: |
| On | On | 18,160 / 2,756 | 12.5029 |
| Off | On | 20,916 / 0 | 10.7161 |
| On | Off | 18,160 / 2,756 | 64.4344 |
| Off | Off | 20,916 / 0 | 69.7487 |

Exact integer qualification helps substantially. On this mixed fast-integer
workload pruning costs about 17% more time than it saves: rational support-bound
construction is more expensive than some rejected integer decisions. With
rational candidate arithmetic pruning saves about 8%. It also eliminates whole
zero-result domains and remains a small, proved, independently switchable feature.
It is retained for those measured benefits; callers can disable it for cheap
integer workloads. More elaborate pruning, subset caches, parallelism, and
heuristics were not added. A smaller candidate count alone is not a speedup.

### Larger optimized-only workloads

All use compact results and the same seed, 100,000 evaluation attempts, 2,000,000
work units, 100,000 hits, 64 MiB hit-capacity budget, and a ten-second time limit.
Times and throughput use medians; throughput divides **actually evaluated**
candidates by search seconds, never pruned domain count by time.

| Buckets / profile / data | Domain | Evaluated | Pruned | Qualifiers retained | Median ms | Evaluations/s | Status |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 16 / 5/5 / mixed | 99,520 | 86,709 | 12,811 | 12,242 | 59.4657 | 1,458,135 | complete |
| 32 / 5/5 / mixed | 3,812,096 | 100,000 | 77,879 | 39,788 | 163.1329 | 612,997 | evaluation-budget; incomplete |
| 58 / 5/5 / mixed | 80,723,385 | 100,000 | 131,785 | 35,532 | 151.3128 | 660,883 | evaluation-budget; incomplete |
| 58 / 6/4 / mixed | 78,280,280 | 100,000 | 94,089 | 48,482 | 207.9173 | 480,960 | evaluation-budget; incomplete |
| 58 / 5/5 / zero outputs | 80,723,385 | 0 | 80,723,385 | 0 | 0.0066 | not applicable | complete root proof |

Median preparation times for these five rows were 0.0283, 0.0602, 0.0755, 0.1298,
and 0.0804 ms respectively. The 58-bucket root proof is not an exhaustive
enumeration demonstration. No reference performance is invented beyond its
12-bucket search limit, and no larger-workload speedup is claimed.

## 9. Resource consumption and budgets

Default search budgets are 1,000,000 evaluation attempts, 5,000,000 work units,
10,000 hits, 16 MiB hit backing capacity, and five seconds. Positive time budgets
up to 24 hours are accepted. They are cooperative, not hard real-time deadlines.
Preparation has separate catalog limits. Benchmarks explicitly override these
defaults as described above; no uncontrolled multi-billion-candidate run occurred.

This GCC ABI uses **200 bytes per compact hit**. Actual result-vector capacity is
reported and bounded separately from count. For example, the 12-bucket mixed 5/5
run allocated 819,200 backing bytes for 2,121 hits; the 58-bucket mixed prefixes
allocated 13,107,200 backing bytes each. Allocator overhead, catalog storage,
runtime libraries, full expanded outputs, and process memory are separate.

Windows `GetProcessMemoryInfo` measured actual process **peak working set** in
separate processes per engine/case, over all five repetitions:

| Case | Reference peak MiB | Compact peak MiB | Expanded peak MiB |
| --- | ---: | ---: | ---: |
| 12, 5/5 mixed | 22.96 | 8.01 | 16.25 |
| 12, 6/4 mixed | 21.95 | 7.59 | 15.40 |
| 12, 5/5 all qualify | 107.23 | 14.70 | 104.13 |

Optimized-only peak working sets were 11.19 MiB for 16 buckets and about
20.98–21.03 MiB for the 32/58-bucket mixed prefixes. These include process/runtime
overhead and allocator retention, not just live engine allocations. Peaks are not
reset between repetitions, but no reference workload runs inside an optimized
measurement process. On platforms without this measurement the executable reports
zero, meaning unavailable; no portable memory claim follows from that value.

On storage exhaustion, status is `result-budget`; `qualifying_found` explicitly
includes the first unretained qualifier and can exceed stored count by one. Work,
evaluation, and time exhaustion expose their distinct counters. No partial result
is silently labeled complete, and no result is silently dropped due to capacity.

## 10. Arithmetic safety and overflow handling

The policy remains `historical-quote-tickets-v1`; fee identity remains
`historical-proportional-13-percent-v1`. Strict rational `R>T`, canonical contract
and evaluation serialization, gross/net EV, ROI, worst modeled receipt, and
historical output valuation assumptions are unchanged.

All numerical decisions use checked exact integers or the existing rational
representation. No compiler-specific 128-bit type, floating threshold, epsilon,
or arbitrary-precision dependency was introduced. Nonnegative integer operations,
LCMs, binomial counts, and domain totals have explicit overflow checks. Binomial
factor cancellation avoids rejecting representable counts merely because of an
avoidable multiply-before-divide step.

Optional acceleration/bound failures fall back or decline pruning. Unsupported
required arithmetic interrupts search with a diagnostic; preparation failures
throw. Reference and optimized algebra have different intermediate ranges, so
identical rejection boundaries for every mathematically representable extreme are
not promised. A summary can overflow even if individually weighted terms might
fit. This is a documented supported-range limit, never approximate output or an
assertion of completeness after failure. Standard allocation failures also throw.

## 11. Windows compilation, verification, and adversarial review

Final direct Debug flags were C++20, all listed warnings as errors, `-O0 -g`, and
`-D_GLIBCXX_ASSERTIONS`. Final Release used `-O2 -DNDEBUG`. Both complete scripts
passed. Checks do not depend on disabled Release `assert` statements.

Executed commands (PowerShell-compatible):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/verify_optimized.ps1 -Configuration Debug
powershell -NoProfile -ExecutionPolicy Bypass -File tools/verify_optimized.ps1 -Configuration Release -Benchmarks
```

Execution-policy bypass applies only to those child processes; no machine/user
policy was changed. No dependencies were installed. `-Benchmarks` is optional and
restricted to Release; the benchmark also rejects unoptimized/Debug builds.

CMake and CTest were checked in PATH and standard CMake/MSYS2/Visual Studio CMake
locations and remain unavailable. The project has separate CMake libraries,
private drivers, six registered tests when Python is present, and an opt-in
`TRADEUP_BUILD_BENCHMARKS` target. **CMake configuration and CTest execution were
not performed.** The direct-compiler path is verified and remains usable.

Real link probes with `-fsanitize=address` and `-fsanitize=undefined` failed because
the toolchain lacks `-lasan` and `-lubsan`. **No sanitizer run is claimed.** Debug
standard-library assertions and the differential/budget/adversarial checks ran.

Adversarial review specifically challenged duplicate identities, multiple wear
buckets per collection, unequal counts, zero prices, equality at strict thresholds,
tiny threshold neighbors, six/four quantities of six, missing outcomes, metadata,
result storage boundaries, domain-count overflow, time conversion, scaled-total
overflow, and deterministic ordering. No mathematical/reference result discrepancy
remained. A test originally intended to overflow used a still-representable value;
its input was corrected to exercise actual overflow without changing its expected
behavior. GCC also caught a redundant Windows `NOMINMAX` definition under `-Werror`;
the benchmark definition is now guarded. Final checks use the corrected code.

## 12. Exact changed/new source files

Modified existing files:

```text
.gitignore
CMakeLists.txt
```

New files:

```text
include/tradeup/optimized.hpp
src/optimized.cpp
tests/optimized_tests.cpp
tests/optimized_fixture_driver.cpp
tools/verify_optimized.py
tools/verify_optimized.ps1
tools/benchmark_search.py
benchmarks/search_benchmark.cpp
docs/optimized-engine.md
docs/milestone-3-report.md
```

The ignore change covers Python bytecode from the independent harness. Reference
implementation/tests, prior documentation/fixtures, and README are unchanged.
Generated objects, executables, logs, benchmark JSON, probe files, and initial
preservation hashes are under ignored `.dist/milestone3/`; Python bytecode is
ignored. They are not part of the source staging list.

All 33 historical files, including untracked/ignored artifacts, matched their
initial SHA-256 snapshot and the existing preservation manifest after verification.
The independent archive was never a write target. Branch and HEAD remain unchanged
at `modernization`, `97a396dff6c0e17bce2f35f5ecf20ae8864366d7`; the index remains
unchanged and empty of staged differences.

## 13. Selective staging command for manual review

This exact command is provided for the user; it has **not been run**. It lists
only the twelve intended source/documentation changes and excludes all historical
and generated files:

```powershell
$milestone3Files = @(
    '.gitignore',
    'CMakeLists.txt',
    'include/tradeup/optimized.hpp',
    'src/optimized.cpp',
    'tests/optimized_tests.cpp',
    'tests/optimized_fixture_driver.cpp',
    'tools/verify_optimized.py',
    'tools/verify_optimized.ps1',
    'tools/benchmark_search.py',
    'benchmarks/search_benchmark.cpp',
    'docs/optimized-engine.md',
    'docs/milestone-3-report.md'
)
git add -- $milestone3Files
```

## 14. Suggested commit message

```text
feat: add verified exact optimized search and bounded performance benchmarks
```

## 15. Technical decisions to explain in interviews

1. **Generate the mathematical objects directly.** An ordered support and positive
   compositions are a unique quantity vector. The stars-and-bars count provides
   both an independent check and exact accounting for pruned support subtrees.
2. **Separate validation, search, and detail materialization.** Sharing validation
   prevents inconsistent accepted inputs; separate traversal/evaluation and an
   independent Python oracle still challenge the optimized mathematics. Deferred
   full distributions are an explicit output-cost choice, measured both ways.
3. **Use algebra before low-level tricks.** Collection sums replace repeated
   outcome traversal, and common-scale integer inequalities replace per-candidate
   rational construction. The probability model and strict threshold do not change.
4. **Prove bounds in the correct probability model.** Gross EV is weighted by
   `n_c*m_c/D`, not `n_c/10`. Combining its upper bound with a positive cost lower
   bound is safe even when those extrema cannot occur together.
5. **Count less work only when it is cheaper.** Support pruning lost time in the
   fast-integer mixed benchmark. The measured ablation, optional switch, and
   root-proof benefit justify its limited inclusion without an invented speedup.
6. **Make resource completion part of the result type.** A supported 58-bucket
   catalog has tens of millions of domain contracts. A 100,000-evaluation prefix
   is useful evidence but cannot be described as a completed historical search.
7. **Preserve identities across presentation.** Equal return, equal distribution,
   and equal rounded output do not identify a contract. Dense indices are local
   implementation details; persistent identities still serialize stable IDs.
8. **State arithmetic and environment limits precisely.** Checked 64-bit rationals
   and fallbacks avoid undefined behavior but do not promise unlimited exact
   arithmetic. Missing sanitizer/CMake tools are reported as unavailable, not passed.

## 16. Remaining limits and prerequisites for Milestone 4

No full historical-data importer, end-user CLI, export workflow, release README
polish, CI deployment, market integration, or current-rule verification was built.
There is no resumable search, streaming export sink, arbitrary-precision arithmetic,
or multi-threaded scheduler. Large mixed domains may require different explicit
budgets and can still be impractical; the report provides no full 58-bucket mixed
coverage claim.

Milestone 4 requires explicit authorization; then define the accepted local input
schema, snapshot/content binding, scenario mappings, provenance for every quote,
and metadata distinguishing real zero outcomes from padding/missing prices. Decide
how declared composite/ratio estimates, unknown capture time/currency, wear
valuation, and unsupported float/supply assumptions are surfaced in application
input/output. Any required game mechanics beyond this historical model need their
own independently supported versioned policy.

Application output must preserve every qualifying identity, expose completion and
all counters, and visibly distinguish interrupted runs. Choose result storage or
streaming/export budgets before broad searches; define any resume requirements
separately. Use the existing reference and independent oracle corpora unchanged
when connecting an importer or application layer. Obtain CMake/CTest and sanitizer
validation in an environment where those tools are available before claiming such
coverage. Milestone 3 stops here.
