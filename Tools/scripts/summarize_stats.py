"""Print a summary of specialization stats for all files in the
default stats folders.
"""

from __future__ import annotations

# NOTE: Bytecode introspection modules (opcode, dis, etc.) should only
# be imported when loading a single dataset. When comparing datasets, it
# could get it wrong, leading to subtle errors.

import argparse
import collections
from datetime import date
import itertools
import json
import os
from pathlib import Path
import re
import sys
from typing import TextIO, TypeAlias


OpcodeStats: TypeAlias = dict[str, dict[str, int]]
PairCounts: TypeAlias = list[tuple[int, tuple[str, str]]]
Defines: TypeAlias = dict[int, list[str]]
Rows: TypeAlias = list[tuple]
Columns: TypeAlias = tuple[str, ...]


if os.name == "nt":
    DEFAULT_DIR = "c:\\temp\\py_stats\\"
else:
    DEFAULT_DIR = "/tmp/py_stats/"


SOURCE_DIR = Path(__file__).parents[2]


TOTAL = "specialization.hit", "specialization.miss", "execution_count"


def pretty(name: str) -> str:
    return name.replace("_", " ").lower()


class Stats(dict):
    def __init__(self, input: Path):
        super().__init__()

        if input.is_file():
            with open(input, "r") as fd:
                self.update(json.load(fd))

            self["_stats_defines"] = {
                int(k): v for k, v in self["_stats_defines"].items()
            }
            self["_defines"] = {int(k): v for k, v in self["_defines"].items()}

        elif input.is_dir():
            stats: collections.Counter = collections.Counter()
            for filename in input.iterdir():
                with open(filename) as fd:
                    for line in fd:
                        try:
                            key, value = line.split(":")
                        except ValueError:
                            print(
                                f"Unparsable line: '{line.strip()}' in {filename}",
                                file=sys.stderr,
                            )
                            continue
                        stats[key.strip()] += int(value)
                stats["__nfiles__"] += 1

            self.update(stats)

            self._load_metadata_from_source()

        else:
            raise ValueError(f"{input:r} is not a file or directory path")

        self._opcode_stats: dict[str, OpcodeStats] = {}

    def _load_metadata_from_source(self):
        def get_defines(filepath: Path, prefix: str = "SPEC_FAIL") -> Defines:
            with open(SOURCE_DIR / filepath) as spec_src:
                defines = collections.defaultdict(list)
                start = "#define " + prefix + "_"
                for line in spec_src:
                    line = line.strip()
                    if not line.startswith(start):
                        continue
                    line = line[len(start) :]
                    name, val = line.split()
                    defines[int(val.strip())].append(name.strip())
            return defines

        import opcode

        self["_specialized_instructions"] = [
            op
            for op in opcode._specialized_opmap.keys()  # type: ignore
            if "__" not in op
        ]
        self["_stats_defines"] = get_defines(
            Path("Include") / "cpython" / "pystats.h", "EVAL_CALL"
        )
        self["_defines"] = get_defines(Path("Python") / "specialize.c")

    @property
    def defines(self) -> Defines:
        return self["_defines"]

    @property
    def pystats_defines(self) -> Defines:
        return self["_stats_defines"]

    @property
    def specialized_instructions(self) -> list[str]:
        return self["_specialized_instructions"]

    def get_opcode_stats(self, prefix: str) -> OpcodeStats:
        if prefix in self._opcode_stats:
            return self._opcode_stats[prefix]

        opcode_stats: OpcodeStats = collections.defaultdict(dict)
        for key, value in self.items():
            if not key.startswith(prefix):
                continue
            name, _, rest = key[len(prefix) + 1 :].partition("]")
            opcode_stats[name][rest.strip(".")] = value

        self._opcode_stats[prefix] = opcode_stats
        return opcode_stats

    def get_pair_counts(self, prefix: str) -> PairCounts:
        opcode_stats = self.get_opcode_stats(prefix)
        pair_counts: PairCounts = []
        for name_i, opcode_stat in opcode_stats.items():
            for key, value in opcode_stat.items():
                if key.startswith("pair_count"):
                    name_j, _, _ = key[len("pair_count") + 1 :].partition("]")
                    if value:
                        pair_counts.append((value, (name_i, name_j)))
        pair_counts.sort(reverse=True)
        return pair_counts

    def get_total(self, prefix: str) -> int:
        return sum(
            x.get("execution_count", 0) for x in self.get_opcode_stats(prefix).values()
        )


class Count(int):
    def markdown(self) -> str:
        return format(self, ",d")


class CountPer:
    def __init__(self, num: int, den: int):
        self.num = num
        self.den = den
        if den == 0 and num != 0:
            raise ValueError("Invalid denominator")

    def markdown(self) -> str:
        if self.den == 0:
            return "0"
        else:
            return f"{int(self.num / self.den):,d}"


class Ratio:
    def __init__(self, num: int, den: int):
        self.num = num
        self.den = den

    def __float__(self):
        if self.den == 0:
            return 0.0
        else:
            return self.num / self.den

    def markdown(self) -> str:
        if self.den == 0:
            return ""
        else:
            return f"{self.num / self.den:,.01%}"


class DiffRatio(Ratio):
    def __init__(self, base: int | str, head: int | str):
        if isinstance(base, str) or isinstance(head, str):
            super().__init__(0, 0)
        else:
            super().__init__(head - base, base)


def sort_by_last_column(rows: Rows):
    rows.sort(key=lambda row: abs(float(row[-1])), reverse=True)


def dont_sort(rows: Rows):
    pass


class Table:
    columns: Columns
    sort_by_last_column: bool = False

    def calculate_rows(self, stats: Stats) -> Rows:
        raise NotImplementedError()

    def join_row(self, key: str, row_a: tuple, row_b: tuple) -> tuple:
        return (key, *row_a, *row_b)

    def join_columns(self, columns: Columns) -> Columns:
        return (
            columns[0],
            *("Base " + x for x in columns[1:]),
            *("Head " + x for x in columns[1:]),
        )

    sort_joined_rows = staticmethod(dont_sort)

    def join_tables(self, rows_a: Rows, rows_b: Rows) -> tuple[Columns, Rows]:
        ncols = len(self.columns)

        default = ("",) * (ncols - 1)
        data_a = {x[0]: x[1:] for x in rows_a}
        data_b = {x[0]: x[1:] for x in rows_b}

        if len(data_a) != len(rows_a) or len(data_b) != len(rows_b):
            raise ValueError("Duplicate keys")

        # To preserve ordering, use A's keys as is and then add any in B that
        # aren't in A
        keys = list(data_a.keys()) + [k for k in data_b.keys() if k not in data_a]
        rows = [
            self.join_row(k, data_a.get(k, default), data_b.get(k, default))
            for k in keys
        ]
        if self.sort_by_last_column:
            rows.sort(key=lambda row: abs(float(row[-1])), reverse=True)

        columns = self.join_columns(self.columns)
        return columns, rows

    def get_rows(
        self, base_stats: Stats, head_stats: Stats | None = None
    ) -> tuple[Columns, Rows]:
        if head_stats is None:
            rows = self.calculate_rows(base_stats)
            return self.columns, rows
        else:
            rows_a = self.calculate_rows(base_stats)
            rows_b = self.calculate_rows(head_stats)
            cols, rows = self.join_tables(rows_a, rows_b)
            return cols, rows

    def output_markdown(
        self,
        out: TextIO,
        base_stats: Stats,
        head_stats: Stats | None = None,
        level: int = 2,
    ) -> None:
        header, rows = self.get_rows(base_stats, head_stats)
        if len(rows) == 0:
            return

        def to_markdown(x):
            if hasattr(x, "markdown"):
                return x.markdown()
            elif isinstance(x, str):
                return x
            elif x is None:
                return ""
            else:
                raise TypeError(f"Can't convert {x} to markdown")

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
        print(header_line, file=out)
        print(under_line, file=out)
        for row in rows:
            if width is not None and len(row) != width:
                raise ValueError("Wrong number of elements in row '" + str(row) + "'")
            print("|", " | ".join(to_markdown(i) for i in row), "|", file=out)
        print(file=out)


class FixedTable(Table):
    def __init__(self, columns: Columns, rows: Rows):
        self.columns = columns
        self.rows = rows

    def get_rows(self, *args) -> tuple[Columns, Rows]:
        return self.columns, self.rows


class SimpleChangeTable(Table):
    """
    Base class of tables where the comparison table has an extra column "Change"
    computed from the change of the second column of the base and head.  Sorted
    by the "Change" column.
    """

    def join_row(self, key: str, base_data: tuple, head_data: tuple) -> tuple:
        return (key, *base_data, *head_data, DiffRatio(base_data[0], head_data[0]))

    def join_columns(self, columns: Columns) -> Columns:
        return super().join_columns(columns) + ("Change:",)

    sort_joined_rows = staticmethod(sort_by_last_column)


class ExecutionCountTable(Table):
    columns = ("Name", "Count:", "Self:", "Cumulative:", "Miss ratio:")

    def __init__(self, prefix: str):
        self.prefix = prefix

    def join_row(self, key: str, row_a: tuple, row_b: tuple) -> tuple:
        return (key, row_a[0], row_b[0], DiffRatio(row_a[0], row_b[0]))

    def join_columns(self, columns: Columns) -> Columns:
        return ("Name", "Base Count:", "Head Count:", "Change")

    sort_joined_rows = staticmethod(sort_by_last_column)

    def calculate_rows(self, stats: Stats) -> Rows:
        opcode_stats = stats.get_opcode_stats(self.prefix)
        total = 0
        counts = []
        for name, opcode_stat in opcode_stats.items():
            if "execution_count" in opcode_stat:
                count = opcode_stat["execution_count"]
                total += count
                miss = 0
                if "specializable" not in opcode_stat:
                    miss = opcode_stat.get("specialization.miss", 0)
                counts.append((count, name, miss))
        counts.sort(reverse=True)
        cumulative = 0
        rows: Rows = []
        for count, name, miss in counts:
            cumulative += count
            if miss:
                miss_val = Ratio(miss, count)
            else:
                miss_val = None
            rows.append(
                (
                    name,
                    Count(count),
                    Ratio(count, total),
                    Ratio(cumulative, total),
                    miss_val,
                )
            )
        return rows


class PairCountTable(Table):
    columns = ("Pair", "Count:", "Self:", "Cumulative:")

    def calculate_rows(self, stats: Stats) -> Rows:
        pair_counts = stats.get_pair_counts("opcode")
        total = stats.get_total("opcode")

        cumulative = 0
        rows: Rows = []
        for count, (name_i, name_j) in itertools.islice(pair_counts, 100):
            cumulative += count
            rows.append(
                (
                    f"{name_i} {name_j}",
                    Count(count),
                    Ratio(count, total),
                    Ratio(cumulative, total),
                )
            )

        return rows


def iter_pre_succ_pairs_tables(base_stats: Stats, head_stats: Stats | None = None):
    assert head_stats is None

    opcode_stats = base_stats.get_opcode_stats("opcode")
    pair_counts = base_stats.get_pair_counts("opcode")

    predecessors: dict[str, collections.Counter] = collections.defaultdict(
        collections.Counter
    )
    successors: dict[str, collections.Counter] = collections.defaultdict(
        collections.Counter
    )
    total_predecessors: collections.Counter = collections.Counter()
    total_successors: collections.Counter = collections.Counter()
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
        pred_rows: Rows = []
        succ_rows: Rows = []
        if total1:
            pred_rows = [
                (pred, Count(count), Ratio(count, total1))
                for (pred, count) in predecessors[name].most_common(5)
            ]
        if total2:
            succ_rows = [
                (succ, Count(count), Ratio(count, total2))
                for (succ, count) in successors[name].most_common(5)
            ]

        yield Section(
            name,
            f"Successors and predecessors for {name}",
            [
                FixedTable(("Predecessors", "Count:", "Percentage:"), pred_rows),
                FixedTable(("Successors", "Count:", "Percentage:"), succ_rows),
            ],
        )


def iter_specialization_tables(base_stats: Stats, head_stats: Stats | None = None):
    class SpecializationTable(SimpleChangeTable):
        columns = ("Kind", "Count:", "Ratio:")

        def __init__(self, name: str):
            self.name = name

        sort_joined_rows = staticmethod(dont_sort)

        def calculate_rows(self, stats: Stats) -> Rows:
            opcode_stats = stats.get_opcode_stats("opcode")
            family_stats = opcode_stats[self.name]
            total = sum(family_stats.get(kind, 0) for kind in TOTAL)
            if total == 0:
                return []
            rows: Rows = []
            for key in sorted(family_stats):
                if key.startswith("specialization.failure_kinds"):
                    continue
                elif key in ("specialization.hit", "specialization.miss"):
                    label = key[len("specialization.") :]
                elif key in (
                    "execution_count",
                    "specialization.success",
                    "specialization.failure",
                    "specializable",
                ) or key.startswith("pair"):
                    continue
                else:
                    label = key
                rows.append(
                    (
                        f"{label:>12}",
                        Count(family_stats[key]),
                        Ratio(family_stats[key], total),
                    )
                )
            return rows

    class SpecializationSuccessFailureTable(SimpleChangeTable):
        columns = ("", "Count:", "Ratio:")

        def __init__(self, name: str):
            self.name = name

        sort_joined_rows = staticmethod(dont_sort)

        def calculate_rows(self, stats: Stats) -> Rows:
            opcode_stats = stats.get_opcode_stats("opcode")
            family_stats = opcode_stats[self.name]
            total_attempts = 0
            for key in ("specialization.success", "specialization.failure"):
                total_attempts += family_stats.get(key, 0)
            rows: Rows = []
            if total_attempts:
                for key in ("specialization.success", "specialization.failure"):
                    label = key[len("specialization.") :]
                    label = label[0].upper() + label[1:]
                    val = family_stats.get(key, 0)
                    rows.append((label, Count(val), Ratio(val, total_attempts)))
            return rows

    class SpecializationFailureKindTable(SimpleChangeTable):
        columns = ("Failure kind", "Count:", "Ratio:")

        def __init__(self, name: str):
            self.name = name

        def calculate_rows(self, stats: Stats) -> Rows:
            def kind_to_text(kind: int, defines: Defines, opname: str):
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
                        return pretty(name[len(opname) + 1 :])
                return "kind " + str(kind)

            defines = stats.defines
            opcode_stats = stats.get_opcode_stats("opcode")
            family_stats = opcode_stats[self.name]
            total_failures = family_stats.get("specialization.failure", 0)
            failure_kinds = [0] * 40
            for key in family_stats:
                if not key.startswith("specialization.failure_kind"):
                    continue
                index = int(key[:-1].split("[")[1])
                failure_kinds[index] = family_stats[key]
            failures = [(value, index) for (index, value) in enumerate(failure_kinds)]
            failures.sort(reverse=True)
            rows: Rows = []
            for value, index in failures:
                if not value:
                    continue
                rows.append(
                    (
                        kind_to_text(index, defines, self.name),
                        Count(value),
                        Ratio(value, total_failures),
                    )
                )
            return rows

    opcode_base_stats = base_stats.get_opcode_stats("opcode")
    names = opcode_base_stats.keys()
    if head_stats is not None:
        opcode_head_stats = head_stats.get_opcode_stats("opcode")
        names &= opcode_head_stats.keys()  # type: ignore
    else:
        opcode_head_stats = {}

    for name in sorted(names):
        if "specializable" not in opcode_base_stats.get(name, {}):
            continue
        total = sum(
            stats.get(name, {}).get(kind, 0)
            for kind in TOTAL
            for stats in (opcode_base_stats, opcode_head_stats)
        )
        if total == 0:
            continue
        yield Section(
            name,
            f"specialization stats for {name} family",
            [
                SpecializationTable(name),
                SpecializationSuccessFailureTable(name),
                SpecializationFailureKindTable(name),
            ],
        )


def iter_specialization_effectiveness_tables(
    base_stats: Stats, head_stats: Stats | None = None
):
    class SpecializationEffectivenessTable(SimpleChangeTable):
        columns = ("Instructions", "Count:", "Ratio:")

        sort_joined_rows = staticmethod(dont_sort)

        def calculate_rows(self, stats: Stats) -> Rows:
            opcode_stats = stats.get_opcode_stats("opcode")
            total = stats.get_total("opcode")
            specialized_instructions = stats.specialized_instructions

            basic = 0
            specialized = 0
            not_specialized = 0
            for name, opcode_stat in opcode_stats.items():
                if "execution_count" not in opcode_stat:
                    continue
                count = opcode_stat["execution_count"]
                if "specializable" in opcode_stat:
                    not_specialized += count
                elif name in specialized_instructions:
                    miss = opcode_stat.get("specialization.miss", 0)
                    not_specialized += miss
                    specialized += count - miss
                else:
                    basic += count

            return [
                ("Basic", Count(basic), Ratio(basic, total)),
                (
                    "Not specialized",
                    Count(not_specialized),
                    Ratio(not_specialized, total),
                ),
                ("Specialized", Count(specialized), Ratio(specialized, total)),
            ]

    class DeferredByInstructionTable(SimpleChangeTable):
        columns = ("Name", "Count:", "Ratio:")

        def calculate_rows(self, stats: Stats) -> Rows:
            opcode_stats = stats.get_opcode_stats("opcode")

            total = 0
            counts = []
            for name, opcode_stat in opcode_stats.items():
                value = opcode_stat.get("specialization.deferred", 0)
                counts.append((value, name))
                total += value
            counts.sort(reverse=True)
            if total:
                return [
                    (name, Count(count), Ratio(count, total))
                    for (count, name) in counts[:10]
                ]
            else:
                return []

    class MissesByInstructionTable(SimpleChangeTable):
        columns = ("Name", "Count:", "Ratio:")

        def calculate_rows(self, stats: Stats) -> Rows:
            opcode_stats = stats.get_opcode_stats("opcode")

            total = 0
            counts = []
            for name, opcode_stat in opcode_stats.items():
                # Avoid double counting misses
                if "specializable" in opcode_stat:
                    continue
                value = opcode_stat.get("specialization.misses", 0)
                counts.append((value, name))
                total += value
            counts.sort(reverse=True)
            if total:
                return [
                    (name, Count(count), Ratio(count, total))
                    for (count, name) in counts[:10]
                ]
            else:
                return []

    yield SpecializationEffectivenessTable()
    yield Section("Deferred by instruction", "", [DeferredByInstructionTable()])
    yield Section("Misses by instruction", "", [MissesByInstructionTable()])


class CallStatsTable(SimpleChangeTable):
    columns = ("", "Count:", "Ratio:")

    sort_joined_rows = staticmethod(dont_sort)

    def calculate_rows(self, stats: Stats) -> Rows:
        defines = stats.pystats_defines

        total = 0
        for key, value in stats.items():
            if "Calls to" in key:
                total += value

        rows: Rows = []
        for key, value in sorted(stats.items()):
            if "Calls to" in key:
                rows.append((key, Count(value), Ratio(value, total)))
            elif key.startswith("Calls "):
                name, index = key[:-1].split("[")
                index = int(index)
                label = f"{name} ({pretty(defines[index][0])})"
                rows.append((label, Count(value), Ratio(value, total)))

        for key, value in sorted(stats.items()):
            if key.startswith("Frame"):
                rows.append((key, Count(value), Ratio(value, total)))

        return rows


class ObjectStatsTable(SimpleChangeTable):
    columns = ("", "Count:", "Ratio:")

    def calculate_rows(self, stats: Stats) -> Rows:
        total_materializations = stats.get("Object new values", 0)
        total_allocations = stats.get("Object allocations", 0) + stats.get(
            "Object allocations from freelist", 0
        )
        total_increfs = stats.get("Object interpreter increfs", 0) + stats.get(
            "Object increfs", 0
        )
        total_decrefs = stats.get("Object interpreter decrefs", 0) + stats.get(
            "Object decrefs", 0
        )
        rows: Rows = []
        for key, value in stats.items():
            if key.startswith("Object"):
                if "materialize" in key:
                    ratio = Ratio(value, total_materializations)
                elif "allocations" in key:
                    ratio = Ratio(value, total_allocations)
                elif "increfs" in key:
                    ratio = Ratio(value, total_increfs)
                elif "decrefs" in key:
                    ratio = Ratio(value, total_decrefs)
                else:
                    ratio = None
                label = key[6:].strip()
                label = label[0].upper() + label[1:]
                rows.append((label, Count(value), ratio))
        return rows


class GCStatsTable(Table):
    columns = ("Generation:", "Collections:", "Objects collected:", "Object visits:")

    def calculate_rows(self, stats: Stats) -> Rows:
        gc_stats: list[dict[str, int]] = []
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
            (
                Count(i),
                Count(gen["collections"]),
                Count(gen["objects collected"]),
                Count(gen["object visits"]),
            )
            for (i, gen) in enumerate(gc_stats)
        ]


def iter_optimization_tables(base_stats: Stats, head_stats: Stats | None = None):
    class OptimizationStatsTable(SimpleChangeTable):
        columns = ("", "Count:", "Ratio:")

        sort_joined_rows = staticmethod(dont_sort)

        def calculate_rows(self, stats: Stats) -> Rows:
            if "Optimization attempts" not in stats:
                return []

            attempts = stats["Optimization attempts"]
            created = stats["Optimization traces created"]
            executed = stats["Optimization traces executed"]
            uops = stats["Optimization uops executed"]
            trace_stack_overflow = stats["Optimization trace stack overflow"]
            trace_stack_underflow = stats["Optimization trace stack underflow"]
            trace_too_long = stats["Optimization trace too long"]
            trace_too_short = stats["Optimization trace too short"]
            inner_loop = stats["Optimization inner loop"]
            recursive_call = stats["Optimization recursive call"]

            return [
                ("Optimization attempts", Count(attempts), ""),
                ("Traces created", Count(created), Ratio(created, attempts)),
                ("Traces executed", Count(executed), ""),
                ("Uops executed", Count(uops), CountPer(uops, executed)),
                (
                    "Trace stack overflow",
                    Count(trace_stack_overflow),
                    Ratio(trace_stack_overflow, created),
                ),
                (
                    "Trace stack underflow",
                    Count(trace_stack_underflow),
                    Ratio(trace_stack_underflow, created),
                ),
                (
                    "Trace too long",
                    Count(trace_too_long),
                    Ratio(trace_too_long, created),
                ),
                (
                    "Trace too short",
                    Count(trace_too_short),
                    Ratio(trace_too_short, created),
                )("Inner loop found", Count(inner_loop), Ratio(inner_loop, created)),
                (
                    "Recursive call",
                    Count(recursive_call),
                    Ratio(recursive_call, created),
                ),
            ]

    class HistogramTable(SimpleChangeTable):
        columns = ("Range", "Count:", "Ratio:")

        def __init__(self, key: str, den: str):
            self.key = key
            self.den = den

        sort_joined_rows = staticmethod(dont_sort)

        def calculate_rows(self, stats: Stats) -> Rows:
            rows: Rows = []
            last_non_zero = 0
            for k, v in stats.items():
                if k.startswith(self.key):
                    match = re.match(r".+\[([0-9]+)\]", k)
                    if match is not None:
                        entry = int(match.groups()[0])
                        if v != 0:
                            last_non_zero = len(rows)
                        rows.append(
                            (f"<= {entry:,d}", Count(v), Ratio(int(v), stats[self.den]))
                        )
            # Don't include any zero entries at the end
            rows = rows[: last_non_zero + 1]
            return rows

    class UnsupportedOpcodesTable(SimpleChangeTable):
        columns = ("Opcode", "Count:")

        def calculate_rows(self, stats: Stats) -> Rows:
            unsupported_opcodes = stats.get_opcode_stats("unsupported_opcode")
            data = []
            for opcode, entry in unsupported_opcodes.items():
                data.append((Count(entry["count"]), opcode))
            data.sort(reverse=True)
            return [(x[1], x[0]) for x in data]

    if "Optimization attempts" not in base_stats or (
        head_stats is not None and "Optimization attempts" not in head_stats
    ):
        return

    yield OptimizationStatsTable()

    for name, den in [
        ("Trace length", "Optimization traces created"),
        ("Optimized trace length", "Optimization traces created"),
        ("Trace run length", "Optimization traces executed"),
    ]:
        yield Section(f"{name} histogram", "", [HistogramTable(name, den)])

    yield Section("Uop stats", "", [ExecutionCountTable("uops")])
    yield Section("Unsupported opcodes", "", [UnsupportedOpcodesTable()])


class MetaStatsTable(Table):
    columns = ("", "Count:")

    def calculate_rows(self, stats: Stats) -> Rows:
        return [("Number of data files", Count(stats.get("__nfiles__", 0)))]


class Section:
    def __init__(
        self,
        title: str,
        summary: str = "",
        part_iter=None,
        comparative: bool = True,
    ):
        self.title = title
        if not summary:
            self.summary = title.lower()
        else:
            self.summary = summary
        if part_iter is None:
            part_iter = []
        if isinstance(part_iter, list):

            def iter_parts(base_stats: Stats, head_stats: Stats | None):
                yield from part_iter

            self.part_iter = iter_parts
        else:
            self.part_iter = part_iter
        self.comparative = comparative

    def output_markdown(
        self,
        out: TextIO,
        base_stats: Stats,
        head_stats: Stats | None = None,
        level: int = 1,
    ) -> None:
        if self.title:
            print("#" * level, self.title, file=out)
            print(file=out)
            print("<details>", file=out)
            print("<summary>", self.summary, "</summary>", file=out)
            print(file=out)
        if head_stats is not None and self.comparative is False:
            print("Not included in comparative output.\n")
        else:
            for part in self.part_iter(base_stats, head_stats):
                part.output_markdown(out, base_stats, head_stats, level=level + 1)
        print(file=out)
        if self.title:
            print("</details>", file=out)
            print(file=out)


LAYOUT = Section(
    "",
    "",
    [
        Section(
            "Execution counts",
            "execution counts for all instructions",
            [ExecutionCountTable("opcode")],
        ),
        Section(
            "Pair counts",
            "Pair counts for top 100 pairs",
            [PairCountTable()],
            comparative=False,
        ),
        Section(
            "Predecessor/Successor Pairs",
            "Top 5 predecessors and successors of each opcode",
            iter_pre_succ_pairs_tables,
            comparative=False,
        ),
        Section(
            "Specialization stats",
            "specialization stats by family",
            iter_specialization_tables,
        ),
        Section(
            "Specialization effectiveness",
            "",
            iter_specialization_effectiveness_tables,
        ),
        Section("Call stats", "Inlined calls and frame stats", [CallStatsTable()]),
        Section(
            "Object stats",
            "allocations, frees and dict materializatons",
            [ObjectStatsTable()],
        ),
        Section("GC stats", "GC collections and effectiveness", [GCStatsTable()]),
        Section(
            "Optimization (Tier 2) stats",
            "statistics about the Tier 2 optimizer",
            iter_optimization_tables,
        ),
        Section("Meta stats", "Meta statistics", [MetaStatsTable()]),
    ],
)


def output_markdown(out: TextIO, base_stats: Stats, head_stats: Stats | None = None):
    LAYOUT.output_markdown(out, base_stats, head_stats)
    print("---", file=out)
    print("Stats gathered on:", date.today(), file=out)


def output_stats(inputs: list[Path], json_output=None):
    if len(inputs) == 1:
        stats = Stats(Path(inputs[0]))
        if json_output is not None:
            json.dump(stats, json_output)
        output_markdown(sys.stdout, stats)
    elif len(inputs) == 2:
        if json_output is not None:
            raise ValueError("Can not output to JSON when there are multiple inputs")

        base_stats = Stats(Path(inputs[0]))
        head_stats = Stats(Path(inputs[1]))
        output_markdown(sys.stdout, base_stats, head_stats)


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
        """,
    )

    parser.add_argument(
        "--json-output",
        nargs="?",
        type=argparse.FileType("w"),
        help="Output complete raw results to the given JSON file.",
    )

    args = parser.parse_args()

    if len(args.inputs) > 2:
        raise ValueError("0-2 arguments may be provided.")

    output_stats(args.inputs, json_output=args.json_output)


if __name__ == "__main__":
    main()
