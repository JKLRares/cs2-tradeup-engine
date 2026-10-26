"""Repeat bounded Release benchmarks; verify equal complete workloads before timing.

Writes raw timings and platform details. No reference extrapolation for larger catalogs.
"""
from __future__ import annotations

import argparse
import csv
import io
import json
from pathlib import Path
import platform
import statistics
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--driver', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=Path('benchmarks/results/search.json'))
    parser.add_argument('--compiler', default='g++')
    parser.add_argument('--repeats', type=int, default=5)
    args = parser.parse_args()
    if not 3 <= args.repeats <= 20:
        parser.error('use 3..20 repetitions')

    def invoke(mode, n, profile, flavor, repeats, pruning=1, scaled=1):
        command = [str(args.driver.resolve()), mode, str(n), profile, flavor, str(repeats), str(pruning), str(scaled)]
        output = subprocess.run(command, text=True, capture_output=True, check=True, timeout=60)
        return output.stdout

    cpu = platform.processor()
    if platform.system() == 'Windows':
        import winreg
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r'HARDWARE\DESCRIPTION\System\CentralProcessor\0') as key:
            cpu = winreg.QueryValueEx(key, 'ProcessorNameString')[0].strip()
    compiler = subprocess.run([args.compiler, '--version'], text=True, capture_output=True, check=True).stdout.strip()
    document = {'cpu': cpu, 'platform': platform.platform(), 'python': platform.python_version(),
                'compiler': compiler, 'flags': '-std=c++20 -O2 -DNDEBUG -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror',
                'seed': 20261026, 'repetitions': args.repeats,
                'limits': {'max_evaluations': 100000, 'max_work': 2000000, 'max_results': 100000,
                           'max_result_bytes': 67108864, 'max_time_ms': 10000},
                'measurements': []}
    cases = [(n, p, 'mixed') for n in (4, 8, 12) for p in ('55', '64')]
    cases += [(12, '55', f) for f in ('none', 'all')]
    verified_contracts = 0
    for n, profile, flavor in cases:
        verification = invoke('verify', n, profile, flavor, 1)
        verified_contracts += int(verification.strip().split(',')[1])
        for mode in ('reference', 'optimized', 'expanded'):
            # Separate processes per engine: Windows peak working set cannot leak across engines.
            rows = list(csv.DictReader(io.StringIO(invoke(mode, n, profile, flavor, args.repeats))))
            item = {'engine': mode, 'buckets': n, 'profile': profile, 'flavor': flavor,
                    'pruning': True, 'scaled': True, 'raw': rows}
            item['median_search_s'] = statistics.median(float(r['search_s']) for r in rows)
            item['median_preparation_s'] = statistics.median(float(r['preparation_s']) for r in rows)
            item['min_search_s'] = min(float(r['search_s']) for r in rows)
            item['max_search_s'] = max(float(r['search_s']) for r in rows)
            document['measurements'].append(item)
            print(mode, n, profile, flavor, f"{item['median_search_s']:.6f}s", rows[0]['status'], flush=True)
    # Ablations and optimized-only finite prefixes; no uncontrolled full large search.
    for n, profile, flavor, pruning, scaled in [
        (12, '55', 'mixed', 0, 1), (12, '55', 'mixed', 1, 0), (12, '55', 'mixed', 0, 0),
        (16, '55', 'mixed', 1, 1), (32, '55', 'mixed', 1, 1), (58, '55', 'mixed', 1, 1),
        (58, '64', 'mixed', 1, 1), (58, '55', 'none', 1, 1)]:
        if n <= 12:
            invoke('verify', n, profile, flavor, 1, pruning, scaled)
        rows = list(csv.DictReader(io.StringIO(invoke('optimized', n, profile, flavor, args.repeats, pruning, scaled))))
        item = {'engine': 'optimized', 'buckets': n, 'profile': profile, 'flavor': flavor,
                'pruning': bool(pruning), 'scaled': bool(scaled), 'raw': rows,
                'median_search_s': statistics.median(float(r['search_s']) for r in rows),
                'median_preparation_s': statistics.median(float(r['preparation_s']) for r in rows)}
        document['measurements'].append(item)
        print('optimized', n, profile, flavor, f"prune={pruning} scale={scaled}",
              f"{item['median_search_s']:.6f}s", rows[0]['status'], flush=True)
    document['same_workload_verified_contracts'] = verified_contracts
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(document, indent=2) + '\n')
    print('Raw benchmark results:', args.output)


if __name__ == '__main__':
    main()
