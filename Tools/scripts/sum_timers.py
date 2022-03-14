"""Print a summary of specialization stats for all files in the
default stats folders.
"""

import collections
import os.path
import opcode
from datetime import date
import itertools
import argparse

if os.name == "nt":
    DEFAULT_DIR = "c:\\temp\\py_timings\\"
else:
    DEFAULT_DIR = "/tmp/py_timings/"

def gather_timings():
    timings = collections.Counter()
    for filename in os.listdir(DEFAULT_DIR):
        with open(os.path.join(DEFAULT_DIR, filename)) as fd:
            for line in fd:
                key, value = line.split(":")
                key = key.strip()
                value = int(value)
                timings[key] += value
    return timings


def format_time(t):
    if t == 0:
        return "0"
    if t >= 1_000_000:
        div = 1_000_000
        units = "secs"
    elif t >= 1_000:
        div = 1_000
        units = "ms"
    else:
        div = 1
        units = "μs"
    return f"{t/div:0.1f} {units}"


def main():
    timings = gather_timings()
    total = sum(timings.values())
    for key, time in sorted(timings.items()):
        print(f"{key}: {format_time(time)} {100*time/total:0.1f}%")

if __name__ == "__main__":
    main()
