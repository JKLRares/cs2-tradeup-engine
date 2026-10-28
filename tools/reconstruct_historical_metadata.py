"""Reconstruct the reviewed 2024 cell ledger from read-only OOXML cached data.

Normal builds do not need Python or the workbook. --check verifies the committed
ledger; --output writes only an explicitly selected nonhistorical source file.
No formulas are executed and no historical file is written.
"""
from __future__ import annotations

import argparse
from collections import Counter
from decimal import Decimal, ROUND_HALF_UP
from fractions import Fraction
import hashlib
import json
from pathlib import Path
import re
import xml.etree.ElementTree as ET
import zipfile

from historical_integrity import archive_form

ROOT = Path(__file__).resolve().parents[1]
NS = {'s': 'http://schemas.openxmlformats.org/spreadsheetml/2006/main'}
REF = re.compile(r'(\$?)([A-Z]{1,3})(\$?)([0-9]+)')


def column(n):
    text = ''
    while n:
        n, digit = divmod(n - 1, 26)
        text = chr(65 + digit) + text
    return text


def coordinates(address):
    m = REF.fullmatch(address)
    n = 0
    for c in m[2]:
        n = n * 26 + ord(c) - 64
    return n, int(m[4])


def reconstruct(historical):
    manifest = json.loads((ROOT / 'tests/fixtures/historical_manifest.json').read_text())['files']
    expected = {entry['path']: entry for entry in manifest}
    for relative in ('supporting/excel/CASES AUTO.xlsx', '2024/most recent version i think/input.txt',
                     '2024/most recent version i think/main.cpp'):
        archive_form((historical / relative).read_bytes(), expected['historical/' + relative])
    book = historical / 'supporting/excel/CASES AUTO.xlsx'
    with zipfile.ZipFile(book) as archive:
        tree = ET.fromstring(archive.read('xl/worksheets/sheet1.xml'))
    cells = {n.get('r'): n for n in tree.findall('s:sheetData/s:row/s:c', NS)}
    anchors = {}
    for address, node in cells.items():
        f = node.find('s:f', NS)
        if f is not None and f.get('t') == 'shared' and f.text:
            anchors[f.get('si')] = (address, f.text)

    def formula(address):
        f = cells[address].find('s:f', NS)
        if f is None:
            return ''
        if f.text:
            return f.text
        anchor, expression = anchors[f.get('si')]
        x, y = coordinates(address)
        ax, ay = coordinates(anchor)
        def shift(m):
            cx, cy = coordinates(m.group())
            return m[1] + column(cx if m[1] else cx + x - ax) + m[3] + str(cy if m[3] else cy + y - ay)
        return REF.sub(shift, expression)

    def classify(address, seen=()):
        if address in seen or address not in cells:
            return 'unknown', 'unresolved reference'
        expression = formula(address)
        if not expression:
            return 'recorded', 'cached numeric observation; acquisition and quote time unknown'
        if expression.startswith('MIN('):
            return 'minimum_derived', expression
        if expression.startswith('AVERAGE('):
            return 'composite_estimate', expression
        if '/' in expression:
            return 'ratio_estimate', expression
        if REF.fullmatch(expression):
            kind, lineage = classify(expression.replace('$', ''), seen + (address,))
            return kind, expression + ' -> ' + lineage
        return 'unknown', expression + '; unclassified formula'

    tokens = (historical / '2024/most recent version i think/input.txt').read_text().split()
    assert tokens[0] == '58' and len(tokens) == 3191
    rows = []
    kinds = Counter()
    for r in range(67, 122):
        offset = 6 if 87 <= r <= 108 or r >= 115 else 5
        for c in range(29):
            for wear in range(2):
                address = f'{column(7*c+offset+wear)}{r}'
                value = cells[address].findtext('s:v', namespaces=NS)
                if value is None:
                    raise ValueError(f'missing cached value at {address}')
                rounded = Decimal(value).quantize(Decimal('.01'), rounding=ROUND_HALF_UP)
                index = (r - 67) * 58 + 2*c + wear
                assert Fraction(rounded) == Fraction(tokens[index + 1]), address
                # Explicit role map from the reviewed basket, independent of numeric value.
                padding = r in (107, 108, 121) or (r in (78, 86, 94, 102) and c in (0, 2, 3, 22, 28))
                if padding and (Decimal(value) != 0 or formula(address)):
                    raise ValueError(f'declared padding does not match preserved evidence: {address}')
                kind, lineage = classify(address)
                kinds['padding' if padding else kind] += 1
                if padding:
                    kind, lineage = 'unknown', 'explicit formula-free zero padding in reviewed scenario projection'
                rows.append((address, str(Fraction(rounded)), kind, padding, lineage))
    lines = ['// Reviewed generated cell ledger. Regenerate/check with tools/reconstruct_historical_metadata.py.',
             '// Values are exported cent-rounded prices; formulas are provenance, not executed rules.',
             'constexpr std::array<HistoricalCell, 3190> historical_cells{{']
    for address, price, kind, padding, lineage in rows:
        lines.append('    {' + ', '.join((json.dumps(address), json.dumps(price), 'QuoteKind::'+kind,
                     str(padding).lower(), json.dumps(lineage))) + '},')
    lines.append('}};')
    return '\n'.join(lines) + '\n', dict(kinds)


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--historical-root', type=Path, default=ROOT / 'historical')
    p.add_argument('--output', type=Path, default=ROOT / 'src/historical_cells.inc')
    p.add_argument('--check', action='store_true')
    args = p.parse_args()
    result, counts = reconstruct(args.historical_root)
    output = args.output.resolve()
    if any(part.lower() in ('historical', '.git') for part in output.parts):
        p.error('output must be outside historical materials and .git')
    if args.check:
        if output.read_text(encoding='utf-8') != result:
            raise SystemExit('historical cell ledger differs from workbook evidence')
    else:
        output.write_text(result, encoding='utf-8', newline='\n')
    print('Historical cell ledger:', len(result.encode()), 'bytes;', json.dumps(counts, sort_keys=True))
    print('Ledger SHA-256:', hashlib.sha256(result.encode()).hexdigest())
