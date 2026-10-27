# Benchmarks

These measurements compare the reference and optimized engines on declared
**synthetic** workloads. They are not historical market-price measurements or
predictions of profitability. The original Milestone 3 run and the final
Milestone 5 rerun are separate records below; no new timings were collected
while curating this document.

## Environment and method

Both runs used Windows 10 build 19045, Intel Core i7-9700 at its reported
3.00 GHz, MSYS2 UCRT64 GCC 14.2.0 Rev2, Python 3.10.11 and these direct Release
flags:

```text
-std=c++20 -O2 -DNDEBUG -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror
```

The [C++ driver](../benchmarks/search_benchmark.cpp) generates catalogs from raw
`mt19937` outputs with seed **20261026**. Buckets alternate wear and adjacent pairs
share one collection; modeled outcome counts cycle through 1-5. Mixed prices are
exact hundredths. All-qualified and zero-output cases are distinct synthetic
workloads. Every benchmark uses the exact strict return threshold **>115**.

Both profiles require ten inputs, the selected 5/5 or 6/4 wear split, and two to
five active buckets. The [optimized-engine specification](optimized-engine.md)
defines the domain and proved pruning bounds.

Before timing, the [Python runner](../tools/benchmark_search.py) checks complete
qualifying identities and all seven exact fields against the reference for eight
workloads covering **87,240 candidate positions**. Three ablation prechecks repeat
the same 20,916-contract workload with different feature options. These are
comparison counts, not globally distinct contracts across datasets.

Each engine/case runs in a separate process containing **five repetitions**.
`steady_clock` measures search elapsed time; generation, correctness prechecks
and printing are excluded. Constructor preparation is measured separately.
There is no CPU affinity, machine isolation or hardware-counter claim.

- **Reference:** retains full qualifying evaluations and outcome distributions.
- **Compact:** retains fixed-size summaries and defers full outcome expansion.
- **Expanded:** retains the compact hits and additionally constructs/retains all
  qualifying full evaluations inside the timed region, matching the reference's
  full-output responsibility.

Ratios divide measured reference median search time by the corresponding
optimized median on the same mathematical workload. Compact and expanded modes
retain the same qualifying identities and exact values; their output costs differ.

## Original measurements (Milestone 3)

All rows below completed their declared domains. Times are median milliseconds.
Evaluated/pruned counts belong to the optimized engine; the reference evaluates
all domain contracts. Trivial root proofs are reported without speedup ratios.

| Buckets | Profile / data | Domain | Evaluated / pruned | Qualifiers | Reference ms | Compact ms | Expanded ms | Ref/compact | Ref/expanded |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 4 | 5/5 mixed | 36 | 0 / 36 | 0 | 0.2684 | 0.0019 | 0.0021 | - | - |
| 4 | 6/4 mixed | 35 | 0 / 35 | 0 | 0.2170 | 0.0019 | 0.0019 | - | - |
| 8 | 5/5 mixed | 2,160 | 1,440 / 720 | 2 | 20.7096 | 0.7609 | 0.7565 | 27.22x | 27.38x |
| 8 | 6/4 mixed | 2,080 | 1,629 / 451 | 6 | 21.1778 | 0.7720 | 0.8148 | 27.43x | 25.99x |
| 12 | 5/5 mixed | 20,916 | 18,160 / 2,756 | 2,121 | 243.2352 | 12.5029 | 32.3427 | 19.45x | 7.52x |
| 12 | 6/4 mixed | 20,181 | 17,442 / 2,739 | 1,992 | 235.1589 | 11.4910 | 30.2783 | 20.46x | 7.77x |
| 12 | 5/5 zero outputs | 20,916 | 0 / 20,916 | 0 | 202.6027 | 0.0033 | 0.0033 | - | - |
| 12 | 5/5 all qualify | 20,916 | 20,916 / 0 | 20,916 | 236.3007 | 30.6498 | 222.9389 | 7.71x | 1.06x |

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

Original median preparation overhead, including validation and engine construction:

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

## Final rerun (Milestone 5)

The same workload was rerun after recursive candidate-array accesses gained
`std::array::at` checks for the GCC `-O3` build diagnostic. The search domain,
model and identity semantics did not change. This rerun still used the **direct
`-O2` flags above**, not CMake's default `-O3` Release flags. It is not a controlled
experiment isolating the cost of those checks.

All eight complete workloads again passed their reference comparisons. Timings
are recorded independently from the original run:

| Buckets | Profile / data | Domain | Evaluated / pruned | Qualifiers | Reference ms | Compact ms | Expanded ms | Ref/compact | Ref/expanded |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 4 | 5/5 mixed | 36 | 0 / 36 | 0 | 0.2966 | 0.0019 | 0.0023 | - | - |
| 4 | 6/4 mixed | 35 | 0 / 35 | 0 | 0.2354 | 0.0019 | 0.0020 | - | - |
| 8 | 5/5 mixed | 2,160 | 1,440 / 720 | 2 | 21.6971 | 0.7299 | 0.7281 | 29.73x | 29.80x |
| 8 | 6/4 mixed | 2,080 | 1,629 / 451 | 6 | 20.3005 | 0.8493 | 0.8842 | 23.90x | 22.96x |
| 12 | 5/5 mixed | 20,916 | 18,160 / 2,756 | 2,121 | 241.7489 | 11.9772 | 34.0684 | 20.18x | 7.10x |
| 12 | 6/4 mixed | 20,181 | 17,442 / 2,739 | 1,992 | 237.3390 | 11.6964 | 30.4695 | 20.29x | 7.79x |
| 12 | 5/5 zero outputs | 20,916 | 0 / 20,916 | 0 | 203.8523 | 0.0032 | 0.0036 | - | - |
| 12 | 5/5 all qualify | 20,916 | 20,916 / 0 | 20,916 | 237.0515 | 30.8193 | 224.6271 | 7.69x | 1.06x |

The 12-bucket mixed expanded ratios were originally **7.52x / 7.77x** and became
**7.10x / 7.79x** in the rerun (5/5 / 6/4). Compact ratios were originally
**19.45x / 20.46x**, then **20.18x / 20.29x**. These are separate observations,
not an averaged or universal speedup.

All five final-rerun search samples for those mixed cases, in milliseconds:

| Profile / mode | Samples |
| --- | --- |
| 5/5 reference | 246.6222, 244.9883, 241.5936, 241.7489, 239.6716 |
| 5/5 compact | 11.5879, 11.9993, 12.6275, 11.8912, 11.9772 |
| 5/5 expanded | 34.0684, 34.3629, 34.1435, 33.2005, 32.1476 |
| 6/4 reference | 235.7785, 237.5795, 235.8411, 238.1067, 237.3390 |
| 6/4 compact | 11.2058, 12.3116, 11.5032, 11.6964, 11.6977 |
| 6/4 expanded | 31.3825, 30.8658, 30.0002, 30.4695, 30.0395 |

## Pruning and arithmetic ablations

Every row completes the same 12-bucket 5/5 mixed domain and retains the same
2,121 qualifiers. Preparation is outside the timed search. Disabling pruning or
scaled arithmetic changes performance, not the expected result set.

| Pruning | Scaled arithmetic | Evaluated / pruned | Original median ms | Final rerun median ms |
| --- | --- | ---: | ---: | ---: |
| On | On | 18,160 / 2,756 | 12.5029 | 11.9772 |
| Off | On | 20,916 / 0 | 10.7161 | 10.6552 |
| On | Off | 18,160 / 2,756 | 64.4344 | 63.6776 |
| Off | Off | 20,916 / 0 | 69.7487 | 70.0047 |

With scaled integers, pruning was about **17% slower originally** and **12.4%
slower in the rerun**, despite rejecting 2,756 candidates. Rational bound
construction can cost more than cheap integer qualification. With scaled
arithmetic disabled, pruning saved about 8% originally and 9% in the rerun.
It also proves entire zero-output domains unprofitable. Fewer candidates alone
do not establish a speedup.

## Larger optimized-only workloads

These use compact results, the same seed, 100,000 evaluation attempts,
2,000,000 work units, 100,000 retained hits, 64 MiB hit capacity and a ten-second
cooperative time limit. Counts and completion statuses agreed in both runs.
No reference speedup is claimed above its twelve-bucket enumeration limit.

| Buckets / profile / data | Domain | Evaluated | Pruned | Retained | Original median ms | Final rerun median ms | Status |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 16 / 5/5 / mixed | 99,520 | 86,709 | 12,811 | 12,242 | 59.4657 | 60.0409 | complete |
| 32 / 5/5 / mixed | 3,812,096 | 100,000 | 77,879 | 39,788 | 163.1329 | 166.6225 | evaluation-budget; incomplete |
| 58 / 5/5 / mixed | 80,723,385 | 100,000 | 131,785 | 35,532 | 151.3128 | 152.4640 | evaluation-budget; incomplete |
| 58 / 6/4 / mixed | 78,280,280 | 100,000 | 94,089 | 48,482 | 207.9173 | 213.2483 | evaluation-budget; incomplete |
| 58 / 5/5 / zero outputs | 80,723,385 | 0 | 80,723,385 | 0 | 0.0066 | 0.0068 | complete root proof |

The mixed 58-bucket rows are synthetic **prefixes**, not completed historical
searches. The zero-output row is a complete root proof without enumeration.
Candidate throughput, when calculated, must use actually evaluated candidates,
not the larger evaluated-plus-pruned domain count.

## Memory observations

The verified GCC ABI uses **200 bytes per compact hit**. The original 12-bucket
mixed 5/5 run allocated 819,200 bytes of hit backing capacity for 2,121 results;
58-bucket mixed prefixes allocated 13,107,200 bytes. These are element capacities,
not whole-process memory. Catalogs, allocator overhead and expanded outcomes are
separate.

Original Windows process peak working sets, measured with `GetProcessMemoryInfo`
in separate engine/case processes across all five repetitions:

| Case | Reference peak MiB | Compact peak MiB | Expanded peak MiB |
| --- | ---: | ---: | ---: |
| 12, 5/5 mixed | 22.96 | 8.01 | 16.25 |
| 12, 6/4 mixed | 21.95 | 7.59 | 15.40 |
| 12, 5/5 all qualify | 107.23 | 14.70 | 104.13 |

Original optimized-only peaks were 11.19 MiB at 16 buckets and about 20.98-21.03
MiB for the 32/58-bucket mixed prefixes. Peaks include process/runtime overhead
and allocator retention and are not reset between repetitions. A reported zero
on platforms without this measurement means unavailable, not zero memory use.

## Reproduction and limits

From PowerShell with GCC and Python available:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\verify_optimized.ps1 -Configuration Release -BuildDirectory build/benchmarks -Benchmarks
```

This runs the existing bounded checks and benchmark workload, saving raw timings,
counters and environment metadata in `build/benchmarks/benchmark-results.json`.
Repeating it produces a new measurement, not a reconstruction of old elapsed
times. The original raw records remain local in ignored
`.dist/milestone3/benchmark-results.json` and
`.dist/milestone5/benchmark-results.json`; neither is a build/test dependency.
The permanent tables above preserve their published evidence without requiring
access to those local artifacts.

CMake can compile the optional benchmark target, but its default GCC Release
`-O3 -DNDEBUG` configuration differs from these measured `-O2` builds. Ordinary
CTest/CI does not run performance benchmarks. See [building](building.md) and
[verification](verification.md) for commands and actual platform coverage.

Full output construction dominates the all-qualified case: the original expanded
speedup was only 1.06x. Small microsecond-scale differences and cross-run changes
can reflect scheduling/noise; the original compact 5/5 outlier remains reported.
No claim extends these ratios to all inputs, real listings, current CS2 rules or
full 58-bucket historical mixed searches. All timings here were obtained locally
on Windows; hosted CI, Linux and sanitizer execution remain pending.
