# Distribution and provenance decisions

No software license is currently granted or selected for the original modern
application code or development tools. A license may be chosen later, after the
public-release decision.

Redistribution rights for `historical/`, externally sourced prices, historical
workbooks, third-party materials and derived datasets remain unestablished.
The absence of a software license does not establish permission to redistribute
those materials. A separate owner decision on historical-data provenance and
public release is still required.

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

The provenance review also covers historical material outside the archive:

- [`src/historical_cells.inc`](../src/historical_cells.inc) is the workbook-derived
  price/formula ledger, despite its source-file extension.
- Historical metadata in [`src/historical.cpp`](../src/historical.cpp) and
  [`scenarios.json`](../tests/fixtures/scenarios.json), and the `saved_2024_first_line`
  case in [`numerical_cases.json`](../tests/fixtures/numerical_cases.json), remain
  subject to the provenance decision.
- Historical prices/results reproduced in test/verification code, README/docs or
  generated exports also require the provenance review.

A tracked-file and notice review found no separate third-party license notices
in the modern code/tools; this does not establish rights to the historical
materials or third-party item/collection names and marks. Confirm authorship of
archived code/scripts and retain any required third-party notices before public
distribution. Keep the independent archive unchanged; do not silently remove or
rewrite historical files or history to produce a cleaner narrative.

The 22 additional local files include saved results, screenshots, damaged or
misnamed files, IDE metadata and six historical executable/object artifacts.
They are not needed in the published checkout. Do not broadly stage them, and do
not run archived programs. Personal archive timestamps are evidence of archiving,
not fabricated development dates. The separate original archive remains read-only.

## Release artifacts

Source review is the present deliverable. Binary packaging was deliberately
deferred: no tested standalone bundle or installer exists. Windows UCRT64 builds
need GCC runtime DLLs (see [building](building.md)); redistribution would also
require checking their applicable notices and transitive dependencies in a clean
machine/VM. No binaries or runtime DLLs belong in this source staging set.

A later owner-approved binary bundle could contain `bin/tradeup.exe`, verified
runtime DLLs, `examples/`, a concise usage document and the applicable notices.
Only include historical fixtures after the data decision. That layout is a
proposal, not an implemented/verified package or public release.

Before making the repository public, review the full existing Git history for
private material, resolve the historical materials' redistribution rights and
make a separate public-release decision, then review the
[successful hosted Linux/Windows and sanitizer results](verification.md#automation-and-actual-coverage).
Keep the repository private while resolving any such question; visibility and
release publication remain owner actions.
