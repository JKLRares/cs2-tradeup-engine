# C++20 reference engine

The library implements the [mathematical model](mathematical-model.md) for explicitly bounded catalogs. It is the correctness baseline for the separate [optimized engine](optimized-engine.md); the [offline CLI](offline-cli.md) handles data import and presentation. The reference library has no I/O or market integration, and historical programs are never build targets.

## Architecture and API

`include/tradeup/reference.hpp` declares the value types and public API. `src/reference.cpp` implements them. The library uses owned values, vectors, standard algorithms, and exceptions; it has no third-party dependencies or I/O.

| Component | Responsibility |
| --- | --- |
| `Rational` | Exact normalized signed fractions, checked arithmetic, parsing, and comparison. |
| `CollectionId`, `BucketId`, `OutcomeId` | Distinct identity types to prevent accidental interchange. |
| `Quote`, `Outcome`, `Collection`, `InputBucket`, `Dataset` | Explicit modeled catalog, costs, outcome values, provenance, and limitations. |
| `ReferenceEngine(Dataset)` | Own, validate, and sort a copy of the catalog. Caller data cannot mutate the engine. |
| `canonical_contract(span<Allocation>)` | Validate ten inputs, sort bucket identities, and merge split entries for one bucket. |
| `evaluate(Contract)` | Validate context and calculate the complete modeled outcome distribution and return values. |
| `enumerate(profile, limits)` | Generate each eligible bounded quantity vector exactly once. |
| `search(profile, threshold, limits)` | Evaluate every enumerated vector and retain every strictly qualifying contract. |

The evaluator accepts any valid ten-input contract with 1–10 active buckets. It intentionally does not enforce a search profile: a six/four or single-bucket mathematical fixture must remain evaluable. The enumerator separately enforces the selected split and the 2–5 active-bucket restriction.

A dataset's scenario fixes one rarity/variant/wear valuation context and a common price unit. The library consumes that declared model; it cannot independently prove item eligibility, authentic quotes, or actual game mechanics from user-assigned IDs.

## Mathematics and calculation identity

For each represented collection, add its input quantities across wear buckets into `n_c`. There is one outcome entry per `(collection, outcome)` identity, independent of how many input wear buckets refer to that collection.

```text
D = sum_c n_c * |O_c|
P(c,o) = n_c / D
C = sum_b q_b * unit_cost_b
G = sum_(c,o) P(c,o) * gross_price_(c,o)
N = 87/100 * G
expected_profit = N - C
return_percent = 100 * N / C
roi_percent = return_percent - 100
worst_net = min_(c,o) 87/100 * gross_price_(c,o)
```

The probability policy ID is `historical-quote-tickets-v1`. The fixed fee ID is `historical-proportional-13-percent-v1`. The dataset supplies a separate output valuation policy ID. A threshold is a nonnegative rational return percentage; the default is 115 and comparison is strictly `>`, with no epsilon or display rounding.

Every event exposes exact probability, gross quote with provenance, and net receipt. Return values and worst-case values are for the declared quote basket. They do not establish exact float feasibility, purchasable listings, or profit over every real-game outcome.

## Validation and failure behavior

Construction rejects empty catalog/context fields, duplicate collection/bucket IDs, duplicate outcome IDs within a collection, unknown collection references, invalid enums, empty or declared-incomplete outcome sets, missing prices, negative prices, and nonpositive input costs. Outcome IDs may repeat in different collections because their identities are scoped. An explicitly priced zero-value outcome remains in the probability distribution; missing prices are represented separately with `optional` and rejected.

Both input and outcome prices are exact `Rational` values. Floating-point constructors are deleted. `Rational::parse` rejects NaN, infinity, scientific notation, whitespace, trailing text, malformed fractions, and zero denominators. Accepted forms are signed integers, `numerator/positive-denominator`, or decimals with digits on both sides of the dot and 1–18 fractional digits. Leading plus signs and negative denominator syntax are rejected. Example: `-12.125` becomes `-97/8`.

Identity tokens are 1–128 ASCII letters/digits/underscore/dot/hyphen. Delimiters used by serialization cannot appear in tokens. Unknown bucket references, nonpositive quantities, entries above ten, totals other than ten, and mismatched snapshot/scenario context are rejected. Repeated allocation entries for the same bucket are merged after the bounded total is validated. Zero padding is rejected at the public allocation boundary.

Errors are exceptions: `invalid_argument` for invalid input, `overflow_error` for arithmetic beyond supported intermediates, and `SearchLimitExceeded` for exhausted traversal/result budgets. Search returns no partial result on failure. An arithmetic error in any candidate aborts that search rather than silently dropping the candidate and claiming completeness.

## Exact arithmetic and supported numerical range

Each normalized fraction has a numerator in `[-INT64_MAX, INT64_MAX]` and denominator in `[1, INT64_MAX]`. `INT64_MIN` is deliberately excluded so negation and absolute value are defined. Zero normalizes to `0/1`.

Multiplication/division cancel common factors before checked multiplication. Addition uses denominator gcd reduction and checks both signed products and their sum. Comparison uses Euclidean continued-fraction steps rather than potentially overflowing cross products. No `__int128`, compiler extension, binary floating arithmetic, or external arbitrary-precision library is required.

The range is a bound on intermediates as well as normalized results. Some mathematically representable answers are conservatively rejected. For example, with `M=INT64_MAX`, `M/2 + M/2` has exact result `M`, but its unreduced numerator sum exceeds the supported intermediate range and throws. In contrast `(M/2)*(2/M)` cancels to one safely. This behavior is documented and tested; it never yields wrapped or approximate output. A future arithmetic replacement must retain exact threshold and error semantics or explicitly version its supported domain.

The oracle checks 6,000 exact operations, including 1,000 comparisons of fractions near signed-64-bit magnitudes, using Python's independently implemented arbitrary-precision `Fraction`. Boundary regressions also test parse overflow, negative limits, cancellation, overflow errors, and tiny changes on either side of the strict return threshold.

## Exact search domain and resource bounds

Two named profiles share the same traversal:

| Profile ID | Lower-category inputs | Higher-category inputs | Active buckets |
| --- | --- | --- | --- |
| `historical-5-5-up-to-5-buckets-v1` | 5 | 5 | 2–5 |
| `historical-6-4-up-to-5-buckets-v1` | 6 | 4 | 2–5 |

Each contract has ten inputs, strictly positive active quantities, canonical bucket order, no repeated active bucket, and no zero padding. Repetition within a bucket is represented by its quantity. One collection may contribute both wear categories. All declared collection outcomes are included; there is no five-outcome limit.

Enumeration deliberately accepts at most **12 catalog buckets** to bound recursion depth in this reference implementation. This is separate from the five-active-bucket rule. Evaluation has no such catalog-bucket cap. Default budgets are 100,000 recursion nodes and 10,000 contracts. Callers may supply other positive budgets; exceeding either throws before a complete result is returned. A 58-bucket historical enumeration is rejected by this interface. No full historical search was executed.

Successful enumeration is complete for that finite catalog and profile. Successful search is complete for the same domain and threshold, provided all evaluations finish within the supported arithmetic range. A catalog lacking either wear side has an empty eligible domain and returns an empty result.

Output order is deterministic: collections, outcomes, and buckets are sorted by their exact IDs; traversal takes the skip branch before positive quantities in increasing order. Search preserves that traversal order rather than ranking by return. It uses no profitability-keyed set, compact grouping, Top-K, or probabilistic pruning.

## Completeness and uniqueness argument

Let the sorted buckets be `b[0..B-1]`. A traversal state is `(index, remaining_lower, remaining_higher, active)`. Initially the remaining totals are the selected profile's split and `active` is empty.

At each state, all earlier buckets have been decided, `active` contains their strictly positive quantities in ID order, and the remaining totals equal the required totals minus those quantities. The traversal either skips the next bucket or assigns it each integer quantity from one through the remaining amount for its wear side. It then advances the index. No branch can make a remaining total negative.

**Soundness.** A contract is emitted only when both remaining totals are zero and support size is 2–5. The totals therefore sum to ten, wear quantities satisfy the profile, IDs are distinct and sorted, and every stored quantity is positive.

**Completeness.** Take any eligible vector. At each bucket choose skip if its quantity is zero, otherwise choose its positive quantity. That quantity is available because the remaining total includes it. The support cap cannot eliminate this path: if five buckets are already active, an eligible vector cannot require another positive bucket. Once both totals reach zero, all remaining vector components must be zero, so early emission represents the whole vector. Thus every eligible vector is reached.

**Uniqueness.** Distinct vectors differ at a first bucket and must take different branches there. A fixed vector has only one skip/quantity choice at each position. Early emission admits just the zero suffix, with no alternative zero-padding representation. Thus each vector is emitted once.

Budget exhaustion interrupts this argument's completed traversal and throws; it never masquerades as a complete empty or shortened result.

## Contract and evaluation identity

Canonical contract serialization is:

```text
contract-v1|snapshot-id|scenario-id|b00:5,b02:5
```

The snapshot ID is supplied by the caller and must identify the bound dataset content. The library does not calculate or verify a content hash; the fixture harness uses SHA-256 for baseline cases. Stable opaque snapshot IDs are also accepted. Assigning the same snapshot ID to changed prices violates the caller's identity contract.

The serializer validates sorted distinct positive quantities totaling ten. Allocation order and split entries for the same bucket are normalized by `canonical_contract`. Changing bucket, quantity, snapshot, or scenario changes contract identity. No return percentage participates in this key.

Evaluation serialization prefixes that contract identity and adds probability, output valuation, and fee policy IDs. A change in policy can therefore preserve the contract identity while changing the evaluation identity. No `std::hash` value or floating equality is used as a persistent identifier.

Distinct qualifying contracts remain distinct even when all prices, probabilities, and returns match. A regression retains all 405 equal-return contracts in a three-lower/three-higher catalog, exceeding the old 99-entry result capacity. No compact presentation mode is implemented.

## Historical estimates and compatibility

`QuoteKind` distinguishes recorded quotes, minimum-derived prices, composite estimates, ratio estimates, synthetic data, and unknown provenance. Derived values require source provenance. Composite/ratio/unknown/synthetic annotations are surfaced in evaluation limitations; quotes retain their source and kind. Dataset limitations are copied into each evaluation, along with the explicit lack of verified float feasibility and listing supply.

Outcome sets are shared at collection level, preventing conflicting copies for the two wear buckets. `declared_outcomes_complete` concerns the declared model, not independent real-game coverage. Missing required outcomes or prices must be rejected, not renormalized away.

The six numerical baseline fixtures are checked against the compiled library, including the first preserved 2024 output. Its exact return remains `186383/1455` percent. The application imports the fixed 2024 matrix; there is no general spreadsheet converter. The historical matrix's averaged slots, price estimates, unknown timestamps/currency, and unsupported float assumptions are documented in the [historical baseline](historical-baseline.md).

## Verification design

- `tests/reference_tests.cpp`: 36 named tests for API validity, exact arithmetic, policies, identities, enumeration restrictions, deterministic order, boundary behavior, and error handling. Test checks remain active in Release.
- `tests/reference_fixture_driver.cpp`: private stream protocol used only to expose compiled-library results to the oracle; not an application importer or user CLI.
- `tools/verify_reference.py`: eight test groups. Its oracle calculates probabilities and values with Python `Fraction` and enumerates full Cartesian quantity vectors including zeros, then filters by the declared constraints. It never calls library logic to decide expected results.
- Eighteen catalog/profile comparisons cover 1,249 eligible vectors, including empty domains, both splits, six-bucket exclusion, and the historically skipped branch. Every canonical ID and qualifying result's seven exact value fields are compared.
- Forty deterministic generated evaluations cover shared collections, varying rational prices, zero-valued outcomes, and outcome counts from one through eight. Six preserved mathematical fixtures are additionally checked against their exact expected values.

For a 2×2 catalog, nonnegative per-side allocations give `6*6=36` contracts. For 1×4, there are `C(8,3)=56`. For 3×3 under 5/5, there are `C(7,2)^2 - C(4,2)^2 = 441-36=405` after excluding six-active-bucket vectors. Under 6/4 the analogous count is `28*15 - 10*3=390`. These are independent count checks, not assumptions borrowed from the original loops.

Adversarial tests cover delimiter collisions, missing/duplicate outcomes, input-order perturbations, probabilities with shared collections, negative and nonfinite price text, extreme arithmetic, exact threshold neighbors, and budgets exactly at and below completion. See [verification results](verification.md#recorded-local-results) for earlier local checks and platform limits, and [benchmarks](benchmarks.md) for measured performance. Linux ASan/UBSan execution passed in both [hosted runs](verification.md#automation-and-actual-coverage).
