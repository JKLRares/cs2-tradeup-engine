# Exact optimized search engine (Milestone 3)

`OptimizedEngine` is a separate C++20 search implementation for the existing
[mathematical model](mathematical-model.md). It consumes owned, local, declared
catalogs. It does not import historical workbooks or claim current game mechanics,
float feasibility, listing supply, or live profitability.

## Architecture and interface

The public API is `include/tradeup/optimized.hpp`; implementation is
`src/optimized.cpp`. The reference source, header, tests, fixtures, and arithmetic
are unchanged. The optimized engine shares the reference's validated sorted
catalog, canonicalization, value types, and checked `Rational`. It never invokes
reference enumeration, search, or evaluation. The traversal and evaluator are
separate implementations. Python `Fraction` supplies independent numerical checks
where these implementations share arithmetic infrastructure.

Preparation validates the entire input, builds collection sums, means, minimum
net receipts and outcome receipts, maps buckets to dense collection indices, and
partitions bucket indices by wear. The engine owns the catalog; external mutation
cannot change a prepared search.

`search(profile, threshold, limits, options)` streams candidate quantity vectors
through an exact qualification decision. Only qualifying compact values are
retained. `contract(hit)` reconstructs a sorted canonical persistent contract.
`evaluate(contract)` independently reconstructs complete probabilities,
provenance, policy identity, limitations, and all exact values on demand. It also
accepts general ten-input contracts outside either search profile, as does the
reference evaluator.

`OptimizedHit` contains up to five dense bucket indices, quantities, and seven
exact numerical fields. Its indices belong to the immutable engine that produced
it; they are neither a persistent identity nor interchangeable between engines.
Use `contract(hit).serialize()` or the full evaluation's `serialize_identity()`
for identity. Return percentages never define equality. Distinct equal-return
contracts are retained separately.

The optional threshold defaults to the exact rational return percentage 115.
`nullopt` is a test/benchmark facility that evaluates and retains *every* domain
contract, including zero-EV contracts, and disables profitability pruning. It is
used to verify complete canonical sets. Options independently disable pruning and
scaled arithmetic for differential checks and performance ablations. There is no
approximate or heuristic mode.

## Domain and direct generation proof

Both historical profiles retain ten inputs, the declared 5/5 or 6/4 wear totals,
and 2–5 active buckets. These are restricted search domains, not unrestricted
ten-input searches. There is no reference-style 12-bucket enumeration ceiling.
Default catalog limits are 4,096 buckets, 4,096 collections, and 100,000 modeled
outcomes in total; callers can explicitly adjust them. Supporting a catalog says
nothing about whether a search over it completes within its budgets.

For wear totals `L,H`, lower/higher catalog sizes `l,h`, and support sizes `a,b`,
the exact number of contracts is:

```text
sum over a,b >= 1, a+b <= 5:
  choose(l,a) * choose(L-1,a-1) * choose(h,b) * choose(H-1,b-1)
```

An impossible binomial coefficient is zero. Positive compositions of `L` into
`a` parts number `choose(L-1,a-1)` by placing cuts between units. The count uses
checked unsigned-64-bit products with factor cancellation before multiplication.
If counting is unsupported, `domain_size` throws; search returns an arithmetic
interruption with `domain_count_available=false`, never an asserted empty domain.

Traversal first chooses `a,b`, then strictly increasing bucket supports within
each wear side, then positive compositions on each support. A composition's last
quantity is forced; earlier quantities leave at least one unit for every remaining
entry. No zero-vector products or invalid final splits are generated.

Every valid vector has a unique pair of support sizes, a unique ordered support
on each side, and unique positive compositions. Thus the generator is complete
and duplicate-free before pruning. Generated recursion depth is bounded by five
support positions and five composition positions, independent of catalog size.
Every support selection iteration and composition recursion consumes a work unit.
No entire candidate list or collection-probability table per candidate is stored.

Order is deterministic by support sizes, sorted wear supports, and ascending
composition quantities. It is deliberately not the reference traversal order.
Canonical sets and exact values agree; permuted input catalogs produce the same
optimized order. Sorting or removing buckets by profitability would change the
identity/domain and is not performed.

## Summary algebra and exact qualification proof

For collection `c` prepare `s_c = sum_o p_(c,o)` and `m_c = |O_c|`. For active
bucket quantities `q_b`, accumulate:

```text
C = sum_b q_b * u_b
S = sum_b q_b * s_c(b)
D = sum_b q_b * m_c(b)
G = S / D
N = 87/100 * G
R = 100 * N / C
```

This equals the reference's `sum_(c,o) n_c/D * p_(c,o)` by distributivity and
`n_c=sum_(b in c) q_b`. Two wear buckets in one collection contribute to its
quantity, not duplicate stochastic events. Unequal outcome counts remain in `D`;
no collection-first policy is substituted. Worst net receipt is the minimum of
prepared collection minima over active buckets. Repeating a collection does not
change that minimum. Full outcome reconstruction aggregates collection quantities
and emits each outcome exactly once in sorted collection/outcome order.

When supported, preparation finds common positive integer cost and outcome-sum
scales `A,B`. Then `C=C_i/A`, `S=S_i/B`, and:

```text
R = 87 * S_i * A / (D * C_i * B)
```

For positive threshold `T`, reduce `87*A/(B*T)=x/y` once and test the strict
integer inequality `x*S_i > y*D*C_i`. Every multiplication is checked. Zero
threshold uses `S_i>0`. Nonqualifying candidates need no outcome objects, strings,
or seven-field evaluations. Qualifying hits receive the same exact fields as
the reference; full details are available on demand.

If an LCM or conversion cannot be represented, the engine uses rational summary
arithmetic. If a candidate's common-scale accumulators overflow, that candidate
also falls back to rationals. Overflow in the fast inequality falls back to the
exact rational return comparison. No decision uses floating point; only elapsed
times use `double`. A failure of required rational arithmetic interrupts search.

## Proven profitability bounds

Write `mu_c=s_c/m_c`. Gross EV is a convex combination:

```text
G = sum_c (n_c*m_c/D) * mu_c <= max represented mu_c = U
```

At the root, cost is at least `L*min_lower_cost + H*min_higher_cost`.
For a selected support, every bucket costs at least one unit. The remaining
`L-a` and `H-b` units cost at least their wear side's cheapest selected price:

```text
C_min = sum_selected u_b
      + (L-a)*min_selected_lower u_b
      + (H-b)*min_selected_higher u_b
```

Since all costs are positive, `R <= 87*U/C_min`. If this bound is **less than or
equal** to the threshold, every represented contract fails the strict threshold.
The root can eliminate the entire domain. A support can eliminate exactly its
product of positive-composition counts. Disjoint supports and their exact counts
ensure accounting does not double-count eliminated vectors.

The bound can be loose: maximum mean and cheapest cost need not be attainable
together. That only reduces pruning. Arithmetic overflow while calculating an
optional bound increments `unavailable_bounds` and leaves the branch searchable;
it never licenses pruning. No bucket dominance deletion, return deduplication,
unproved wear feasibility restriction, or approximate qualification is used.

## Resources and completeness

Defaults per search:

| Budget | Default |
| --- | ---: |
| Candidate evaluations attempted | 1,000,000 |
| Work units | 5,000,000 |
| Stored qualifying hits | 10,000 |
| Vector backing capacity for hits | 16 MiB |
| Search elapsed time | 5 seconds |

All budgets must be positive; the byte budget must hold at least one hit. Time
budgets are bounded to 24 hours to keep chrono conversions representable. The
elapsed limit is cooperative, checked at each work unit; it is not a hard
real-time deadline. Preparation, allocation latency, and scheduling are not
preempted. Preparation is separately bounded by catalog counts and measured in
benchmarks. Resource allocations can still raise standard allocation exceptions;
these cannot produce a completed result.

Result capacity grows within the configured count/byte ceiling on the verified
standard library; `result_capacity_bytes` reports actual `vector.capacity()`
times `sizeof(OptimizedHit)`. This is a backing-element storage budget, **not** a
process RSS cap: catalog strings, summaries, allocator overhead, and executable
memory are separate. No outcome distributions are stored per search hit.

The result exposes independent counters:

- `domain_contracts` and `domain_count_available`: mathematical domain size,
  independent of what was traversed.
- `candidates_considered`: evaluation attempts, including a failing arithmetic
  attempt; bounded by `max_evaluations`.
- `candidates_evaluated`: completed qualification/value calculations. Pruned
  contracts are not included.
- `candidates_pruned`: exact number eliminated by the proved bounds.
- `qualifying_found`: qualifiers observed, including the first hit that exceeds
  storage if interruption occurs there; retained count is `qualifying.size()`.
- `supports`, `work`, `integer_decisions`, and `unavailable_bounds`: diagnostics
  with different meanings, not interchangeable candidate counts.

`complete` means the whole selected profile was accounted for with supported
arithmetic, and every qualifying contract was retained. A runtime invariant checks
`evaluated+pruned == domain_contracts` on completion. Empty complete results differ
from interrupted searches. The other statuses are `evaluation-budget`,
`work-budget`, `time-budget`, `result-budget`, and `arithmetic-limit`. Partial
hits remain inspectable, but are never a complete qualifying set. Storage
exhaustion is explicit: `qualifying_found` can exceed retained size by one and the
status requires treating the search as incomplete. No silent truncation or resume
cursor is provided.

## Arithmetic limits and verification

The existing normalized signed-64-bit rational representation, strict threshold,
fee identity, quote-ticket policy, canonical serialization, and historical
valuation metadata remain unchanged. Checked operations may fail even if a final
mathematical value could fit after different algebraic cancellation. Summary
preparation may also overflow before search; it throws explicitly. Constructors
reject invalid input identically through shared validation. Optimized and reference
arithmetic intermediates differ, so their sets of unsupported *intermediates* are
not asserted identical. Every reported complete evaluation is exact; tests cover
overflows, successful cancellation, fast-path fallback, and unavailable bounds.

`tests/optimized_tests.cpp` compares complete reference sets and all seven value
fields; detailed evaluations additionally compare all outcome identities,
probabilities, prices, receipts, policies, provenance, and limitations. All four
pruning/scaling combinations are compared on the same bounded cases. Tests include
both profiles, shared collections, unequal outcome counts, exact threshold
neighbors, equal returns, zero outcomes, empty domains, tight bounds, all budgets,
determinism, reference-limit catalogs, and independently checkable 58-bucket cases.

`tools/verify_optimized.py` reuses the independent Cartesian/Fraction oracle and
adds adversarial cases; it does not use production enumeration for expectations.
`tools/verify_optimized.ps1` preserves direct GCC builds and runs the complete
previous suite before the new checks. Optional `-Benchmarks` builds/runs bounded
Release measurements. CMake adds separate optimized/test/benchmark targets but
does not compile historical sources. See [the report](milestone-3-report.md) for
executed checks, actual numbers, performance, environment limitations, and manual
selective staging instructions.
