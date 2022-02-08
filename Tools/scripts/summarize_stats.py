"""Print a summary of specialization stats for all files in the
default stats folders.
"""

import collections
import os.path
import opcode

if os.name == "nt":
    DEFAULT_DIR = "c:\\temp\\py_stats\\"
else:
    DEFAULT_DIR = "/tmp/py_stats/"

#Create list of all instruction names
specialized = iter(opcode._specialized_instructions)
opname = ["<0>"]
for name in opcode.opname[1:]:
    if name.startswith("<"):
        try:
            name = next(specialized)
        except StopIteration:
            pass
    opname.append(name)

TOTAL = "specialization.deferred", "specialization.hit", "specialization.miss", "execution_count"

def print_specialization_stats(name, family_stats):
    if "specializable" not in family_stats:
        return
    total = sum(family_stats.get(kind, 0) for kind in TOTAL)
    if total == 0:
        return
    print(name+":")
    for key in sorted(family_stats):
        if key.startswith("specialization.failure_kinds"):
            continue
        if key.startswith("specialization."):
            label = key[len("specialization."):]
        elif key == "execution_count":
            label = "unquickened"
        if key not in ("specialization.success",  "specialization.failure"):
            print(f"{label:>12}:{family_stats[key]:>12} {100*family_stats[key]/total:0.1f}%")
    for key in ("specialization.success",  "specialization.failure"):
        label = key[len("specialization."):]
        print(f"  {label}:{family_stats.get(key, 0):>12}")
    total_failures = family_stats.get("specialization.failure", 0)
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

def gather_stats():
    stats = collections.Counter()
    for filename in os.listdir(DEFAULT_DIR):
        with open(os.path.join(DEFAULT_DIR, filename)) as fd:
            for line in fd:
                key, value = line.split(":")
                key = key.strip()
                value = int(value.strip())
                stats[key] += value
    return stats

def extract_opcode_stats(stats):
    opcode_stats = [ {} for _ in range(256) ]
    for key, value in stats.items():
        if not key.startswith("opcode"):
            continue
        n, _, rest = key[7:].partition("]")
        opcode_stats[int(n)][rest.strip(".")] = value
    return opcode_stats


def categorized_counts(opcode_stats):
    basic = 0
    specialized = 0
    not_specialized = 0
    specialized_instructions = {
        op for op in opcode._specialized_instructions
        if "__" not in op and "ADAPTIVE" not in op}
    adaptive_instructions = {
        op for op in opcode._specialized_instructions
        if "ADAPTIVE" in op}
    for i, opcode_stat in enumerate(opcode_stats):
        if "execution_count" not in opcode_stat:
            continue
        count = opcode_stat['execution_count']
        name = opname[i]
        if "specializable" in opcode_stat:
            not_specialized += count
        elif name in adaptive_instructions:
            not_specialized += count
        elif name in specialized_instructions:
            miss = opcode_stat.get("specialization.miss", 0)
            not_specialized += miss
            specialized += count - miss
        else:
            basic += count
    return basic, not_specialized, specialized

def main():
    stats = gather_stats()
    opcode_stats = extract_opcode_stats(stats)
    print("Execution counts:")
    counts = []
    total = 0
    for i, opcode_stat in enumerate(opcode_stats):
        if "execution_count" in opcode_stat:
            count = opcode_stat['execution_count']
            miss = 0
            if "specializable" not in opcode_stat:
                miss = opcode_stat.get("specialization.miss")
            counts.append((count, opname[i], miss))
            total += count
    counts.sort(reverse=True)
    cummulative = 0
    for (count, name, miss) in counts:
        cummulative += count
        print(f"{name}: {count} {100*count/total:0.1f}% {100*cummulative/total:0.1f}%")
        if miss:
            print(f"    Misses: {miss} {100*miss/count:0.1f}%")
    print("Specialization stats:")
    for i, opcode_stat in enumerate(opcode_stats):
        name = opname[i]
        print_specialization_stats(name, opcode_stat)
    basic, not_specialized, specialized = categorized_counts(opcode_stats)
    print("Specialization effectiveness:")
    print(f"    Base instructions {basic} {basic*100/total:0.1f}%")
    print(f"    Not specialized {not_specialized} {not_specialized*100/total:0.1f}%")
    print(f"    Specialized {specialized} {specialized*100/total:0.1f}%")
    print("Call stats:")
    total = 0
    for key, value in stats.items():
        if "Calls to" in key:
            total += value
    for key, value in stats.items():
        if "Calls to" in key:
            print(f"    {key}: {value} {100*value/total:0.1f}%")
    for key, value in stats.items():
        if key.startswith("Frame"):
            print(f"    {key}: {value} {100*value/total:0.1f}%")
    print("Object stats:")
    total = stats.get("Object new values")
    for key, value in stats.items():
        if key.startswith("Object"):
            if "materialize" in key:
                print(f"    {key}: {value} {100*value/total:0.1f}%")
            else:
                print(f"    {key}: {value}")
    total = 0


if __name__ == "__main__":
    main()
