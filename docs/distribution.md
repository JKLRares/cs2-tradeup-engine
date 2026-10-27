# Distribution and provenance decisions

No repository license has been selected or added, and no repository/release was
published as part of this work.
The application is ready for source review; permission to redistribute every
preserved material still needs the owner's decision.

For original source, an **MIT license** is a reasonable portfolio recommendation
if the author wants permissive reuse with preservation of the copyright/license
notice. Review the [official MIT text](https://opensource.org/license/mit) before
choosing. This recommendation does not grant rights to third-party content.
Confirm ownership and retain any required third-party notices before distribution.

Review these **already tracked** historical materials before public distribution:

| Tracked group | Files | Decision to review |
| --- | ---: | --- |
| 2023 C++/notes/Apps Script text | 4 | Original authorship, copied snippets and required notices |
| 2023 workbook | 1 | Raw/cached data, formulas, embedded metadata and source terms |
| 2024 C++ and price matrix | 3 | Original code, item/collection naming and price-data acquisition/redistribution |
| Supporting workbook and Apps Scripts | 3 | Workbook data/formulas, script provenance and external material |

The exact eleven paths are marked `tracked: true` in the
[manifest](../tests/fixtures/historical_manifest.json). Workbooks are complete
OOXML archives, not only the projected cells used by tests. A read-only package
inspection found no external-link, comment or custom-property parts in the two
tracked workbooks; that narrow check is not a certification of redistribution
rights, privacy, or the contents of every cell. Acquisition source, currency,
quote times and market-data permissions remain unestablished. Item names and
third-party marks do not establish affiliation or permission.

The reconstructed cell ledger and required numeric fixtures are derived from
this evidence. Include them in the scope of the provenance decision. An original
source-code license cannot automatically resolve workbook/data rights. If notices
or exclusions are needed, decide them explicitly before publication while
retaining the independent archive unchanged. Do not silently remove or rewrite
historical files or history to produce a cleaner narrative.

The 22 additional local files include saved results, screenshots, damaged or
misnamed files, IDE metadata and six historical executable/object artifacts.
They are not needed in the published checkout. Do not broadly stage them, and do
not run archived programs. Personal archive timestamps are evidence of archiving,
not fabricated development dates. The separate original archive remains read-only.

## Release artifacts

Source distribution is the present deliverable. Binary packaging was deliberately
deferred: no tested standalone bundle or installer exists. Windows UCRT64 builds
need GCC runtime DLLs (see [building](building.md)); redistribution would also
require checking their applicable notices and transitive dependencies in a clean
machine/VM. No binaries or runtime DLLs belong in this source staging set.

A later owner-approved binary bundle could contain `bin/tradeup.exe`, verified
runtime DLLs, `examples/`, a concise usage document and the applicable notices.
Only include historical fixtures after the data decision. That layout is a
proposal, not an implemented/verified package or public release.

Before making the repository public, review the full existing Git history for
private material, decide the license/data scope, and obtain actual passing hosted
Linux/sanitizer/Windows results. Keep the repository private while resolving any
such question; visibility and release publication remain owner actions.
