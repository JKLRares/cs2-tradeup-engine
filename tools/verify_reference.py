"""Independent exact arithmetic and Cartesian-product oracle for the C++ test driver.

No historical imports or search implementation are supplied by this test harness.
Requires Python 3.10+ and a compiled reference_fixture_driver executable.
"""
from __future__ import annotations

import argparse
from fractions import Fraction as F
import hashlib
import itertools
import json
from pathlib import Path
import random
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]
DRIVER: Path
STATISTICS = {"rational_operations": 0, "fixture_evaluations": 0,
              "generated_evaluations": 0, "enumeration_comparisons": 0,
              "enumerated_vectors": 0}


def dataset(lower: int, higher: int, seed: int = 0):
    rng = random.Random(seed)
    collections = [
        {"id": "c0", "prices": [F(7 + seed % 3, 3), F(13, 2)]},
        {"id": "c1", "prices": [F(rng.randrange(0, 30), 4) for _ in range(3)]},
    ]
    buckets = [{"id": f"b{index:02}", "collection": f"c{index % 2}",
                "wear": "L" if index < lower else "H", "cost": F(index + 2, 10)}
               for index in range(lower + higher)]
    return {"snapshot": f"synthetic-{seed}-{lower}-{higher}", "scenario": "fixture",
            "collections": collections, "buckets": buckets}


def encoded(data):
    words = [data["snapshot"], data["scenario"], str(len(data["collections"])), str(len(data["buckets"]))]
    for collection in data["collections"]:
        words += [collection["id"], str(len(collection["prices"]))]
        for index, value in enumerate(collection["prices"]):
            words += [f"o{index:02}", str(value)]
    for bucket in data["buckets"]:
        words += [bucket["id"], bucket["collection"], bucket["wear"], str(bucket["cost"])]
    return " ".join(words)


def identity(data, quantities):
    pairs = sorted((bucket["id"], quantity) for bucket, quantity in zip(data["buckets"], quantities) if quantity)
    return f"contract-v1|{data['snapshot']}|{data['scenario']}|" + ",".join(f"{key}:{q}" for key, q in pairs)


def arithmetic_oracle(data, quantities):
    counts = {collection["id"]: 0 for collection in data["collections"]}
    cost = F(0)
    for bucket, quantity in zip(data["buckets"], quantities):
        counts[bucket["collection"]] += quantity
        cost += quantity * bucket["cost"]
    denominator = sum(counts[c["id"]] * len(c["prices"]) for c in data["collections"])
    distribution = {}
    gross = F(0)
    for collection in data["collections"]:
        n = counts[collection["id"]]
        if not n:
            continue
        for index, price in enumerate(collection["prices"]):
            probability = F(n, denominator)
            distribution[(collection["id"], f"o{index:02}")] = (probability, price, price * F(87, 100))
            gross += probability * price
    net = gross * F(87, 100)
    percent = 100 * net / cost
    worst = min(receipt for _, _, receipt in distribution.values())
    return [cost, gross, net, percent, percent - 100, net - cost, worst], distribution


def cartesian_oracle(data, lower_total, higher_total):
    # A different traversal: form full vectors including zeros, then filter them.
    # The production traversal recursively emits only active positive entries.
    buckets = data["buckets"]
    ranges = [range((lower_total if bucket["wear"] == "L" else higher_total) + 1) for bucket in buckets]
    result = {}
    for quantities in itertools.product(*ranges):
        if not 2 <= sum(q > 0 for q in quantities) <= 5:
            continue
        lower = sum(q for bucket, q in zip(buckets, quantities) if bucket["wear"] == "L")
        higher = sum(q for bucket, q in zip(buckets, quantities) if bucket["wear"] == "H")
        if (lower, higher) == (lower_total, higher_total):
            result[identity(data, quantities)] = quantities
    return result


def invoke(payload, *, success=True):
    process = subprocess.run([str(DRIVER)], input=payload, text=True, capture_output=True, timeout=20)
    if success and process.returncode:
        raise AssertionError(f"driver failed ({process.returncode}): {process.stdout}\n{process.stderr}")
    return process


def evaluate(data, quantities):
    allocations = [(bucket["id"], q) for bucket, q in zip(data["buckets"], quantities) if q]
    suffix = str(len(allocations)) + " " + " ".join(f"{name} {q}" for name, q in reversed(allocations))
    lines = invoke("eval " + encoded(data) + " " + suffix).stdout.splitlines()
    fields = lines[0].split("\t")
    if fields[0] != "E" or lines[-1] != "END":
        raise AssertionError("incomplete evaluation output")
    probabilities = {}
    for line in lines[1:-1]:
        tag, collection, outcome, probability, gross, net = line.split("\t")
        if tag != "P" or (collection, outcome) in probabilities:
            raise AssertionError("duplicate or malformed probability output")
        probabilities[(collection, outcome)] = (F(probability), F(gross), F(net))
    return list(map(F, fields[1:8])), probabilities, fields[8], fields[9]


class ReferenceVerification(unittest.TestCase):
    def test_01_existing_six_mathematical_fixtures(self):
        document = json.loads((ROOT / "tests/fixtures/numerical_cases.json").read_text())
        for case in document["cases"]:
            with self.subTest(case=case["id"]):
                data = {"snapshot": hashlib.sha256(json.dumps(case, sort_keys=True).encode()).hexdigest(),
                        "scenario": "fixture", "collections": [], "buckets": []}
                for index, (prices, cost) in enumerate(zip(case["outcome_prices"], case["unit_costs"])):
                    data["collections"].append({"id": f"c{index}", "prices": list(map(F, prices))})
                    data["buckets"].append({"id": f"b{index:02}", "collection": f"c{index}",
                                            "wear": "L" if index == 0 else "H", "cost": F(cost)})
                actual, probabilities, qualified, key = evaluate(data, case["quantities"])
                expected, distribution = arithmetic_oracle(data, case["quantities"])
                self.assertEqual(actual, expected)
                self.assertEqual(probabilities, distribution)
                self.assertEqual(sum(p for p, _, _ in probabilities.values()), 1)
                self.assertEqual([actual[i] for i in (0, 1, 2, 3, 6)],
                                 [F(case[name]) for name in ("cost", "gross_ev", "net_ev", "return_percent", "worst_net")])
                self.assertEqual(qualified, str(int(expected[3] > 115)))
                self.assertEqual(key, identity(data, case["quantities"]))
                STATISTICS["fixture_evaluations"] += 1

    def test_02_seeded_evaluator_and_probability_oracle(self):
        rng = random.Random(20261023)
        for index in range(40):
            bucket_count = rng.randrange(2, 6)
            data = dataset(1, bucket_count - 1, index + 1)
            for collection in data["collections"]:
                collection["prices"] = [F(rng.randrange(0, 1000), rng.randrange(1, 11))
                                        for _ in range(rng.randrange(1, 9))]
            cuts = sorted(rng.sample(range(1, 10), bucket_count - 1))
            points = [0] + cuts + [10]
            quantities = [b - a for a, b in zip(points, points[1:])]
            with self.subTest(instance=index):
                actual, probabilities, _, key = evaluate(data, quantities)
                expected, distribution = arithmetic_oracle(data, quantities)
                self.assertEqual(actual, expected)
                self.assertEqual(probabilities, distribution)
                self.assertEqual(sum(p for p, _, _ in probabilities.values()), 1)
                self.assertEqual(key, identity(data, quantities))
                STATISTICS["generated_evaluations"] += 1

    def test_03_cartesian_enumeration_and_qualification(self):
        layouts = [(1, 1), (1, 2), (2, 1), (2, 2), (1, 4), (3, 2), (3, 3), (2, 0), (0, 2)]
        for lower, higher in [(5, 5), (6, 4)]:
            for index, (n_lower, n_higher) in enumerate(layouts):
                with self.subTest(split=(lower, higher), catalog=(n_lower, n_higher)):
                    data = dataset(n_lower, n_higher, index)
                    expected = cartesian_oracle(data, lower, higher)
                    threshold = F(115)
                    command = "enum55" if lower == 5 else "enum64"
                    payload = command + " " + encoded(data) + f" {threshold} 10000 10000"
                    output = invoke(payload).stdout.splitlines()
                    stats = output[0].split("\t")
                    contracts = [line.split("\t")[1] for line in output if line.startswith("C\t")]
                    qualified = [line.split("\t")[1:] for line in output if line.startswith("Q\t")]
                    self.assertEqual(len(contracts), len(set(contracts)))
                    self.assertEqual(set(contracts), set(expected))
                    expected_values = {key: arithmetic_oracle(data, quantities)[0] for key, quantities in expected.items()}
                    expected_qualified = {key: values for key, values in expected_values.items() if values[3] > threshold}
                    self.assertEqual(len(qualified), len({line[0] for line in qualified}))
                    self.assertEqual({line[0]: list(map(F, line[1:])) for line in qualified}, expected_qualified)
                    self.assertEqual((int(stats[1]), int(stats[2])), (len(expected), len(expected_qualified)))
                    self.assertEqual(output[-1], "END")
                    STATISTICS["enumeration_comparisons"] += 1
                    STATISTICS["enumerated_vectors"] += len(contracts)

    def test_04_deterministic_order_under_catalog_permutation(self):
        data = dataset(3, 3, 19)
        original = invoke("enum55 " + encoded(data) + " 0 10000 10000").stdout
        data["buckets"].reverse()
        data["collections"].reverse()
        for collection in data["collections"]:
            collection["prices"].reverse()
        # Preserve outcome identities: reversing prices changes IDs assigned by the encoder,
        # so only verify contract order here; unit tests verify canonical outcome sorting.
        changed = invoke("enum55 " + encoded(data) + " 0 10000 10000").stdout
        self.assertEqual([line for line in original.splitlines() if line.startswith("C\t")],
                         [line for line in changed.splitlines() if line.startswith("C\t")])

    def test_05_exact_rational_differential(self):
        rng = random.Random(4319)
        operations = []
        for _ in range(1000):
            a = F(rng.randrange(-1_000_000, 1_000_001), rng.randrange(1, 10_001))
            b = F(rng.randrange(-1_000_000, 1_000_001), rng.randrange(1, 10_001))
            for op, value in [("+", a + b), ("-", a - b), ("*", a * b), ("cmp", F((a > b) - (a < b)))]:
                operations.append((op, a, b, value))
            if b:
                operations.append(("/", a, b, a / b))
        maximum = 2**63 - 1
        for _ in range(1000):
            a = F(rng.randrange(-maximum, maximum), rng.randrange(1, maximum))
            b = F(rng.randrange(-maximum, maximum), rng.randrange(1, maximum))
            operations.append(("cmp", a, b, F((a > b) - (a < b))))
        payload = "\n".join(f"calc {op} {a} {b}" for op, a, b, _ in operations)
        output = invoke(payload).stdout.splitlines()
        self.assertEqual(len(output), len(operations))
        for line, (op, a, b, expected) in zip(output, operations):
            with self.subTest(operation=op, left=a, right=b):
                status, value = line.split("\t")
                self.assertEqual(status, "OK")
                self.assertEqual(F(value), expected)
        STATISTICS["rational_operations"] = len(operations)

    def test_06_checked_overflow_and_large_cancellation(self):
        maximum = 2**63 - 1
        payload = f"calc * {maximum}/2 2/{maximum}\ncalc + {maximum} 1\ncalc cmp {maximum}/{maximum-1} {maximum-1}/{maximum}"
        lines = invoke(payload).stdout.splitlines()
        self.assertEqual(lines[0], "OK\t1/1")
        self.assertTrue(lines[1].startswith("ERR\toverflow\t"))
        self.assertEqual(lines[2], "OK\t1/1")
        # Documented intermediate bound: exact final representability alone is insufficient.
        line = invoke(f"calc + {maximum}/2 {maximum}/2").stdout
        self.assertTrue(line.startswith("ERR\toverflow\t"))

    def test_07_malformed_streams_and_missing_outcomes(self):
        for payload in ["eval truncated", "eval s s -1 2", "eval s s 1 1 c0 0 b0 c0 L 1 1 b0 10",
                        "calc + nan 1", "calc + inf 1", "calc + 1e3 1", "calc + 1/0 1"]:
            with self.subTest(payload=payload):
                process = invoke(payload, success=False)
                self.assertNotEqual(process.returncode, 0)
                self.assertIn("ERR\tinvalid\t", process.stdout)

    def test_08_budget_exhaustion_is_explicit(self):
        data = dataset(3, 3)
        process = invoke("enum55 " + encoded(data) + " 115 2 10000", success=False)
        self.assertNotEqual(process.returncode, 0)
        self.assertTrue(process.stdout.startswith("ERR\tlimit\t"))
        self.assertNotIn("END", process.stdout)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--driver", type=Path, required=True)
    args = parser.parse_args()
    DRIVER = args.driver.resolve()
    if not DRIVER.is_file():
        parser.error("compiled fixture driver not found")
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(ReferenceVerification)
    outcome = unittest.TextTestRunner(verbosity=2).run(suite)
    print("Independent reference checks:", json.dumps(STATISTICS, sort_keys=True))
    raise SystemExit(0 if outcome.wasSuccessful() else 1)
