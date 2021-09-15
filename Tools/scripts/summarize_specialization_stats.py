"""Print a summary of specialization stats for all files in the
default stats folders.
"""

import collections
import os.path

if os.name == "nt":
    DEFAULT_DIR = "c:\\temp\\py_stats\\"
else:
    DEFAULT_DIR = "/tmp/py_stats/"


TOTAL = "deferred", "hit", "miss", "unquickened"

def print_stats(name, family_stats):
    total = sum(family_stats[kind] for kind in TOTAL)
    if total == 0:
        return
    print(name+":")
    for key in sorted(family_stats):
        if not key.startswith("specialization"):
            print(f"{key:>12}:{family_stats[key]:>12} {100*family_stats[key]/total:0.1f}%")
    for key in ("specialization_success",  "specialization_failure"):
        print(f"  {key}:{family_stats[key]:>12}")
    total_failures = family_stats["specialization_failure"]
    failure_kinds = [ 0 ] * 20
    for key in family_stats:
        if not key.startswith("specialization_failure_kind"):
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
            family, stat = key.split(".")
            value = int(value.strip())
            stats[family][stat] += value

    for name in sorted(stats):
        print_stats(name, stats[name])

if __name__ == "__main__":
    main()
