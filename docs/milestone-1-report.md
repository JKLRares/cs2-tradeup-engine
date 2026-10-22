# Milestone 1 review report

Milestone 1 supplies a reviewable historical baseline and mathematical specification. It does not implement the reference engine, optimized search, application, or release automation. Work stops at this milestone pending authorization to continue.

## 1. Historical behavior

The surviving workflow organized pasted price data with Apps Script, derived collection/wear representatives and scenario output prices in Sheets, exported a numeric matrix, and searched quantity allocations in C++. The 2024 `main.cpp` defines twelve scenarios over 29 collections and 58 adjacent wear buckets. The input contains 55 rows × 58 prices. The 2023 materials contain both 5/5 and 6/4 strategies and six-vector input formats.

Source evidence, exact scenario mappings, content-based inventory, and format compatibility are in [historical-baseline.md](historical-baseline.md). All 33 repository archive files matched their independent-archive counterparts at capture and at final verification.

## 2. Mathematical specification

For collection quantities `n_c`, modeled outcome counts `m_c`, and values `p_(c,o)`, define `D=sum(n_c*m_c)` and `P(c,o)=n_c/D`. Cost is `C=sum(q_b*u_b)`, gross EV is the probability-weighted output value, and net EV is 0.87 times gross EV under the historical proportional fee policy. Reported return is `R=100*netEV/C`; expected ROI is `R-100`. Historical qualification is strict `R>115`.

The policy is named `historical-quote-tickets-v1`. Its probabilities are normalized by construction. It is a declared quote-basket model reconstructed from source, not independently verified game mechanics. [mathematical-model.md](mathematical-model.md) defines the full constraints, arithmetic, identity, and unsupported guarantees.

## 3. Suspected defects

Confirmed static defects include negative array indexing, reading uninitialized `f[0]`, unchecked result-array capacity, and missing input validation. Bounded independent witnesses confirm lossy integer-normalized probability weights and skipped allocations. One quantity branch admits 34 of its 56 intended sum-constrained allocations; this is not a count of global search coverage. Mutating `k` also changes the outer selection traversal.

Repeated evaluations are an intentional historical behavior. Equal-return suppression is a presentation choice that cannot serve complete-results semantics. Wear splits, representatives, and higher-wear valuation are explicit restrictions/approximations. Five-slot averaging and ratio-derived quote estimates are preserved and labeled. Intent or rationale not established by evidence remains unresolved. The classification table supplies source lines and separates these cases.

## 4. Historical ideas to retain

Retain interchangeable-input grouping, minimum-price preprocessing with an explicit dominance condition, quantity contracts, increasing option order, scenario reuse, EV calculations, and fee-aware filtering. These form a clear algorithmic portfolio story when backed by precise coverage, safe representations, and verification. Replacing pointer/array hazards need not replace the original mathematical ideas.

## 5. Search-space definition

The initial proposed default is ten inputs, one scenario, five inputs in each of two selected wear categories, and 2–5 active buckets. Repetition within a bucket and both wear buckets of one collection are allowed. Outcomes may have any declared positive count. The 6/4 profile is separately specified historical scope, not part of a hidden unrestricted search.

The reference milestone will start with explicitly bounded catalogs and candidate limits. Neither the original traversal nor this baseline is claimed complete over all real trade-ups. No full 58-bucket historical search or uncontrolled performance experiment was run.

## 6. Deduplication semantics

Contract identity binds dataset, scenario, and sorted positive bucket quantities. Evaluation identity additionally binds calculation policies. Remove repeated representations of one identity. Preserve distinct contracts even when return percentages, full distributions, or displayed text coincide. Compact modes may group already-preserved results with an explicit rule, member IDs, and count.

## 7. Float and wear limitations

The preserved matrix lacks exact input floats, per-listing supply, and a supported float-mechanics policy. Category counts do not prove output wear or purchasing feasibility. A positive EV does not prove profit under every modeled outcome; the first saved result has net minimum 2.784 below cost 4.85. The basket's averaged slots also limit claims about the real outcome distribution and worst case.

## 8. Data compatibility

The 2024 numeric matrix is directly usable as a fixture with explicit row and collection metadata. Earlier six-vector parsers need a declared scenario extraction/converter. Valid XLSX workbooks need local cached-value conversion; damaged/misnamed files and partial result fragments remain evidence only. The later script/workbook includes Kilowatt; the 58-bucket C++ dataset does not.

Every exported value matches the corresponding cached workbook value after decimal cent rounding: 3,177 values agree exactly before rounding, 13 differ before rounding, and all 3,190 agree after rounding. This proves consistency of these copies, not market authenticity. For example, Revolver uses averaged outcomes, while some Covert values use `/4`, `/3.5`, and `/1.65` estimates. A future converter must preserve derivation provenance and unknown timestamps.

## 9. Independent examples

Six exact rational fixtures cover equal/unequal outcome counts, integer truncation, a strict qualification boundary, six outcomes, and a historical result. Synthetic values are explicitly marked as test data. The historical witness uses buckets 2 and 29 in `StatTrak FT MW mov`, five inputs each:

```text
cost = 4.85
gross EV = 6427/900 ≈ 7.141111111
net EV = 186383/30000 ≈ 6.212766667
return percentage = 186383/1455 ≈ 128.098281787
expected ROI ≈ 28.098281787 percent
```

The leading saved value `128.098` agrees at display precision. No unsafe historical code was run to obtain this result.

## 10. Actual verification and environment

| Check | Observed result |
| --- | --- |
| Python standard-library suite | All 14 groups passed, including six rational fixtures, matrix shape, twelve source-call mappings, costs/pair counts, full workbook projection, estimate formulas, identity witnesses, strict threshold, and bounded loop omission. |
| Local archive integrity | 33/33 byte counts and SHA-256 hashes match; all match the independent archive. This includes untracked and ignored files. |
| Tracked-only checkout fixture | All 14 groups passed with 11 tracked historical files and 22 optional local files absent. |
| Direct C++20 compilation | GCC 14.2.0 (MSYS2 UCRT64 Rev2), `-O0 -g -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror`; succeeded. |
| Compiled C++ witnesses | `PASS: 9 numerical and language-semantics checks`; checks remain active without relying on `assert`. |
| CMake/CTest | Minimal scaffold provided, not executed: CMake/CTest were not found in the available toolchain. No dependencies were installed. |
| Historical execution | None. No historical source was compiled, linked, or run. |
| Git state | Branch remains `modernization`; no staging, commits, pushes, or history changes. Existing historical untracked files remain untouched. |

Python version: 3.10.11. Windows reports `Microsoft Windows NT 10.0.19045.0`. The processor environment reports `Intel64 Family 6 Model 158 Stepping 13, GenuineIntel`; the exact marketing name/core counts were unavailable because CIM inspection was denied. These are verification details, not a benchmark environment claim. No speedup, search timing, candidate throughput, or memory measurement is reported.

Commands to reproduce the supported checks are in the root README. The suite and C++ executable only check bounded baseline examples; they do not validate an unimplemented reference engine.

## 11. Exact file changes

Ten new files comprise the milestone. No existing repository file is modified, and nothing under `historical/` or in the independent archive is changed.

```text
CMakeLists.txt
README.md
docs/historical-baseline.md
docs/mathematical-model.md
docs/milestone-1-report.md
tests/baseline_examples.cpp
tests/fixtures/historical_manifest.json
tests/fixtures/numerical_cases.json
tests/fixtures/scenarios.json
tools/verify_baseline.py
```

Build outputs and preparatory inspection files are outside the repository. The initial README states current capabilities and limitations; release usage and benchmark sections await working implementations.

## 12. Concrete next step for Milestone 2

After approval, implement a small C++20 reference library with explicit collection/bucket/outcome/quantity values, checked input and arithmetic boundaries, and the ticket/fee policies specified here. Start from synthetic bounded catalogs and these exact fixtures. Then add exhaustive quantity enumeration for the declared 5/5 domain and canonical identifiers, proving each bounded valid quantity vector is visited once. Preserve equal-return contracts. Differential optimization and a practical historical converter remain later work.

Before labeling historical evaluations, explicitly accept that the original five-slot dataset contains estimates/composites. Exact real-outcome modeling would require a separately declared catalog and multiplicities, without changing the archive or silently substituting rules. The specification proposes historical basket compatibility as the initial baseline, with no float-feasibility claim.

## 13. Git readiness

The milestone is ready for manual review and a selective manual commit of the ten listed files, with the CMake validation limitation clearly recorded. No files have been staged. Do not include existing untracked archive files or binary artifacts inadvertently.

Suggested commit message:

```text
docs: specify historical trade-up model and verify milestone 1 baseline
```

## 14. Engineering assessment and open decisions

The strongest portfolio elements are the original reduction of interchangeable inputs, the constrained combinatorial search, and the chance to demonstrate that performance optimizations preserve a precise result set. Rigorous probability modeling, explicit duplicate semantics, and reproducible small oracles will make these ideas reviewable.

Material uncertainties for the next stage are the economic rationale behind selected price divisors, exact float constraints behind wear assumptions, provenance of all saved output lines, and independent evidence for any claim of game-mechanics compatibility. Currency, capture times, and listing availability remain unknown. These do not prevent arithmetic verification of the explicitly declared quote basket; they do prevent stronger real-world claims.

Milestones 2–5 are not started. Continuation requires explicit authorization after review of this baseline.
