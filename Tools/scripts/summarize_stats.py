"""Print a summary of specialization stats for all files in the
default stats folders.
"""

# NOTE: Bytecode introspection modules (opcode, dis, etc.) should only
# happen when loading a single dataset. When comparing datasets, it
# could get it wrong, leading to subtle errors.

import argparse
import collections
import json
import os.path
from datetime import date
import itertools
import sys
import re

if os.name == "nt":
    DEFAULT_DIR = "c:\\temp\\py_stats\\"
else:
    DEFAULT_DIR = "/tmp/py_stats/"

TOTAL = "specialization.hit", "specialization.miss", "execution_count"

def format_ratio(num, den):
    """
    Format a ratio as a percentage. When the denominator is 0, returns the empty
    string.
    """
    if den == 0:
        return ""
    else:
        return f"{num/den:.01%}"

def percentage_to_float(s):
    """
    Converts a percentage string to a float.  The empty string is returned as 0.0
    """
    if s == "":
        return 0.0
    else:
        assert s[-1] == "%"
        return float(s[:-1])

def join_rows(a_rows, b_rows):
    """
    Joins two tables together, side-by-side, where the first column in each is a
    common key.
    """
    if len(a_rows) == 0 and len(b_rows) == 0:
        return []

    if len(a_rows):
        a_ncols = list(set(len(x) for x in a_rows))
        if len(a_ncols) != 1:
            raise ValueError("Table a is ragged")

    if len(b_rows):
        b_ncols = list(set(len(x) for x in b_rows))
        if len(b_ncols) != 1:
            raise ValueError("Table b is ragged")

    if len(a_rows) and len(b_rows) and a_ncols[0] != b_ncols[0]:
        raise ValueError("Tables have different widths")

    if len(a_rows):
        ncols = a_ncols[0]
    else:
        ncols = b_ncols[0]

    default = [""] * (ncols - 1)
    a_data = {x[0]: x[1:] for x in a_rows}
    b_data = {x[0]: x[1:] for x in b_rows}

    if len(a_data) != len(a_rows) or len(b_data) != len(b_rows):
        raise ValueError("Duplicate keys")

    # To preserve ordering, use A's keys as is and then add any in B that aren't
    # in A
    keys = list(a_data.keys()) + [k for k in b_data.keys() if k not in a_data]
    return [(k, *a_data.get(k, default), *b_data.get(k, default)) for k in keys]

def calculate_specialization_stats(family_stats, total):
    rows = []
    for key in sorted(family_stats):
        if key.startswith("specialization.failure_kinds"):
            continue
        if key in ("specialization.hit", "specialization.miss"):
            label = key[len("specialization."):]
        elif key == "execution_count":
            continue
        elif key in ("specialization.success",  "specialization.failure", "specializable"):
            continue
        elif key.startswith("pair"):
            continue
        else:
            label = key
        rows.append((f"{label:>12}", f"{family_stats[key]:>12}", format_ratio(family_stats[key], total)))
    return rows

def calculate_specialization_success_failure(family_stats):
    total_attempts = 0
    for key in ("specialization.success",  "specialization.failure"):
        total_attempts += family_stats.get(key, 0)
    rows = []
    if total_attempts:
        for key in ("specialization.success",  "specialization.failure"):
            label = key[len("specialization."):]
            label = label[0].upper() + label[1:]
            val = family_stats.get(key, 0)
            rows.append((label, val, format_ratio(val, total_attempts)))
    return rows

def calculate_specialization_failure_kinds(name, family_stats, defines):
    total_failures = family_stats.get("specialization.failure", 0)
    failure_kinds = [ 0 ] * 40
    for key in family_stats:
        if not key.startswith("specialization.failure_kind"):
            continue
        _, index = key[:-1].split("[")
        index = int(index)
        failure_kinds[index] = family_stats[key]
    failures = [(value, index) for (index, value) in enumerate(failure_kinds)]
    failures.sort(reverse=True)
    rows = []
    for value, index in failures:
        if not value:
            continue
        rows.append((kind_to_text(index, defines, name), value, format_ratio(value, total_failures)))
    return rows

def print_specialization_stats(name, family_stats, defines):
    if "specializable" not in family_stats:
        return
    total = sum(family_stats.get(kind, 0) for kind in TOTAL)
    if total == 0:
        return
    with Section(name, 3, f"specialization stats for {name} family"):
        rows = calculate_specialization_stats(family_stats, total)
        emit_table(("Kind", "Count", "Ratio"), rows)
        rows = calculate_specialization_success_failure(family_stats)
        if rows:
            print_title("Specialization attempts", 4)
            emit_table(("", "Count:", "Ratio:"), rows)
            rows = calculate_specialization_failure_kinds(name, family_stats, defines)
            emit_table(("Failure kind", "Count:", "Ratio:"), rows)

def print_comparative_specialization_stats(name, base_family_stats, head_family_stats, defines):
    if "specializable" not in base_family_stats:
        return

    base_total = sum(base_family_stats.get(kind, 0) for kind in TOTAL)
    head_total = sum(head_family_stats.get(kind, 0) for kind in TOTAL)
    if base_total + head_total == 0:
        return
    with Section(name, 3, f"specialization stats for {name} family"):
        base_rows = calculate_specialization_stats(base_family_stats, base_total)
        head_rows = calculate_specialization_stats(head_family_stats, head_total)
        emit_table(
            ("Kind", "Base Count", "Base Ratio", "Head Count", "Head Ratio"),
            join_rows(base_rows, head_rows)
        )
        base_rows = calculate_specialization_success_failure(base_family_stats)
        head_rows = calculate_specialization_success_failure(head_family_stats)
        rows = join_rows(base_rows, head_rows)
        if rows:
            print_title("Specialization attempts", 4)
            emit_table(("", "Base Count:", "Base Ratio:", "Head Count:", "Head Ratio:"), rows)
            base_rows = calculate_specialization_failure_kinds(name, base_family_stats, defines)
            head_rows = calculate_specialization_failure_kinds(name, head_family_stats, defines)
            emit_table(
                ("Failure kind", "Base Count:", "Base Ratio:", "Head Count:", "Head Ratio:"),
                join_rows(base_rows, head_rows)
            )

def gather_stats(input):
    # Note the output of this function must be JSON-serializable

    if os.path.isfile(input):
        with open(input, "r") as fd:
            stats = json.load(fd)

        stats["_stats_defines"] = {int(k): v for k, v in stats["_stats_defines"].items()}
        stats["_defines"] = {int(k): v for k, v in stats["_defines"].items()}
        return stats

    elif os.path.isdir(input):
        stats = collections.Counter()
        for filename in os.listdir(input):
            with open(os.path.join(input, filename)) as fd:
                for line in fd:
                    try:
                        key, value = line.split(":")
                    except ValueError:
                        print(f"Unparsable line: '{line.strip()}' in  {filename}", file=sys.stderr)
                        continue
                    key = key.strip()
                    value = int(value)
                    stats[key] += value
            stats['__nfiles__'] += 1

        import opcode

        stats["_specialized_instructions"] = [
            op for op in opcode._specialized_opmap.keys()
            if "__" not in op
        ]
        stats["_stats_defines"] = get_stats_defines()
        stats["_defines"] = get_defines()

        return stats
    else:
        raise ValueError(f"{input:r} is not a file or directory path")

def extract_opcode_stats(stats, prefix):
    opcode_stats = collections.defaultdict(dict)
    for key, value in stats.items():
        if not key.startswith(prefix):
            continue
        name, _, rest = key[len(prefix) + 1:].partition("]")
        opcode_stats[name][rest.strip(".")] = value
    return opcode_stats

def parse_kinds(spec_src, prefix="SPEC_FAIL"):
    defines = collections.defaultdict(list)
    start = "#define " + prefix + "_"
    for line in spec_src:
        line = line.strip()
        if not line.startswith(start):
            continue
        line = line[len(start):]
        name, val = line.split()
        defines[int(val.strip())].append(name.strip())
    return defines

def pretty(defname):
    return defname.replace("_", " ").lower()

def kind_to_text(kind, defines, opname):
    if kind <= 8:
        return pretty(defines[kind][0])
    if opname == "LOAD_SUPER_ATTR":
        opname = "SUPER"
    elif opname.endswith("ATTR"):
        opname = "ATTR"
    elif opname in ("FOR_ITER", "SEND"):
        opname = "ITER"
    elif opname.endswith("SUBSCR"):
        opname = "SUBSCR"
    for name in defines[kind]:
        if name.startswith(opname):
            return pretty(name[len(opname)+1:])
    return "kind " + str(kind)

def categorized_counts(opcode_stats, specialized_instructions):
    basic = 0
    specialized = 0
    not_specialized = 0
    for name, opcode_stat in opcode_stats.items():
        if "execution_count" not in opcode_stat:
            continue
        count = opcode_stat['execution_count']
        if "specializable" in opcode_stat:
            not_specialized += count
        elif name in specialized_instructions:
            miss = opcode_stat.get("specialization.miss", 0)
            not_specialized += miss
            specialized += count - miss
        else:
            basic += count
    return basic, not_specialized, specialized

def print_title(name, level=2):
    print("#"*level, name)
    print()

class Section:

    def __init__(self, title, level=2, summary=None):
        self.title = title
        self.level = level
        if summary is None:
            self.summary = title.lower()
        else:
            self.summary = summary

    def __enter__(self):
        print_title(self.title, self.level)
        print("<details>")
        print("<summary>", self.summary, "</summary>")
        print()
        return self

    def __exit__(*args):
        print()
        print("</details>")
        print()

def to_str(x):
    if isinstance(x, int):
        return format(x, ",d")
    else:
        return str(x)

def emit_table(header, rows):
    width = len(header)
    header_line = "|"
    under_line = "|"
    for item in header:
        under = "---"
        if item.endswith(":"):
            item = item[:-1]
            under += ":"
        header_line += item + " | "
        under_line += under + "|"
    print(header_line)
    print(under_line)
    for row in rows:
        if width is not None and len(row) != width:
            raise ValueError("Wrong number of elements in row '" + str(row) + "'")
        print("|", " | ".join(to_str(i) for i in row), "|")
    print()

def emit_histogram(title, stats, key, total):
    rows = []
    for k, v in stats.items():
        if k.startswith(key):
            entry = int(re.match(r".+\[([0-9]+)\]", k).groups()[0])
            rows.append((f"<= {entry}", int(v), format_ratio(int(v), total)))
    print(f"**{title}**\n")
    emit_table(("Range", "Count:", "Ratio:"), rows)

def calculate_execution_counts(opcode_stats, total):
    counts = []
    for name, opcode_stat in opcode_stats.items():
        if "execution_count" in opcode_stat:
            count = opcode_stat['execution_count']
            miss = 0
            if "specializable" not in opcode_stat:
                miss = opcode_stat.get("specialization.miss")
            counts.append((count, name, miss))
    counts.sort(reverse=True)
    cumulative = 0
    rows = []
    for (count, name, miss) in counts:
        cumulative += count
        if miss:
            miss = format_ratio(miss, count)
        else:
            miss = ""
        rows.append((name, count, format_ratio(count, total),
                     format_ratio(cumulative, total), miss))
    return rows

def emit_execution_counts(opcode_stats, total):
    with Section("Execution counts", summary="execution counts for all instructions"):
        rows = calculate_execution_counts(opcode_stats, total)
        emit_table(
            ("Name", "Count:", "Self:", "Cumulative:", "Miss ratio:"),
            rows
        )

def _emit_comparative_execution_counts(base_rows, head_rows):
    base_data = {x[0]: x[1:] for x in base_rows}
    head_data = {x[0]: x[1:] for x in head_rows}
    opcodes = base_data.keys() | head_data.keys()

    rows = []
    default = [0, "0.0%", "0.0%", 0]
    for opcode in opcodes:
        base_entry = base_data.get(opcode, default)
        head_entry = head_data.get(opcode, default)
        if base_entry[0] == 0:
            change = 1
        else:
            change = (head_entry[0] - base_entry[0]) / base_entry[0]
        rows.append(
            (opcode, base_entry[0], head_entry[0],
                f"{change:0.1%}"))

    rows.sort(key=lambda x: abs(percentage_to_float(x[-1])), reverse=True)

    emit_table(
        ("Name", "Base Count:", "Head Count:", "Change:"),
        rows
    )

def emit_comparative_execution_counts(
    base_opcode_stats, base_total, head_opcode_stats, head_total, level=2
):
    with Section("Execution counts", summary="execution counts for all instructions", level=level):
        base_rows = calculate_execution_counts(base_opcode_stats, base_total)
        head_rows = calculate_execution_counts(head_opcode_stats, head_total)
        _emit_comparative_execution_counts(base_rows, head_rows)

def get_defines():
    spec_path = os.path.join(os.path.dirname(__file__), "../../Python/specialize.c")
    with open(spec_path) as spec_src:
        defines = parse_kinds(spec_src)
    return defines

def emit_specialization_stats(opcode_stats, defines):
    with Section("Specialization stats", summary="specialization stats by family"):
        for name, opcode_stat in opcode_stats.items():
            print_specialization_stats(name, opcode_stat, defines)

def emit_comparative_specialization_stats(base_opcode_stats, head_opcode_stats, defines):
    with Section("Specialization stats", summary="specialization stats by family"):
        opcodes = set(base_opcode_stats.keys()) & set(head_opcode_stats.keys())
        for opcode in opcodes:
            print_comparative_specialization_stats(
                opcode, base_opcode_stats[opcode], head_opcode_stats[opcode], defines
            )

def calculate_specialization_effectiveness(
    opcode_stats, total, specialized_instructions
):
    basic, not_specialized, specialized = categorized_counts(
        opcode_stats, specialized_instructions
    )
    return [
        ("Basic", basic, format_ratio(basic, total)),
        ("Not specialized", not_specialized, format_ratio(not_specialized, total)),
        ("Specialized", specialized, format_ratio(specialized, total)),
    ]

def emit_specialization_overview(opcode_stats, total, specialized_instructions):
    with Section("Specialization effectiveness"):
        rows = calculate_specialization_effectiveness(opcode_stats, total, specialized_instructions)
        emit_table(("Instructions", "Count:", "Ratio:"), rows)
        for title, field in (("Deferred", "specialization.deferred"), ("Misses", "specialization.miss")):
            total = 0
            counts = []
            for name, opcode_stat in opcode_stats.items():
                # Avoid double counting misses
                if title == "Misses" and "specializable" in opcode_stat:
                    continue
                value = opcode_stat.get(field, 0)
                counts.append((value, name))
                total += value
            counts.sort(reverse=True)
            if total:
                with Section(f"{title} by instruction", 3):
                    rows = [ (name, count, format_ratio(count, total)) for (count, name) in counts[:10] ]
                    emit_table(("Name", "Count:", "Ratio:"), rows)

def emit_comparative_specialization_overview(
    base_opcode_stats, base_total, head_opcode_stats, head_total, specialized_instructions
):
    with Section("Specialization effectiveness"):
        base_rows = calculate_specialization_effectiveness(
            base_opcode_stats, base_total, specialized_instructions
        )
        head_rows = calculate_specialization_effectiveness(
            head_opcode_stats, head_total, specialized_instructions
        )
        emit_table(
            ("Instructions", "Base Count:", "Base Ratio:", "Head Count:", "Head Ratio:"),
            join_rows(base_rows, head_rows)
        )

def get_stats_defines():
    stats_path = os.path.join(os.path.dirname(__file__), "../../Include/cpython/pystats.h")
    with open(stats_path) as stats_src:
        defines = parse_kinds(stats_src, prefix="EVAL_CALL")
    return defines

def calculate_call_stats(stats, defines):
    total = 0
    for key, value in stats.items():
        if "Calls to" in key:
            total += value
            rows = []
    for key, value in stats.items():
        if "Calls to" in key:
            rows.append((key, value, format_ratio(value, total)))
        elif key.startswith("Calls "):
            name, index = key[:-1].split("[")
            index =  int(index)
            label = name + " (" + pretty(defines[index][0]) + ")"
            rows.append((label, value, format_ratio(value, total)))
    for key, value in stats.items():
        if key.startswith("Frame"):
            rows.append((key, value, format_ratio(value, total)))
    return rows

def emit_call_stats(stats, defines):
    with Section("Call stats", summary="Inlined calls and frame stats"):
        rows = calculate_call_stats(stats, defines)
        emit_table(("", "Count:", "Ratio:"), rows)

def emit_comparative_call_stats(base_stats, head_stats, defines):
    with Section("Call stats", summary="Inlined calls and frame stats"):
        base_rows = calculate_call_stats(base_stats, defines)
        head_rows = calculate_call_stats(head_stats, defines)
        rows = join_rows(base_rows, head_rows)
        rows.sort(key=lambda x: -percentage_to_float(x[-1]))
        emit_table(
            ("", "Base Count:", "Base Ratio:", "Head Count:", "Head Ratio:"),
            rows
        )

def calculate_object_stats(stats):
    total_materializations = stats.get("Object new values")
    total_allocations = stats.get("Object allocations") + stats.get("Object allocations from freelist")
    total_increfs = stats.get("Object interpreter increfs") + stats.get("Object increfs")
    total_decrefs = stats.get("Object interpreter decrefs") + stats.get("Object decrefs")
    rows = []
    for key, value in stats.items():
        if key.startswith("Object"):
            if "materialize" in key:
                ratio = format_ratio(value, total_materializations)
            elif "allocations" in key:
                ratio = format_ratio(value, total_allocations)
            elif "increfs"     in key:
                ratio = format_ratio(value, total_increfs)
            elif "decrefs"     in key:
                ratio = format_ratio(value, total_decrefs)
            else:
                ratio = ""
            label = key[6:].strip()
            label = label[0].upper() + label[1:]
            rows.append((label, value, ratio))
    return rows

def calculate_gc_stats(stats):
    gc_stats = []
    for key, value in stats.items():
        if not key.startswith("GC"):
            continue
        n, _, rest = key[3:].partition("]")
        name = rest.strip()
        gen_n = int(n)
        while len(gc_stats) <= gen_n:
            gc_stats.append({})
        gc_stats[gen_n][name] = value
    return [
        (i, gen["collections"], gen["objects collected"], gen["object visits"])
        for (i, gen) in enumerate(gc_stats)
    ]

def emit_object_stats(stats):
    with Section("Object stats", summary="allocations, frees and dict materializatons"):
        rows = calculate_object_stats(stats)
        emit_table(("",  "Count:", "Ratio:"), rows)

def emit_comparative_object_stats(base_stats, head_stats):
    with Section("Object stats", summary="allocations, frees and dict materializatons"):
        base_rows = calculate_object_stats(base_stats)
        head_rows = calculate_object_stats(head_stats)
        emit_table(("",  "Base Count:", "Base Ratio:", "Head Count:", "Head Ratio:"), join_rows(base_rows, head_rows))

def emit_gc_stats(stats):
    with Section("GC stats", summary="GC collections and effectiveness"):
        rows = calculate_gc_stats(stats)
        emit_table(("Generation:",  "Collections:", "Objects collected:", "Object visits:"), rows)

def emit_comparative_gc_stats(base_stats, head_stats):
    with Section("GC stats", summary="GC collections and effectiveness"):
        base_rows = calculate_gc_stats(base_stats)
        head_rows = calculate_gc_stats(head_stats)
        emit_table(
            ("Generation:",
            "Base collections:", "Head collections:",
            "Base objects collected:", "Head objects collected:",
            "Base object visits:", "Head object visits:"),
            join_rows(base_rows, head_rows))

def get_total(opcode_stats):
    total = 0
    for opcode_stat in opcode_stats.values():
        if "execution_count" in opcode_stat:
            total += opcode_stat['execution_count']
    return total

def emit_pair_counts(opcode_stats, total):
    pair_counts = []
    for name_i, opcode_stat in opcode_stats.items():
        for key, value in opcode_stat.items():
            if key.startswith("pair_count"):
                name_j, _, _ = key[11:].partition("]")
                if value:
                    pair_counts.append((value, (name_i, name_j)))
    with Section("Pair counts", summary="Pair counts for top 100 pairs"):
        pair_counts.sort(reverse=True)
        cumulative = 0
        rows = []
        for (count, pair) in itertools.islice(pair_counts, 100):
            name_i, name_j = pair
            cumulative += count
            rows.append((f"{name_i} {name_j}", count, format_ratio(count, total),
                         format_ratio(cumulative, total)))
        emit_table(("Pair", "Count:", "Self:", "Cumulative:"),
            rows
        )
    with Section("Predecessor/Successor Pairs", summary="Top 5 predecessors and successors of each opcode"):
        predecessors = collections.defaultdict(collections.Counter)
        successors = collections.defaultdict(collections.Counter)
        total_predecessors = collections.Counter()
        total_successors = collections.Counter()
        for count, (first, second) in pair_counts:
            if count:
                predecessors[second][first] = count
                successors[first][second] = count
                total_predecessors[second] += count
                total_successors[first] += count
        for name in opcode_stats.keys():
            total1 = total_predecessors[name]
            total2 = total_successors[name]
            if total1 == 0 and total2 == 0:
                continue
            pred_rows = succ_rows = ()
            if total1:
                pred_rows = [(pred, count, f"{count/total1:.1%}")
                             for (pred, count) in predecessors[name].most_common(5)]
            if total2:
                succ_rows = [(succ, count, f"{count/total2:.1%}")
                             for (succ, count) in successors[name].most_common(5)]
            with Section(name, 3, f"Successors and predecessors for {name}"):
                emit_table(("Predecessors", "Count:", "Percentage:"),
                    pred_rows
                )
                emit_table(("Successors", "Count:", "Percentage:"),
                    succ_rows
                )


def calculate_optimization_stats(stats):
    attempts = stats["Optimization attempts"]
    created = stats["Optimization traces created"]
    executed = stats["Optimization traces executed"]
    uops = stats["Optimization uops executed"]
    trace_stack_overflow = stats["Optimization trace stack overflow"]
    trace_stack_underflow = stats["Optimization trace stack underflow"]
    trace_too_long = stats["Optimization trace too long"]
    inner_loop = stats["Optimization inner loop"]
    recursive_call = stats["Optimization recursive call"]

    return [
        ("Optimization attempts", attempts, ""),
        (
            "Traces created", created,
            format_ratio(created, attempts)
        ),
        ("Traces executed", executed, ""),
        ("Uops executed", uops, int(uops / executed)),
        ("Trace stack overflow", trace_stack_overflow, ""),
        ("Trace stack underflow", trace_stack_underflow, ""),
        ("Trace too long", trace_too_long, ""),
        ("Inner loop found", inner_loop, ""),
        ("Recursive call", recursive_call, "")
    ]


def calculate_uop_execution_counts(opcode_stats):
    total = 0
    counts = []
    for name, opcode_stat in opcode_stats.items():
        if "execution_count" in opcode_stat:
            count = opcode_stat['execution_count']
            counts.append((count, name))
            total += count
    counts.sort(reverse=True)
    cumulative = 0
    rows = []
    for (count, name) in counts:
        cumulative += count
        rows.append((name, count, format_ratio(count, total),
                     format_ratio(cumulative, total)))
    return rows


def emit_optimization_stats(stats):
    if "Optimization attempts" not in stats:
        return

    uop_stats = extract_opcode_stats(stats, "uop")

    with Section("Optimization (Tier 2) stats", summary="statistics about the Tier 2 optimizer"):
        with Section("Overall stats", level=3):
            rows = calculate_optimization_stats(stats)
            emit_table(("", "Count:", "Ratio:"), rows)

        emit_histogram(
            "Trace length histogram",
            stats,
            "Trace length",
            stats["Optimization traces created"]
        )
        emit_histogram(
            "Optimized trace length histogram",
            stats,
            "Optimized trace length",
            stats["Optimization traces created"]
        )
        emit_histogram(
            "Trace run length histogram",
            stats,
            "Trace run length",
            stats["Optimization traces executed"]
        )

        with Section("Uop stats", level=3):
            rows = calculate_uop_execution_counts(uop_stats)
            emit_table(
                ("Uop", "Count:", "Self:", "Cumulative:"),
                rows
            )

        with Section("Unsupported opcodes", level=3):
            unsupported_opcodes = extract_opcode_stats(stats, "unsupported_opcode")
            data = []
            for opcode, entry in unsupported_opcodes.items():
                data.append((entry["count"], opcode))
            data.sort(reverse=True)
            rows = [(x[1], x[0]) for x in data]
            emit_table(("Opcode", "Count"), rows)


def emit_comparative_optimization_stats(base_stats, head_stats):
    print("## Comparative optimization stats not implemented\n\n")


def output_single_stats(stats):
    opcode_stats = extract_opcode_stats(stats, "opcode")
    total = get_total(opcode_stats)
    emit_execution_counts(opcode_stats, total)
    emit_pair_counts(opcode_stats, total)
    emit_specialization_stats(opcode_stats, stats["_defines"])
    emit_specialization_overview(opcode_stats, total, stats["_specialized_instructions"])
    emit_call_stats(stats, stats["_stats_defines"])
    emit_object_stats(stats)
    emit_gc_stats(stats)
    emit_optimization_stats(stats)
    with Section("Meta stats", summary="Meta statistics"):
        emit_table(("", "Count:"), [('Number of data files', stats['__nfiles__'])])


def output_comparative_stats(base_stats, head_stats):
    base_opcode_stats = extract_opcode_stats(base_stats, "opcode")
    base_total = get_total(base_opcode_stats)

    head_opcode_stats = extract_opcode_stats(head_stats, "opcode")
    head_total = get_total(head_opcode_stats)

    emit_comparative_execution_counts(
        base_opcode_stats, base_total, head_opcode_stats, head_total
    )
    emit_comparative_specialization_stats(
        base_opcode_stats, head_opcode_stats, head_stats["_defines"]
    )
    emit_comparative_specialization_overview(
        base_opcode_stats, base_total, head_opcode_stats, head_total,
        head_stats["_specialized_instructions"]
    )
    emit_comparative_call_stats(base_stats, head_stats, head_stats["_stats_defines"])
    emit_comparative_object_stats(base_stats, head_stats)
    emit_comparative_gc_stats(base_stats, head_stats)
    emit_comparative_optimization_stats(base_stats, head_stats)

def output_stats(inputs, json_output=None):
    if len(inputs) == 1:
        stats = gather_stats(inputs[0])
        if json_output is not None:
            json.dump(stats, json_output)
        output_single_stats(stats)
    elif len(inputs) == 2:
        if json_output is not None:
            raise ValueError(
                "Can not output to JSON when there are multiple inputs"
            )

        base_stats = gather_stats(inputs[0])
        head_stats = gather_stats(inputs[1])
        output_comparative_stats(base_stats, head_stats)

    print("---")
    print("Stats gathered on:", date.today())

def main():
    parser = argparse.ArgumentParser(description="Summarize pystats results")

    parser.add_argument(
        "inputs",
        nargs="*",
        type=str,
        default=[DEFAULT_DIR],
        help=f"""
        Input source(s).
        For each entry, if a .json file, the output provided by --json-output from a previous run;
        if a directory, a directory containing raw pystats .txt files.
        If one source is provided, its stats are printed.
        If two sources are provided, comparative stats are printed.
        Default is {DEFAULT_DIR}.
        """
    )

    parser.add_argument(
        "--json-output",
        nargs="?",
        type=argparse.FileType("w"),
        help="Output complete raw results to the given JSON file."
    )

    args = parser.parse_args()

    if len(args.inputs) > 2:
        raise ValueError("0-2 arguments may be provided.")

    output_stats(args.inputs, json_output=args.json_output)

if __name__ == "__main__":
    main()
