"""Run the independent Fraction/Cartesian oracle against the optimized test driver.

Reuses only the existing independent oracle, never optimized enumeration logic.
"""
from __future__ import annotations

import argparse
from fractions import Fraction as F
import json
from pathlib import Path
import random
import unittest

import verify_reference as oracle

EXTRA = {"extra_cartesian_cases": 0, "extra_cartesian_contracts": 0,
         "large_catalog_evaluations": 0}


class OptimizedVerification(oracle.ReferenceVerification):
    def test_09_more_cartesian_thresholds_and_equal_returns(self):
        for seed in range(12):
            lower, higher = 1 + seed % 3, 1 + (seed // 3) % 3
            data = oracle.dataset(lower, higher, 700 + seed)
            threshold = [F(0), F(115), F(115000000000001, 10**12)][seed % 3]
            if seed % 4 == 0:
                for c in data["collections"]:
                    c["prices"] = [F(100)] * len(c["prices"])
                for b in data["buckets"]:
                    b["cost"] = F(1)
            for l, h in [(5, 5), (6, 4)]:
                expected = oracle.cartesian_oracle(data, l, h)
                command = "enum55" if l == 5 else "enum64"
                output = oracle.invoke(f"{command} {oracle.encoded(data)} {threshold} 10000 10000").stdout.splitlines()
                contracts = [line.split("\t")[1] for line in output if line.startswith("C\t")]
                found = [line.split("\t")[1:] for line in output if line.startswith("Q\t")]
                wanted = {key: values for key, q in expected.items()
                          if (values := oracle.arithmetic_oracle(data, q)[0])[3] > threshold}
                self.assertEqual(len(contracts), len(set(contracts)))
                self.assertEqual(set(contracts), set(expected))
                self.assertEqual(len(found), len(wanted))
                self.assertEqual({line[0]: list(map(F, line[1:])) for line in found}, wanted)
                EXTRA["extra_cartesian_cases"] += 1
                EXTRA["extra_cartesian_contracts"] += len(contracts)

    def test_10_large_catalog_independent_evaluations(self):
        data = oracle.dataset(29, 29, 58)
        rng = random.Random(20261026)
        for _ in range(24):
            q = [0] * 58
            a = rng.randrange(29)
            b = rng.randrange(29, 58)
            q[a], q[b] = (5, 5) if _ % 2 else (6, 4)
            expected, probabilities = oracle.arithmetic_oracle(data, q)
            actual, distribution, _, key = oracle.evaluate(data, q)
            self.assertEqual(actual, expected)
            self.assertEqual(distribution, probabilities)
            self.assertEqual(key, oracle.identity(data, q))
            EXTRA["large_catalog_evaluations"] += 1

    def test_11_threshold_equality_all_zero_and_neighbors(self):
        for price in [F(0), F(115, 100), F(115, 100) + F(1, 10**12), F(115, 100) - F(1, 10**12)]:
            data = oracle.dataset(2, 2, 91)
            for c in data["collections"]:
                c["prices"] = [price] * len(c["prices"])
            for b in data["buckets"]:
                b["cost"] = F(87, 1000)
            output = oracle.invoke(f"enum55 {oracle.encoded(data)} 115 10000 10000").stdout.splitlines()
            qualified = [line.split("\t")[1:] for line in output if line.startswith("Q\t")]
            expected = oracle.cartesian_oracle(data, 5, 5)
            wanted = {key: values for key, q in expected.items()
                      if (values := oracle.arithmetic_oracle(data, q)[0])[3] > 115}
            self.assertEqual({line[0]: list(map(F, line[1:])) for line in qualified}, wanted)
            EXTRA["extra_cartesian_cases"] += 1
            EXTRA["extra_cartesian_contracts"] += len(expected)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--driver", type=Path, required=True)
    args = parser.parse_args()
    oracle.DRIVER = args.driver.resolve()
    if not oracle.DRIVER.is_file():
        parser.error("compiled optimized driver not found")
    outcome = unittest.TextTestRunner(verbosity=2).run(
        unittest.defaultTestLoader.loadTestsFromTestCase(OptimizedVerification))
    print("Independent optimized checks:", json.dumps(oracle.STATISTICS | EXTRA, sort_keys=True))
    raise SystemExit(0 if outcome.wasSuccessful() else 1)
