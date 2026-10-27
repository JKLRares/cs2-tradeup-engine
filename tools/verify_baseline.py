"""Bounded, read-only Milestone 1 checks; Python 3.10+, standard library only.

This checks fixed witnesses and preserved data. It is not the reference engine.
No historical code or binaries are executed. No files are written.
"""
from __future__ import annotations

import argparse
from decimal import Decimal, ROUND_HALF_UP
from fractions import Fraction as F
import hashlib
import json
from pathlib import Path
import re
import unittest
import xml.etree.ElementTree as ET
import zipfile

from historical_integrity import archive_form

ROOT = Path(__file__).resolve().parents[1]
FIXTURES = ROOT / "tests" / "fixtures"
NS = {"s": "http://schemas.openxmlformats.org/spreadsheetml/2006/main"}
HISTORICAL = ROOT / "historical"
REQUIRE_LOCAL = False
INDEPENDENT: Path | None = None


def fixture(name: str):
    return json.loads((FIXTURES / name).read_text(encoding="utf-8"))


def column(number: int) -> str:
    label = ""
    while number:
        number, digit = divmod(number - 1, 26)
        label = chr(65 + digit) + label
    return label


class Baseline(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest = fixture("historical_manifest.json")["files"]
        cls.scenarios = fixture("scenarios.json")["scenarios"]
        cls.numerical = fixture("numerical_cases.json")
        cls.current = HISTORICAL / "2024" / "most recent version i think"
        cls.input_lines = (cls.current / "input.txt").read_text().splitlines()
        cls.prices = [list(map(F, line.split())) for line in cls.input_lines[1:]]
        with zipfile.ZipFile(HISTORICAL / "supporting" / "excel" / "CASES AUTO.xlsx") as book:
            strings = ["".join(node.itertext()) for node in ET.fromstring(
                book.read("xl/sharedStrings.xml"))]
            tree = ET.fromstring(book.read("xl/worksheets/sheet1.xml"))
        cls.cells = {node.get("r"): node
                     for node in tree.findall("s:sheetData/s:row/s:c", NS)}
        cls.strings = strings

    def value(self, address: str) -> str:
        node = self.cells[address]
        if node.get("t") == "inlineStr":
            return "".join(node.find("s:is", NS).itertext())
        value = node.find("s:v", NS)
        if value is None:
            return ""
        return self.strings[int(value.text)] if node.get("t") == "s" else value.text

    def test_01_preservation(self):
        self.assertEqual(len(self.manifest), 33)
        self.assertEqual(sum(entry["tracked"] for entry in self.manifest), 11)
        present = 0
        absent_local = 0
        for entry in self.manifest:
            relative = Path(entry["path"]).relative_to("historical")
            path = HISTORICAL / relative
            if not path.is_file() and not entry["tracked"] and not REQUIRE_LOCAL:
                absent_local += 1
                continue
            with self.subTest(path=entry["path"]):
                data = path.read_bytes()
                authenticated = archive_form(data, entry, allow_git_lf=not REQUIRE_LOCAL)
                self.assertEqual(len(authenticated), entry["bytes"])
                self.assertEqual(hashlib.sha256(authenticated).hexdigest(), entry["sha256"])
                if INDEPENDENT is not None:
                    self.assertEqual(authenticated, (INDEPENDENT / Path(*relative.parts[1:])).read_bytes())
                present += 1
        print(f"Integrity: {present} matched, {absent_local} optional local files absent")

    def test_02_matrix_shape(self):
        self.assertEqual(self.input_lines[0], "58")
        self.assertEqual(len(self.prices), 55)
        self.assertTrue(all(len(row) == 58 for row in self.prices))
        self.assertTrue(all(value >= 0 for row in self.prices for value in row))
        self.assertTrue(all(value == 0 for row in (107, 108, 121)
                            for value in self.prices[row - 67]))

    def test_03_scenario_calls_match_source(self):
        source = (self.current / "main.cpp").read_text()
        calls = re.findall(r'strcpy\(raritate,"([^"]+)"\);\s*'
                           r'programel\(n,(.*?),raritate\);', source)
        self.assertEqual(len(calls), 12)
        for (name, arguments), scenario in zip(calls, self.scenarios):
            rows = [int(row) for row in re.findall(r'sefu\[(\d+)-67\+1\]', arguments)]
            self.assertEqual(name, scenario["name"])
            self.assertEqual(rows, scenario["workbook_output_rows"]
                             + [scenario["workbook_cost_row"]])

    def test_04_workbook_export_projection(self):
        differences = 0
        for row in range(67, 122):
            offset = 6 if 87 <= row <= 108 or row >= 115 else 5
            for collection in range(29):
                for wear in range(2):
                    address = f"{column(7 * collection + offset + wear)}{row}"
                    raw = Decimal(self.value(address))
                    exported = self.prices[row - 67][2 * collection + wear]
                    rounded = raw.quantize(Decimal(".01"), rounding=ROUND_HALF_UP)
                    self.assertEqual(F(rounded), exported, address)
                    differences += F(raw) != exported
        self.assertEqual(differences, 13)
        print("Workbook: 3190/3190 rounded export values match; 13 differ before rounding")

    def test_05_costs_and_paired_outcome_slots(self):
        for scenario in self.scenarios:
            output = [self.prices[row - 67] for row in scenario["workbook_output_rows"]]
            self.assertTrue(all(price > 0 for price in self.prices[
                scenario["workbook_cost_row"] - 67]))
            counts = [sum(row[bucket] != 0 for row in output) for bucket in range(58)]
            for bucket in range(0, 58, 2):
                self.assertGreater(counts[bucket], 0)
                self.assertEqual(counts[bucket], counts[bucket + 1])

    def test_06_estimates_are_in_the_workbook(self):
        expected = {"EV78": "AVERAGE(EV38,EV39)", "EV86": "AVERAGE(EV58,EV59)",
                    "S109": "S29/4", "T109": "S29/4", "S112": "S49/3.5",
                    "GS109": "GS29/1.65", "GS112": "GS49/1.65"}
        for address, formula in expected.items():
            self.assertEqual(self.cells[address].find("s:f", NS).text, formula)
        self.assertEqual(F(self.value("EV78")),
                         (F(self.value("EV38")) + F(self.value("EV39"))) / 2)
        self.assertEqual(self.value("GV25"), "KILOWATT")

    def test_07_fixed_rational_examples(self):
        retained = 1 - F(self.numerical["fee"])
        for case in self.numerical["cases"]:
            with self.subTest(case=case["id"]):
                quantities = case["quantities"]
                outcomes = [list(map(F, group)) for group in case["outcome_prices"]]
                self.assertEqual(sum(quantities), 10)
                self.assertTrue(all(quantity > 0 for quantity in quantities))
                denominator = sum(q * len(group) for q, group in zip(quantities, outcomes))
                self.assertEqual(sum(F(q, denominator) * len(group)
                                     for q, group in zip(quantities, outcomes)), 1)
                cost = sum(q * F(price) for q, price in zip(quantities, case["unit_costs"]))
                gross = sum(F(q, denominator) * sum(group)
                            for q, group in zip(quantities, outcomes))
                worst = retained * min(price for group in outcomes for price in group)
                self.assertEqual(cost, F(case["cost"]))
                self.assertEqual(gross, F(case["gross_ev"]))
                self.assertEqual(retained * gross, F(case["net_ev"]))
                self.assertEqual(100 * retained * gross / cost, F(case["return_percent"]))
                self.assertEqual(worst, F(case["worst_net"]))

    def test_08_saved_historical_witness(self):
        case = next(case for case in self.numerical["cases"]
                    if case["id"] == "saved_2024_first_line")
        rows = [82, 83, 84, 85, 86]
        self.assertEqual([self.prices[1][bucket - 1] for bucket in (2, 29)],
                         list(map(F, case["unit_costs"])))
        for bucket, expected in zip((2, 29), case["outcome_prices"]):
            quote_column = ((bucket - 1) // 2) * 2 + 1
            actual = [self.prices[row - 67][quote_column] for row in rows
                      if self.prices[row - 67][quote_column] != 0]
            self.assertEqual(actual, list(map(F, expected)))
        saved = self.current / "www.txt"
        if saved.is_file():
            first = saved.read_text().splitlines()[0]
            self.assertEqual(F(first.split()[0]), F("128.098"))
            self.assertIn("StatTrak FT MW mov", first)
            self.assertLess(abs(F(case["return_percent"]) - F("128.098")), F(".0005"))
        self.assertLess(F(case["worst_net"]), F(case["cost"]))

    def test_09_integer_normalization_witness(self):
        quantities = (5, 2, 3)
        divisor = min(quantities)
        weights = [quantity // divisor for quantity in quantities]
        self.assertEqual(weights, [2, 1, 1])
        self.assertEqual(F(2 * 40 + 100 + 160, 8), F("42.5"))
        self.assertEqual(F(5 * 40 + 2 * 100 + 3 * 160, 20), 44)
        self.assertNotEqual(F(6 // 4), F(6, 4))

    def test_10_quantity_loop_omission_witness(self):
        # Source lines 44-48, isolated from all memory-unsafe legacy behavior.
        intended = set()
        observed = set()
        for a in range(6):
            for b in range(6):
                for c in range(6):
                    intended.update((a, b, c, d) for d in range(6) if a + b + c + d == 5)
                    d = 0
                    while d <= 5:
                        if a + b + c + d != 5:
                            d += 1
                        else:
                            observed.add((a, b, c, d))
                        d += 1
        self.assertEqual((len(intended), len(observed)), (56, 34))
        self.assertIn((0, 0, 0, 5), intended - observed)
        self.assertTrue(observed < intended)

    def test_11_equal_returns_do_not_define_identity(self):
        # Literal specification witnesses; no production identifier implementation.
        key_a = ("snapshot", "scenario", "historical-quote-tickets-v1", (("A", 5), ("B", 5)))
        key_b = ("snapshot", "scenario", "historical-quote-tickets-v1", (("C", 5), ("D", 5)))
        self.assertNotEqual(key_a, key_b)
        self.assertEqual(F(25) * F(".87") / 15, F(50) * F(".87") / 30)
        padded = (("A", 5), ("B", 5), ("unused", 0))
        self.assertEqual(tuple(pair for pair in padded if pair[1]), key_a[-1])

    def test_12_alternative_probability_policy_is_different(self):
        ticket_ev = F(6 * 40 + 4 * 120, 24)
        collection_first_ev = F(6, 10) * F(40, 2) + F(4, 10) * F(120, 3)
        self.assertEqual((ticket_ev, collection_first_ev), (30, 28))

    def test_13_merging_without_multiplicity_changes_mean(self):
        six_quotes = [F(self.value(f"EV{row}")) for row in range(34, 40)]
        five_slots = six_quotes[:4] + [(six_quotes[4] + six_quotes[5]) / 2]
        self.assertNotEqual(sum(six_quotes) / 6, sum(five_slots) / 5)
        self.assertEqual(sum(six_quotes) / 6,
                         (sum(five_slots[:4]) + 2 * five_slots[4]) / 6)

    def test_14_threshold_is_strict(self):
        case = next(case for case in self.numerical["cases"]
                    if case["id"] == "strict_threshold_boundary")
        self.assertEqual(F(case["return_percent"]), 115)
        self.assertFalse(F(case["return_percent"]) > F(self.numerical["qualification_return_percent"]))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--historical-root", type=Path, default=HISTORICAL)
    parser.add_argument("--require-local", action="store_true",
                        help="Require all 33 local files, including untracked/ignored materials")
    parser.add_argument("--independent-archive", type=Path,
                        help="Also compare preserved files to their original archive paths")
    arguments = parser.parse_args()
    HISTORICAL = arguments.historical_root.resolve()
    REQUIRE_LOCAL = arguments.require_local
    INDEPENDENT = arguments.independent_archive
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(Baseline)
    outcome = unittest.TextTestRunner(verbosity=2).run(suite)
    raise SystemExit(0 if outcome.wasSuccessful() else 1)
