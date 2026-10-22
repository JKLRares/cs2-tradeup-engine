# Milestone 2 review report

Milestone 2 implements the C++20 reference engine authorized after review of the mathematical baseline. It stops before optimization, application development, or release work. All work uses local data and the existing `modernization` branch.

## 1. Architecture

`tradeup_reference` is an I/O-free library with one public header and one implementation file. Owned value types model collections, outcomes, input buckets, prices/provenance, allocations, contracts, and evaluations. `ReferenceEngine` validates and owns a sorted dataset copy. The evaluator, canonicalization, and bounded enumeration are separate operations.

The library uses standard containers, RAII, distinct collection/bucket/outcome ID types, enums, and checked exceptions. `Rational` supplies portable exact arithmetic without dependencies or compiler-specific wide integers. A separate private driver connects compiled-library results to the independent Python test oracle.

## 2. Implemented policies

`historical-quote-tickets-v1` aggregates collection input quantity `n_c`, computes `D=sum(n_c*m_c)`, and assigns each modeled outcome probability `n_c/D`. Multiple wear buckets from the same collection share one outcome set.

The fixed historical fee is a proportional 13% deduction, identified separately as `historical-proportional-13-percent-v1`. The evaluator returns gross/net EV, positive input cost, expected profit, capital-inclusive return percentage, ROI percentage, worst modeled net receipt, and the complete event distribution. Default qualification is exact and strict `return_percent > 115`. The dataset supplies an output valuation policy identity. No current-game rules or unverified exact-float assumptions were substituted.

## 3. Exact supported search domain

Both `historical-5-5-up-to-5-buckets-v1` and a clean `historical-6-4-up-to-5-buckets-v1` extension are implemented. They require ten inputs, the named lower/higher wear totals, 2–5 active buckets, strictly positive integer active quantities, and distinct sorted active bucket IDs. Repetition within a bucket is a quantity; zero padding is absent.

The reference enumerator accepts at most 12 catalog buckets. Default limits are 100,000 visited recursion nodes and 10,000 emitted contracts; callers may supply positive budgets. Exhaustion raises an explicit exception with no partial complete result. A 58-bucket historical search is rejected by this enumeration interface and was not executed. The evaluator is separate and accepts general ten-input contracts, including single-bucket fixtures and catalogs larger than 12. There is no five-outcome cap.

## 4. Completeness and uniqueness

Buckets are sorted once. At each index, the traversal either skips that bucket or assigns each positive quantity allowed by its remaining wear total, then advances. Stored allocations contain only positive quantities and preserve canonical order.

Every eligible vector determines exactly one skip/quantity choice at each bucket. No valid vector is cut by the five-active-bucket limit, because such a vector cannot need a sixth active bucket. When both wear totals reach zero, all remaining quantities must be zero, so emission is complete without padding. Two distinct vectors diverge at a first branch; one vector cannot reach two emissions. Successful traversal is therefore sound, complete, and unique for the declared finite domain. Search evaluates each emitted vector once and applies the exact threshold.

The full state invariant and argument are in [reference-engine.md](reference-engine.md). This claim is conditional on traversal completion and, for evaluated search results, supported arithmetic. Budget or arithmetic failure throws rather than dropping candidates silently.

## 5. Independent testing methodology

C++ tests exercise public validation, arithmetic, identity, policies, and domain boundaries. The independent Python oracle does not call production functions to derive expected results: it enumerates full Cartesian quantity vectors including zeros, filters the declared sums/support, and computes every probability and value using arbitrary-precision `fractions.Fraction`.

Comparisons check every canonical contract ID and every qualifying result's exact cost, gross EV, net EV, return, ROI, expected profit, and worst net receipt. Separate evaluation cases check each outcome's identity, price, probability, and net receipt. Deterministic seeds make generated examples reproducible. The six approved numerical fixtures are passed into the actual compiled evaluator rather than only rechecking formulas in Python.

## 6. Number of tests and actual results

The complete available suite passed in **both Debug and Release**. Counts below describe each configuration, not an inflated combined test count.

| Suite | Actual result |
| --- | --- |
| Baseline C++ examples | 9 numerical/language-semantics checks passed. |
| C++ reference tests | 36 named tests passed, 0 failed; Release checks do not depend on `assert`. |
| Python historical baseline | 14 groups passed; 33 present historical files match manifest hashes; all 3,190 rounded workbook prices still match. |
| Python reference comparison | 8 groups passed. |
| Existing exact fixtures through compiled evaluator | 6/6 passed, including the preserved first 2024 output. |
| Seeded generated evaluations | 40/40 matched exact fields and complete probability distributions. |
| Bounded enumeration comparisons | 18/18 catalog/profile cases matched, covering 1,249 eligible quantity vectors. |
| Rational differential operations | 6,000/6,000 matched Python, including 1,000 large-magnitude comparisons. |

Independent combinatorial checks include 36 contracts for a 2×2 catalog, all 56 allocations for the historically skipped 1×4 branch, 405 five/five contracts for a 3×3 catalog, and 390 six/four contracts for the same catalog.

The first build attempt surfaced an unnecessary test-vector copy under `-Werror`; it was corrected before the complete passing runs. No mathematical discrepancy was found by the independent comparison. The final adversarial review added exact threshold-neighbor, ordering, metadata, and node-budget boundary tests before both final configuration runs.

## 7. Numerical edge cases and adversarial review

Fractions use normalized signed-64-bit numerators excluding `INT64_MIN`, positive signed-64-bit denominators, and checked intermediates. Multiplication/division cancel factors; comparison avoids cross products. Nonfinite/scientific/malformed price text and implicit floating construction are rejected. Explicit zero outcome values remain possible; zero input costs and absent outcome prices are rejected.

Checked arithmetic intentionally rejects some operations whose final reduced result would fit but intermediate calculation would not. With `M=INT64_MAX`, `M/2+M/2` throws; `(M/2)*(2/M)` returns one. This is an explicit supported-range limitation, not silent overflow or approximate output. Price and quantity multiplication overflow aborts evaluation/search.

Counterexamples exercised during review include:

- Two wear buckets of one collection: probability mass is aggregated before outcome emission, with no duplicated events.
- Delimiters embedded in IDs: rejected to prevent ambiguous serialization.
- Missing, empty, duplicate, or declared-incomplete outcomes: rejected rather than renormalized away.
- Prices a trillionth above and below a qualification boundary: compared exactly, without epsilon.
- Zero-valued modeled outcomes: retained with positive probability, not mistaken for padding.
- A completed traversal's exact node/result budgets: accepted at completion and rejected one below.
- More than 99 identical-return results: all 405 distinct contracts retained.
- External allocation order and split entries for one bucket: canonicalized without creating duplicate input identities.

Historical defects remain preserved in the archive. Their modern regressions cover integer-normalized weighting, omitted allocations, zero padding, result-capacity handling, and equal-return identity. No sanitizer execution is claimed.

## 8. Canonical identity

Contract identity serializes dataset snapshot ID, scenario ID, and sorted positive bucket quantities as `contract-v1|snapshot|scenario|b00:5,b02:5`. Restricted ASCII tokens exclude separators. Canonicalization merges repeated entries for the same bucket after validating the ten-input total.

Evaluation identity adds probability, output valuation, and fee policy IDs. A different evaluation policy therefore need not change the input contract identity. The caller must assign a snapshot ID bound to dataset content; automatic content hashing is not implemented in the library. The fixture harness uses SHA-256 for the approved numerical cases. No floating equality or process-dependent `std::hash` defines identity.

## 9. Equal-return contracts

Search stores every qualifying evaluation in deterministic enumeration order. It does not key results by profitability, distribution, or rounded display text. Distinct bucket quantities/IDs remain distinct even when all modeled outcome values and probabilities coincide. Compact presentation filtering and ranking are outside this milestone.

## 10. Historical compatibility and remaining limitations

The preserved first result evaluates to exact return `186383/1455` percent, agreeing with displayed `128.098`. The model retains all earlier mathematical assumptions explicitly. Quote kinds and source fields identify minima, composite estimates, ratio estimates, synthetic values, and unknown provenance; limitations accompany evaluations. Missing required outcomes/prices are rejected.

No general historical importer or spreadsheet converter is included. Data remain dependent on declared collection/scenario metadata. Unknown currency, capture time, float feasibility, listing supply, the historical averaged outcome slots, and independently unsupported game-rule claims remain limitations. Positive EV is not a real-world profit guarantee. No historical source or binary was compiled or executed, and no full historical search was run.

## 11. Compiler and CMake verification

Windows environment: `Microsoft Windows NT 10.0.19045.0`. Compiler: MSYS2 UCRT64 GCC 14.2.0, Rev2. Python: 3.10.11. All C++ targets use C++20 with `-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror`. Debug uses `-O0 -g -D_GLIBCXX_ASSERTIONS`; Release uses `-O2 -DNDEBUG`.

`tools/verify_reference.ps1` was executed successfully for both configurations, including compilation, all C++ checks, baseline preservation checks, and independent Python comparisons. Direct script invocation was blocked by the Windows execution policy; the tested command uses a child-process-only `-ExecutionPolicy Bypass`, without modifying user/machine policy.

CMake and CTest were checked again and not found in PATH or the checked standard CMake/MSYS2 locations. Portable targets are configured for the static library, C++ tests, and private fixture driver, with four registered CTest suites when Python is available. CMake/CTest configuration and execution remain **unverified**. No software was installed, no network was used, and no sanitizer or benchmark result is claimed.

## 12. Exact files changed

Modified existing files:

```text
CMakeLists.txt
README.md
docs/mathematical-model.md
```

New files:

```text
include/tradeup/reference.hpp
src/reference.cpp
tests/reference_tests.cpp
tests/reference_fixture_driver.cpp
tools/verify_reference.py
tools/verify_reference.ps1
docs/reference-engine.md
docs/milestone-2-report.md
```

The mathematical specification change updates implementation status and links the concrete engine documentation; its approved formulas and mathematical requirements remain intact. Existing fixtures, baseline verification code, Milestone 1 report, and historical evidence documentation are unchanged. Build and temporary review output remain outside the repository.

All 33 historical files remain byte-for-byte unchanged, including untracked/ignored artifacts, and match the independent archive. No files were staged or committed. Branch, HEAD, and index remain unchanged by this milestone.

## 13. Proposed selective staging and commit

For manual use after review; this command has **not** been run:

```powershell
$milestoneFiles = @(
    'CMakeLists.txt',
    'README.md',
    'docs/mathematical-model.md',
    'docs/reference-engine.md',
    'docs/milestone-2-report.md',
    'include/tradeup/reference.hpp',
    'src/reference.cpp',
    'tests/reference_tests.cpp',
    'tests/reference_fixture_driver.cpp',
    'tools/verify_reference.py',
    'tools/verify_reference.ps1'
)
git add -- $milestoneFiles
```

Suggested commit message: `feat: add exact bounded C++ reference trade-up engine`

The list intentionally names only these eleven milestone files and includes no historical materials or generated binaries.

## 14. Five decisions to explain in an interview

1. **Represent contracts by quantities, not input permutations.** Ten interchangeable inputs have many orderings but one quantity vector. Canonical positive entries remove zero-padding duplication, and the first differing bucket gives a short uniqueness proof.
2. **Define exact arithmetic and its failure boundary.** Strict profitability thresholds can change with tiny price differences. Rational arithmetic makes the decision exact; checked bounds and explicit failure are preferable to wrapped or silently rounded results. The documented intermediate limit is a conscious portability tradeoff.
3. **Aggregate collection quantities before assigning outcome mass.** Under the selected ticket policy, per-outcome weight is `n_c/sum(n*m)`, not collection-first `n_c/(10*m_c)`. Outcome counts and shared collection identities matter; treating two wear buckets as separate stochastic collections can duplicate events.
4. **Separate contract identity from valuation and presentation.** Inputs define a contract; calculation policies define its evaluation. Two different contracts can have identical returns, so a profitability-keyed set loses valid results. The 405-contract equal-return test demonstrates the consequence.
5. **Make completeness conditional on explicit resource completion.** A bounded reference enumerator can be exhaustive and easy to audit while refusing oversized catalogs and throwing on budgets. It must distinguish a completed empty result from an interrupted search. Independent Cartesian enumeration checks a different traversal rather than reproducing the production algorithm.

## 15. Prerequisites for Milestone 3

Explicit authorization is required before optimization. Preserve this reference implementation and freeze its documented profiles, arithmetic/error semantics, identity rules, and full result comparison corpus. Any optimized engine must match canonical contract sets and exact evaluation values on the same supported bounded domain, including equal-return and boundary cases.

Before making performance claims, choose identical input/scenario/threshold/resource workloads and measure the reference bottleneck with reproducible environment details and preparation/I/O boundaries. Changes such as precomputation, allocation reduction, or pruning require an equivalence argument and differential tests. Broader historical search needs explicit limits and metadata acceptance; current-rule support requires separate evidence and versioned policies. CMake remains an outstanding environment validation task, and sanitizer coverage remains unperformed.

Milestone 3 has not started. The completed reference milestone is ready for manual review and selective commit.
