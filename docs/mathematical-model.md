# Mathematical specification for the first reference milestone

Status: approved mathematical baseline, now implemented by the Milestone 2 reference library. [Reference engine documentation](reference-engine.md) defines its concrete API, arithmetic limits, enumeration proof, and verification. This is a versioned model reconstructed from local historical evidence, not an independently certified statement of game rules. Current CS2 mechanics are outside scope.

## Domain and input meaning

A dataset snapshot defines stable collection IDs, input bucket IDs, scenario IDs, and modeled outcome IDs. A scenario fixes one input rarity transition, normal versus StatTrak, two selected input wear categories, output valuation, and a common price unit. Currency and quote time remain unknown for the historical snapshot.

An input bucket `b` belongs to collection `c(b)` and one selected wear category. Its representative unit cost is `u_b`. All inputs in the scenario share the same input rarity and variant. A contract is a nonnegative integer quantity vector `q`, with:

```text
sum_b q_b = 10
q_b >= 0, integral
active(q) = { b : q_b > 0 }
```

Repeated use of a bucket is permitted. Quantities represent modeled purchases, not proof that ten suitable listings exist. Two wear buckets from the same collection are permitted; their quantities contribute to the same collection's outcome mass.

For collection `c`, define:

```text
n_c = sum_{b: c(b)=c} q_b
O_c = its declared modeled outcome set for this scenario
m_c = |O_c|, a positive integer
p_(c,o) = gross sale valuation of modeled outcome o
```

Outcome sets are unique by `(collection_id, outcome_id)`. The two wear buckets of one collection must not create duplicate stochastic outcomes. Collections may have any positive modeled outcome count; five is a historical storage constraint, not a mathematical limit.

For historical compatibility, `O_c` initially means the non-padding quote slots from the declared scenario. Some slots are composite estimates, notably the averaged Revolver slot. They must be labeled as such. This makes the computation exact for a stated quote basket; it does not make the basket an exact real-game outcome distribution.

## Probability policy: `historical-quote-tickets-v1`

Each input contributes one ticket for every declared modeled outcome of its collection. All tickets are equally likely. Consequently:

```text
D = sum_c n_c * m_c
P(c,o) = n_c / D                     for o in O_c and n_c > 0
P(collection c) = n_c * m_c / D
sum_c sum_{o in O_c} P(c,o) = 1
```

The normalization proof is immediate: the numerator sum is exactly `D`. There is no need to divide quantities by a minimum, and no intermediate integer truncation is allowed. Scaling all ticket weights by the same positive factor would preserve probabilities; truncating different quantities does not.

This is the algebra implicit in `main.cpp:81–92` after removing lossy integer normalization. It differs from first choosing a collection with probability `n_c/10`, then an outcome uniformly within that collection. That alternative would assign `n_c/(10*m_c)` per outcome. It agrees when all represented collections have the same outcome count, but otherwise defines a different model. Do not silently substitute it.

Example: six inputs from a two-outcome collection and four from a three-outcome collection give `D=24`. Each first-collection outcome has probability `6/24=1/4`; each second-collection outcome has `4/24=1/6`. Total collection mass is 1/2 each. Under collection-first weighting it would be 3/5 and 2/5 instead.

External verification of which historical game rules apply is separate work. The baseline establishes what the surviving implementation was attempting to calculate and supplies a coherent explicit policy for reviewing that calculation.

## Cost, expected value, fee, and qualification

Use one consistent price unit throughout a dataset:

```text
C = sum_b q_b * u_b                         input cost; require C > 0
G = sum_c sum_o [ n_c / D * p_(c,o) ]       gross expected sale value
F(p) = 87/100 * p                           historical proportional net receipt
N = sum_c sum_o [ n_c / D * F(p_(c,o)) ]
  = 87/100 * G                              under this linear fee policy only
E_profit = N - C
R = 100 * N / C                             net return percentage, capital included
ROI = 100 * (N - C) / C = R - 100           expected net profit percentage
```

The source's `profit1` means `100*G/C`; `profit2` means `R`. Therefore `profit2 > 115` means expected net ROI strictly above 15%, not profit above 115%. `R=115` is excluded. `R>100` means positive expected modeled profit. Negative, zero, or invalid cost must never reach division.

The 13% deduction is a historical calculation policy, without a claim about actual platform fees, minimum fees, or sale rounding. No extra purchase fee is modeled. Keep fractional expected values until display; historical cent rounding of imported prices is separate. If a future fee policy rounds each sale, compute the sum of net outcomes directly rather than assuming `F(E[p])=E[F(p)]`.

The calculations may be checked with rational arithmetic. For common-scale integer prices, if `S=sum_c n_c * sum_o p_(c,o)`, then `G=S/D` and the >115 threshold is equivalent to `87*S > 115*D*C`. A C++ implementation must check intermediate overflow or use an adequate representation with explicit limits. Floating display equality must never define contract equality or threshold truth.

## Expected profit and profit under every modeled outcome

```text
W = min_{c: n_c>0, o in O_c} F(p_(c,o))
modeled worst-case profit = W - C
strict profit under every modeled outcome iff W > C
```

This condition differs from `N>C`. Even `W>C` would be conditional on complete outcome coverage, valid prices, successful purchases, and the stated fee and wear policies. The historical dataset cannot establish an unconditional real-world profit guarantee. Composite slots prevent treating this basket's minimum as the minimum of every underlying real outcome.

Report three different levels explicitly: preserved historical estimates, mathematically validated values under the declared quote model, and exact float-dependent values if sufficient data and a supported rules policy are later supplied. None should be described as actual purchasing availability.

## Initial bounded search domain

The proposed default historical profile is `historical-5-5-up-to-5-buckets-v1`:

| Constraint | Definition |
| --- | --- |
| Contract size | Exactly ten inputs. |
| Scenario | One declared variant, input rarity transition, two input wear categories, and output valuation policy. |
| Lower-category quantity | Exactly five. |
| Higher-category quantity | Exactly five. |
| Active bucket count | At least two, at most five. |
| Quantity per bucket | Positive integer for active buckets; at most five follows from the split. |
| Collection count | One to five, subject to available buckets; both wear buckets of one collection may be used. |
| Repetition | A bucket may supply multiple inputs; selecting it twice is represented by one summed quantity. |
| Outcomes | All declared modeled outcomes of each represented collection, without an artificial five-outcome cap. |
| Feasibility | Modeled bucket quantities only. Exact floats and actual listing supply are not asserted. |
| Qualification | `R>115` for historical comparisons; a different threshold must be an explicit query parameter. |
| Evaluation limit | Small explicitly bounded catalogs/candidate budgets for reference verification. No automatic full 58-bucket search. |

The 2023 6/4 strategy can be a separately named profile with six lower-category and four higher-category inputs and the same explicit support cap. It is not mixed into the 5/5 results. An unrestricted ten-input search would be a third, larger domain, requiring separate coverage claims.

The old program selected five indices even when some quantities were zero. The modern domain uses active buckets directly and does not require padding. This expresses the intended restriction without inheriting the loop omissions or redundant presentations. Exact reproduction of the old program's defective traversal is not a correctness target.

A future reference enumerator should select each active support in strictly increasing bucket-ID order, choose strictly positive quantities for that support, and retain exactly those allocations satisfying the declared sums. For a bounded catalog, completeness follows because every valid contract has one support and one quantity tuple; uniqueness follows from that representation. A proof and exhaustive small-instance tests belong to Milestone 2, after implementation. No full historical completeness claim is made here.

## Identity and compact results

Separate contract identity from evaluation identity:

```text
K_contract = (dataset_snapshot_id, scenario_id,
              sorted positive (bucket_id, quantity) pairs)
K_evaluation = (K_contract, probability_policy_id,
                output_valuation_policy_id, fee_policy_id)
```

Within a snapshot, bucket IDs encode the relevant collection/wear/rarity/variant and representative-choice semantics. Input order and zero padding do not affect identity. Changing a bucket, quantity, or scenario does. The snapshot binds prices and provenance; a changed price file is a different evaluation context. Exact listing contracts, if introduced later, need identities for their distinct float/price properties rather than pretending all listings are interchangeable.

Use a deterministic serialization of stable IDs and positive integer quantities for future identifiers. Do not use process-dependent `std::hash` output as a persistent ID. One proposed encoding is a version prefix, SHA-256 snapshot ID, restricted-ASCII scenario ID, and sorted `b001:5,b029:5` pairs. The actual implementation and tests are deferred to Milestone 2.

Distinguish four cases:

1. The same canonical contract reached repeatedly: evaluate or retain once.
2. Distinct contracts with equal expected return: retain both in complete results.
3. Distinct contracts with the same full outcome distribution: retain both; costs or input choices may differ.
4. Distinct contracts with the same displayed rounded text: retain both; display precision does not define mathematical equality.

Compact presentation may group equal exact return, equal distribution, or equal rounded display as separately named grouping modes. Groups must expose their rule, count, and member identities. They are formed after qualification; grouping must not erase the complete result set. Equivalent distributions include outcome identities and exact probability mass, not just equal average prices.

## Representative inputs and dominance

Replacing an input with a cheaper representative is valid for this abstraction when both choices have the same collection, scenario eligibility, wear policy, and all other properties affecting modeled outcomes. Then the replacement preserves `G` and reduces `C`, improving `R` whenever `N>0`.

That argument justifies minimum-price preprocessing in a declared interchangeable bucket. It does not prove that the cheapest historical listing had a suitable float or enough available copies. Once exact input floats affect outcomes, a more expensive lower-float item may no longer be interchangeable. Retain the historical idea with an explicit equivalence contract and provenance.

## Missing data, estimates, and wear

Every future imported value needs source-file/row-or-cell provenance and a classification such as recorded quote, copied quote, minimum-derived representative, average-derived composite, ratio-derived estimate, or unknown provenance. Unknown pricing timestamps remain null/unknown; neither archive commit dates nor screenshot dates fill them in.

A modeled outcome must have a declared identity and a valid price. An absent price must not be silently converted into zero or removed from the probability denominator. Confirmed structural padding can be omitted. If a required outcome price or eligibility mapping is unknown, reject that collection/scenario for validated evaluation, or report an explicit incomplete estimate without a validated EV label. Negative/nonfinite prices, duplicate conflicting IDs, inconsistent outcome sets for paired wear buckets, and nonpositive cost are invalid.

The historical input gives category prices, not exact floats. The chosen 5/5 or 6/4 input category counts may have been useful empirical constraints, but do not by themselves prove that all possible input floats produce the priced output categories. There is no float computation in the C++ source and no independently supported mechanics policy in this baseline.

For a future supported policy where output float is a monotone function `g_o` of average input float, known intervals `[L_b,U_b]` would imply an average-input interval bounded by `sum(q_b*L_b)/10` and `sum(q_b*U_b)/10`. A wear claim would then need every relevant `g_o` image inside the desired output interval, with correct endpoint treatment. This is a conditional mathematical method, not an assertion that such a rule or those bounds have been verified here. Exact listing floats, item-specific float ranges, eligible outcomes, wear boundaries, prices, and policy evidence are all required before exact float-dependent claims.

## Independently checked numerical cases

Values below use the ticket policy and fee 13/100. The first five are synthetic mathematical fixtures, not invented historical market records. Full exact values are in `tests/fixtures/numerical_cases.json`.

| Case | Inputs and modeled outcomes | Cost C | Gross EV G | Net return R | Implication |
| --- | --- | --- | --- | --- | --- |
| Equal outcome counts | 5×cost 1 → {10,30}; 5×cost 2 → {20,40} | 15 | 25 | 145% | Expected ROI 45%; worst net 8.7 is a loss. |
| Unequal counts, 6/4 | 6×cost 1 → {10,30}; 4×cost 2 → {20,40,60} | 14 | 30 | 1305/7% ≈186.428571% | Collection-first policy instead gives EV 28; policies differ. |
| Integer-division witness | Quantities 5,2,3, unit costs 1; outcomes {10,30}, {40,60}, {70,90} | 10 | 44 | 382.8% | Truncating q/Min produces EV 42.5. |
| Strict boundary | 10×cost 87/1000 → {115/100} | 87/100 | 23/20 | 115% | Excluded by strict >115. Symbolic rational prices, not a historical cent-price record. |
| Six outcomes | 10×cost 1 → {1,2,3,4,5,6} | 10 | 7/2 | 30.45% | Outcome count is not limited to five. |
| Preserved first output | StatTrak FT MW mov; bucket 2×5 and bucket 29×5 | 97/20 | 6427/900 | 186383/1455% ≈128.098282% | Matches displayed 128.098; expected ROI ≈28.098282%, not guaranteed profit. |

For the preserved example, BREAKOUT higher-category unit cost is 0.41; BROKEN FANG lower-category cost is 0.56. Their higher-category output slots are `{3.82,3.54,3.20,3.29}` and `{10.65,10.06,14.89,6.14,8.68}`. There are `5*4+5*5=45` tickets, with probability 1/9 for each of the nine outcomes. Thus:

```text
C = 5*0.41 + 5*0.56 = 4.85
G = (13.85 + 50.42)/9 = 6427/900
N = 186383/30000 ≈ 6.212766667
R = 186383/1455 ≈ 128.098281787 percent
W = 0.87*3.20 = 2.784 < 4.85
```

This validates one saved numerical result under the model. It does not validate every output line, unsafe memory behavior, actual supply, or exact output wear.

The Revolver averaging example exposes a separate limitation: raw lower-category quotes `{2.09,2.00,0.70,0.94,0.81,0.71}` have mean `7.25/6`; replacing the last two with their average 0.76 produces five-slot mean `6.49/5`. These differ. Preserving a group mean alone is insufficient; its multiplicity also matters. Under a six-outcome ticket model, that group needs multiplicity two, and individual outcomes are still needed for a faithful distribution and worst-case analysis.
