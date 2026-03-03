#!/usr/bin/env python3
"""Benchmark Python-level unicodedata.category() lookups.

Runs three fixed workloads:
- all Unicode code points
- BMP only
- ASCII only
"""

from __future__ import annotations

import statistics
import time
import unicodedata


LOOPS = 5
SAMPLES = 7
DATASETS = {
    "all": "".join(map(chr, range(0x110000))),
    "bmp": "".join(map(chr, range(0x10000))),
    "ascii": "".join(map(chr, range(0x80))),
}


def run_once(chars: str) -> tuple[float, int]:
    category = unicodedata.category
    checksum = 0
    t0 = time.perf_counter()
    for _ in range(LOOPS):
        for ch in chars:
            gc = category(ch)
            checksum += ord(gc[0]) + ord(gc[1])
    elapsed = time.perf_counter() - t0
    return elapsed, checksum


def benchmark(name: str, chars: str) -> None:
    lookups = len(chars) * LOOPS

    # Warm up specialization and caches before timing.
    run_once(chars)

    samples = []
    checksum = None
    for _ in range(SAMPLES):
        elapsed, checksum = run_once(chars)
        samples.append(elapsed)

    best = min(samples)
    median = statistics.median(samples)
    mean = statistics.fmean(samples)

    print(f"dataset: {name}")
    print(f"codepoints: {len(chars)}")
    print(f"lookups/sample: {lookups}")
    print(f"checksum: {checksum}")
    print(f"best_s: {best:.6f}")
    print(f"median_s: {median:.6f}")
    print(f"mean_s: {mean:.6f}")
    print(f"best_ns_per_lookup: {best * 1e9 / lookups:.2f}")
    print(f"median_ns_per_lookup: {median * 1e9 / lookups:.2f}")
    print()


def main() -> None:
    print(f"python: {unicodedata.unidata_version=}")
    print(f"samples: {SAMPLES}")
    print(f"loops: {LOOPS}")
    print()

    benchmark("all", DATASETS["all"])
    benchmark("bmp", DATASETS["bmp"])
    benchmark("ascii", DATASETS["ascii"])


if __name__ == "__main__":
    main()
