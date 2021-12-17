"""Print a summary of specialization stats for all files in the
default stats folders.
"""

import collections
import os.path

if os.name == "nt":
    DEFAULT_DIR = "c:\\temp\\py_stats\\"
else:
    DEFAULT_DIR = "/tmp/py_stats/"


TOTAL = "specialization.deferred", "specialization.hit", "specialization.miss", "count"

def print_stats(name, family_stats):
    total = sum(family_stats[kind] for kind in TOTAL)
    if total == 0:
        return
    print(name+":")
    for key in sorted(family_stats):
        if key.startswith("specialization.failure_kinds"):
            continue
        if key.startswith("specialization."):
            label = key[len("specialization."):]
        elif key == "count":
            label = "unquickened"
        if key not in ("specialization.success",  "specialization.failure"):
            print(f"{label:>12}:{family_stats[key]:>12} {100*family_stats[key]/total:0.1f}%")
    for key in ("specialization.success",  "specialization.failure"):
        label = key[len("specialization."):]
        print(f"  {label}:{family_stats[key]:>12}")
    total_failures = family_stats["specialization.failure"]
    failure_kinds = [ 0 ] * 30
    for key in family_stats:
        if not key.startswith("specialization.failure_kind"):
            continue
        _, index = key[:-1].split("[")
        index =  int(index)
        failure_kinds[index] = family_stats[key]
    for index, value in enumerate(failure_kinds):
        if not value:
            continue
        print(f"    kind {index:>2}: {value:>8} {100*value/total_failures:0.1f}%")

def main():
    stats = collections.defaultdict(collections.Counter)
    for filename in os.listdir(DEFAULT_DIR):
        for line in open(os.path.join(DEFAULT_DIR, filename)):
            key, value = line.split(":")
            key = key.strip()
            family, _, stat = key.partition(".")
            value = int(value.strip())
            stats[family][stat] += value

    for name in sorted(stats):
        print_stats(name, stats[name])

if __name__ == "__main__":
    main()
