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

def print_stats(name, stats):
    total = sum(stats[kind] for kind in TOTAL)
    if total == 0:
        return
    print(name+":")
    for key in sorted(stats):
        if not key.startswith("specialization"):
            print(f"{key:>12}:{stats[key]:>12} {100*stats[key]/total:0.1f}%")
    for key in ("specialization_success",  "specialization_failure"):
        print(f"  {key}:{stats[key]:>12}")

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
