# Historical baseline and evidence

This baseline reconstructs local files without executing historical programs. Paths below are relative to the repository. Source line numbers refer to preserved bytes; the SHA-256 manifest is in `tests/fixtures/historical_manifest.json`.

## Provenance and preservation

The three archival commits dated 2026-10-22 document preservation, not original development. Their metadata has not been rewritten.

There are 33 preserved files: 11 tracked and 22 untracked or ignored. Each repository copy matched its corresponding file in the independent archive during baseline capture. The manifest records all 33, including binary and damaged files. Optional local files are not required in a clean public checkout, but are verified whenever present.

Four locally preserved screenshots show Sheets version-history entries from August 2023, later 2023, July/August 2024, and September 2024. `Screenshot 2026-10-20 184349.jpg` visibly includes August 14–17, 2023 entries; `184502.jpg` shows a September 5, 2024 current version. These support development during those periods. Screenshot capture dates and version-history dates are not pricing timestamps. No remote document was accessed.

## Inventory by actual content

The exact names, sizes, and hashes of every file are in the manifest. This table covers every file, grouping only generated build artifacts and screenshots.

| Location | Actual content and use |
| --- | --- |
| `historical/2023/history/c++ 5 higher quality numbers.txt` | C++ text with a complete-looking translation unit; six-vector parser and 5/5 strategy. Evidence source, not a build target. |
| `historical/2023/history/final version.txt` | C++ text, 6/4 experiments and later commented material. Its name does not establish chronological finality. |
| `historical/2023/history/principiu.txt` | Six-line probability/value note using `6/4` and a 13% deduction. |
| `historical/2023/history/CASES AUTO og.xlsx` | Valid ZIP/OOXML spreadsheet; older formulas and cached values. |
| `historical/2023/history/google sheets final code.txt` | Readable Apps Script with normal/StatTrak item mappings and seven-cell transfers. |
| `historical/2023/history/c++ 6 higher quality numbers.txt` | Starts mid-source and ends in partial numerical results; mixed/truncated evidence. |
| `historical/2023/history/c++ 5 higher quality letters.txt` | Numerical result text, with truncated boundaries; filename is misleading. |
| `historical/2023/history/c++ main numbers.txt` | Numerical result text, with truncated boundaries. |
| `historical/2023/history/CASES AUTO latest update.xlsx` | Numerical result text, not an XLSX workbook. |
| `historical/2023/history/c++ 5 higher quality letters descrescator.txt` | Non-text/damaged-looking bytes; no trustworthy source reconstruction established. |
| `historical/2023/history/c++ final version full auto.txt` | Non-text/damaged-looking bytes; no trustworthy source reconstruction established. |
| `historical/2023/history/google sheets normal.txt` | Begins with an `MZ` executable signature; not trusted script text. Not executed. |
| `historical/2023/history/google sheets st.txt` | Starts with a ZIP local-file signature, but Python rejects it as a valid ZIP archive. Not assumed to be recoverable script. |
| `historical/2024/most recent version i think/main.cpp` | Most complete multi-scenario source; operational design reference, containing unsafe code. |
| `historical/2024/most recent version i think/Copy.cpp` | Older single-scenario C++ source, expecting six vectors after `n`. |
| `historical/2024/most recent version i think/input.txt` | Valid numeric matrix: header 58, then 55 rows × 58 values. Primary preserved price fixture. |
| `historical/2024/most recent version i think/www.txt` | 671 saved output lines; first line has an independently reconstructable 128.098 return. No assumption that all lines came from the exact surviving source build. |
| `historical/2024/most recent version i think/trade up.cbp` | Code::Blocks project XML; historical build context. |
| `historical/2024/most recent version i think/trade up.depend` | Dependency metadata; no mathematical authority. |
| `historical/2024/most recent version i think/trade up.layout` | IDE layout XML. |
| `Copy.exe`, `main.exe`, `bin/Debug/trade up.exe` in the 2024 directory | PE executable signatures; preserved and never executed. |
| `Copy.o`, `main.o`, `obj/Debug/main.o` in the 2024 directory | COFF-looking compiled object files; preserved, not linked. |
| `historical/supporting/excel/CASES AUTO.xlsx` | Valid workbook with two sheets; Sheet1 contains item data, 30 collection blocks, and derived scenario rows. C++ uses only 29 blocks. |
| `historical/supporting/excel/script excel 2023.txt` | Apps Script mapping pasted names to collection cells; source search begins at GY. |
| `historical/supporting/excel/script excel 2024.txt` | Extended Apps Script; source HC:HI and 30 blocks, including Kilowatt. |
| Four `historical/supporting/excel/Screenshot 2026-10-20 *.jpg` files | Visually inspected Sheets history screenshots; provenance evidence, not market snapshots with verified quote times. |

Classification describes observed content, not the cause of corruption or original author intent. No attempted recovery overwrote any material.

## Reconstructed workflow

1. Item/price rows were pasted into a Sheet1 staging area. The local evidence does not establish how quotes were acquired, their currency, or their capture times.
2. Apps Script matched an exact item-name string, looked up its destination, copied that cell and six cells to its right, and cleared the source name cell. The 2024 script implements this at lines 1139–1168. It is a sheet rearrangement script; it is not itself evidence of automatic price acquisition.
3. Collection blocks stored normal and StatTrak items and wear prices. `MIN` formulas selected a representative input price for each collection/rarity/wear. Scenario-specific rows copied, averaged, or estimated output values.
4. Scenario prices were exported as numeric text. All 3,190 `input.txt` values equal the corresponding workbook cached values rounded to two decimal places using decimal half-up rounding. Thirteen values differ before rounding. This demonstrates data consistency, not the exact historical export procedure.
5. The 2024 C++ source reads the entire matrix, invokes `programel` twelve times, searches selected five-index/quantity patterns, computes an estimated fee-adjusted return percentage, and writes qualifying results to `www.txt`.

The workflow is partly automated: sheet organization and calculations are automated; acquisition/export steps are not fully specified by the surviving files.

## Data formats and compatibility

### 2024 matrix

`main.cpp` lines 2995–3002 read `n` followed by `55*n` whitespace-separated numbers, row-major. In the surviving file `n=58`, there are 56 physical lines and 3,191 total tokens. Tabs and repeated whitespace have no semantic role. The original parser does not enforce physical row widths or successful reads.

Matrix data row `r` corresponds to workbook row `r+66`. All prices are finite, nonnegative numeric values; every scenario cost row is positive. Rows corresponding to workbook 107, 108, and 121 are all-zero output padding.

Each collection has adjacent lower/higher wear options. One-based odd bucket `2c-1` is the lower wear-price category and even bucket `2c` its selected higher category. The labels say FT/MW or MW/FN; they describe input wear categories, not mixed rarity inputs. The source's colloquial comments about quality/rarity should not override the same-rarity price rows supplied to each scenario.

The hard-coded order in `main.cpp:11` is:

```text
BREAKOUT, CS20, CHROMA, CHROMA 2, CHROMA 3, CLUTCH,
DANGER ZONE, DREAMS&NIGHTMARES, FALCHION, FRACTURE,
GAMMA, GAMMA 2, GLOVE, HORIZON, BROKEN FANG, HYDRA,
RIPTIDE, PRISMA, PRISMA 2, RECOIL, REVOLUTION, REVOLVER,
SHADOW, SHATTERED WEB, SNAKEBITE, SPECTRUM, SPECTRUM 2,
WILDFIRE, WINTER OFFENSIVE
```

Kilowatt exists in the later workbook and script, but not in the C++ label table or 58-column input. Adding it would expand coverage and requires an explicit converter/catalog change.

For workbook collection block `c=0..28`, columns are selected by one-based column number `7c+5,7c+6` for rows 67–86 and 109–114; columns `7c+6,7c+7` for rows 87–108 and 115–121. Sheet labels and other cells are excluded. The verifier checks all resulting values.

### Earlier six-vector files

`c++ 5 higher quality numbers.txt`, `final version.txt`, and `Copy.cpp` expect `n`, then six vectors of length `n`: output slots `a,b,c,d,e`, followed by input unit prices `f`. Files were named `date.in/date.out` in 2023 and `input.txt/www.txt` in `Copy.cpp`.

The surviving 2024 matrix cannot be fed to these parsers as if the formats were interchangeable. A six-vector converter would need to select the five output rows and cost row from a declared scenario. No such converter or historical executable is shipped; the [modern CLI](offline-cli.md) imports the fixed 2024 matrix directly.

### Compatibility decisions

| Material | Modern use |
| --- | --- |
| 2024 `input.txt` | Read-only application input using the fixed [29-collection mapping and cell ledger](offline-cli.md#historical-matrix). The matrix is not self-describing. |
| Twelve source calls | Direct scenario definitions; machine-readable transcription in `tests/fixtures/scenarios.json`. |
| Valid XLSX workbooks | Read-only XML/cached-value evidence for metadata reconstruction. General runtime XLSX import and formula recalculation are not provided. |
| Readable Apps Script | Metadata/workflow reference; no Google account access or execution needed. |
| Numerical output fragments | Evidence only; partial rows and ambiguous identifiers prevent reliable general import. |
| `www.txt` | Optional regression evidence; human labels and zero padding make it unsuitable as a canonical dataset. |
| Binary/damaged/misnamed files | Preserve; do not infer content from extension or execute them. |

## Twelve 2024 scenarios

Rows in this table are original workbook row numbers, not matrix data-row numbers. `mov`, `roz`, and `rosu` are the source's purple, pink, and red output labels, interpreted as the respective Mil-Spec→Restricted, Restricted→Classified, and Classified→Covert transitions from the sheet item grouping. A scenario never mixes normal and StatTrak or input rarity tiers.

| Source label | Output rows | Cost row | `main.cpp` call line |
| --- | --- | --- | --- |
| NORMAL FT MW mov | 74–78 | 67 | 3005 |
| NORMAL FT MW roz | 71–73,107,108 | 69 | 3008 |
| StatTrak FT MW mov | 82–86 | 68 | 3011 |
| StatTrak FT MW roz | 79–81,107,108 | 70 | 3014 |
| NORMAL MW FN mov | 90–94 | 103 | 3017 |
| NORMAL MW FN roz | 87–89,107,108 | 105 | 3020 |
| StatTrak MW FN mov | 98–102 | 104 | 3023 |
| StatTrak MW FN roz | 95–97,107,108 | 106 | 3026 |
| NORMAL FT MW rosu | 109,110,121,107,108 | 111 | 3029 |
| StatTrak FT MW rosu | 112,113,121,107,108 | 114 | 3032 |
| NORMAL MW FN rosu | 115,116,121,107,108 | 117 | 3035 |
| StatTrak MW FN rosu | 118,119,121,107,108 | 120 | 3038 |

The five-slot outcome counts are 4 or 5 for `mov`, 3 for `roz`, and 2 for `rosu`, equal across each adjacent wear pair in all twelve scenarios. They are modeled slot counts, not proof of the full real-game outcome catalog.

## Original intended search restrictions

The 2024 source selects five strictly increasing option indices `i<j<k<v<w` (`main.cpp:32–36`). A parity branch groups even and odd options, allocating five inputs to each wear side. There are 30 mixed-parity branch patterns. Zero quantities let fewer than five selected indices contribute to a contract. Both wear sides must contribute, so the intended active support is 2–5 buckets, potentially from just one collection if both wear buckets are used. The same bucket may supply multiple inputs.

Odd input buckets use the following even bucket's output prices (`main.cpp:999–1008` and analogous branches). Thus both wear sides of a collection are valued using the selected higher output wear category. No float calculation checks that assumption.

The 2023 sources contain both 5/5 and 6/4 experiments. `final version.txt:36–44` explicitly costs six lower-category and four higher-category inputs in a two-bucket experiment; later branches distribute quantities more broadly. These are useful, deliberately bounded strategies. Neither strategy is a search over all possible ten-input wear allocations. Actual legacy enumeration is additionally affected by the defects below; full intended coverage is not established.

At 58 options, just the five-index selection count is `C(58,5)=4,582,116`. A deliberately loose `6^5` quantity-loop bound gives over 35 billion loop positions per scenario. This is not a measured candidate count or complexity proof for every branch. No full historical search was run.

## Behavior classification

“Confirmed defect” below means a violation of the stated memory, arithmetic, or enumeration contract. It does not imply knowledge of the author's intent. Restrictions, approximations, and presentation choices are classified separately.

| ID / behavior | Classification | Concrete evidence and implication |
| --- | --- | --- |
| D1: negative label index | Confirmed software defect | `main.cpp:12` writes `caseuri[-1]`; zero-quantity labels also access `caseuri[i2-1]` at e.g. line 105. Both violate array bounds. No execution is needed to establish this. |
| D2: uninitialized sentinel input price | Confirmed software defect | Lines 27–31 initialize output index zero; `f[0]` is never initialized. E.g. lines 64–71 map zero quantities to index zero and line 93 evaluates `0*f[0]`. Multiplying by zero does not remove the uninitialized read. |
| D3: unbounded qualifying-result array | Confirmed capacity defect | `profit[100]` at line 9 and `++i4; profit[i4]=...` at 98–99 permit only indices 1–99, with no cap. Every qualifying evaluation increments the index, including results later suppressed as duplicates. The 100th qualifying evaluation writes outside the array. No runtime hit count is claimed. |
| D4: integer-normalized quantity weights | Confirmed mathematical defect relative to proportional-ticket weights | `Min` and quantities are integers; lines 81–90 truncate divisions before assigning to float. For 5/2/3 quantities, `Min=2` produces weights 2/1/1 instead of 2.5/1/1.5. The bounded witness changes EV 44 to 42.5. It is harmless when the chosen divisor divides all quantities. |
| D5: skipped quantity allocations | Confirmed enumeration defect relative to each branch's own sum constraint | Lines 44–48 increment `v1` on failure, then the `for` loop increments again. Exhaustive isolation over only `6^4=1296` tuples accepts 34 of 56 solutions to a+b+c+d=5 and omits (0,0,0,5). No global coverage count is inferred. |
| D6: modifying an outer selection variable | Confirmed traversal defect; intent of the attempted shortcut is visible | Lines 37–38 mean to avoid an all-even selection, but increment `k` inside the `w` loop. After (2,4,6,8,10), subsequent `w` values use k=7, losing later selections with the original k=6. The parity restriction is deliberate; this implementation does not implement a simple skip. |
| D7: unchecked parser and array bounds | Confirmed validation defect; supplied matrix is within nominal bounds | Lines 2998–3002 do not check file open, extraction success, or n against 100-element arrays and the fixed 58-label table. Missing/truncated input leaves values unusable. Validating this input does not validate the general parser. |
| H1: repeated canonical contracts | Intentional behavior per project owner; redundant representations are evident | Zero-padded five-index selections can represent the same active quantities using different unused indices. Retain broad coverage, replace repeated evaluation later with proven canonical generation. Repetition alone does not establish completeness. |
| H2: identical-return suppression | Presentation-only historical choice; incompatible with future complete-results semantics | Lines 100–102 compare exact float `profit2` values, even across distinct contracts. It can reduce repetitive output, but equal return does not imply identical inputs or outcome distributions. Retain only as explicit grouping after results are preserved. D3 is a separate defect. |
| H3: 5/5 and 6/4 wear splits | Intentional restriction | Explicit quantities and the owner's account establish a chosen opportunity-search scope. Document and name them; do not claim unrestricted completeness. |
| H4: minimum-price representatives | Intentional preprocessing, conditionally justified | Workbook `E67=MIN(E39:E45)` and corresponding cost rows. Dominance is valid when replaced inputs share all relevant eligibility/outcome properties and quantities are available; exact float eligibility and supply are not established. |
| H5: higher-category output valuation | Intentional approximation/restriction | Odd indices map to adjacent even output columns. Wear groups alone cannot verify every resulting output wear. Preserve as a named historical valuation policy. |
| H6: five slots and averaged Revolver outputs | Intentional approximation in the worksheet; origin of the limit remains ambiguous | `EV78=AVERAGE(EV38,EV39)`, with analogous StatTrak and MW/FN rows. Six named raw Revolver rows 34–39 become five priced slots. Unweighted averaging into one ticket does not generally preserve a six-outcome distribution or EV. This is not merely a display limit. |
| H7: divided Covert price estimates | Intentional derived estimates; rationale unresolved | `S109=S29/4`, `S112=S49/3.5`, `GS109=GS29/1.65`, and related cells. Formula evidence establishes derivation, not economic validity or timestamp. Preserve the estimates and mark their provenance. |
| H8: zero output slots | Intentional padding where independently identified; ambiguous for arbitrary data | Rows 107/108/121 are explicit zero padding. Other zeros align with empty outcome slots in this snapshot. A future importer must not treat an unknown/missing quote as an impossible outcome without metadata. |
| H9: 13% deduction and >115 threshold | Intentional calculation/filter policy | Lines 94–96: gross return ×0.87, then strict >115. The name `profit2` includes returned capital; it is not net ROI. No claim about actual platform fee schedules or rounding is established. |
| U1: 2023 two-bucket weighting | Confirmed integer division; ambiguous intended coefficient placement | `final version.txt:39–41` and `principiu.txt` use integer 6/4, while the numerator applies the stated larger/smaller coefficients to opposite output sums. Merely changing 6/4 to floating arithmetic would not establish normalized intended probabilities when counts differ. Re-derive from the model. |
| U2: saved results versus surviving source | Ambiguous provenance | The first line is independently consistent with the model and input, but binaries/build dates and unsafe source do not establish which exact executable produced all 671 lines. No byte-for-byte legacy output oracle is asserted. |

The most valuable original ideas are the abstraction of interchangeable inputs, representative preprocessing, quantity allocations, ordered selection, and reuse of a single price matrix across scenarios. Modernization should make their equivalence conditions and coverage precise, rather than discard them.
