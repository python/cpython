import argparse
import glob
import re

from dataclasses import dataclass
from datetime import datetime, date, time

# Verstrichene Zeit 00:00:00.74
msbuild_time_str = "Verstrichene Zeit"

# Total duration: 1 min 15 sec
pgo_time_str = "Total duration:"
pgo_regex = re.compile(
    r"%s\s(\d{1,2})\smin\s(\d{1,2})\ssec" % pgo_time_str)

begin_ts_str = "BeginTimeStamp"
end_ts_str = "EndTimeStamp"

# ReSTTable and MarkDownTable shamelessly taken from pyperf
class ReSTTable:
    def __init__(self, headers, rows):
        self.headers = headers
        self.rows = rows
        self.widths = [len(header) for header in self.headers]
        for row in self.rows:
            for column, cell in enumerate(row):
                self.widths[column] = max(self.widths[column], len(cell))

    def _render_line(self, char='-'):
        parts = ['']
        for width in self.widths:
            parts.append(char * (width + 2))
        parts.append('')
        return '+'.join(parts)

    def _render_row(self, row):
        parts = ['']
        for width, cell in zip(self.widths, row):
            parts.append(' %s ' % cell.ljust(width))
        parts.append('')
        return '|'.join(parts)

    def render(self, write_line):
        write_line(self._render_line('-'))
        write_line(self._render_row(self.headers))
        write_line(self._render_line('='))
        for row in self.rows:
            write_line(self._render_row(row))
            write_line(self._render_line('-'))


class MarkDownTable:
    def __init__(self, headers, rows):
        self.headers = headers
        self.rows = rows
        self.widths = [len(header) for header in self.headers]
        for row in self.rows:
            for column, cell in enumerate(row):
                self.widths[column] = max(self.widths[column], len(cell))

    def _render_line(self, char='-'):
        parts = ['']
        for idx, width in enumerate(self.widths):
            if idx == 0:
                parts.append(char * (width + 2))
            else:
                parts.append(f':{char * width}:')
        parts.append('')
        return '|'.join(parts)

    def _render_row(self, row):
        parts = ['']
        for width, cell in zip(self.widths, row):
            parts.append(" %s " % cell.ljust(width))
        parts.append('')
        return '|'.join(parts)

    def render(self, write_line):
        write_line(self._render_row(self.headers))
        write_line(self._render_line('-'))
        for row in self.rows:
            write_line(self._render_row(row))

@dataclass
class Build:
    name: str
    build_times: list
    project_times: dict


@dataclass
class PgoBuild:
    name: str
    build_times: list
    project_times_pginstrument: dict
    project_times_pgupdate: dict


def get_secs_from_pgo(t):
    m = pgo_regex.match(t)
    return float(m[1]) * 60.0 + float(m[2])


def get_secs_from_msbuild(t):
    t = t[len(msbuild_time_str):].strip()
    secs = (datetime.combine(date.min, time.fromisoformat(t))
        - datetime.min).total_seconds()
    return secs


def read_build(filepath):
    with open(build_name) as fh:
        content = fh.read()
        return pgo_time_str in content, content.splitlines()


def dump_summary(builds, args):
    if not builds:
        return
    num_builds = len(builds[0].build_times)
    is_pgo = num_builds > 1
    headers = [""]
    rows = []
    for i in range(num_builds):
        row = [i] * (len(builds) + 1)
        rows.append(row)

    if is_pgo:
        for i, n in enumerate(("pginstr", "pgo", "kill", "pgupd", "total time")):
            rows[i][0] = n
    else:
        rows[0][0] = "total time"

    for x, build in enumerate(builds):
        headers.append(build.name)
        for y, t in enumerate(build.build_times):
            rows[y][x + 1] = "%6.1f" % t

    if args.table_format == "rest":
        table = ReSTTable(headers, rows)
    else:
        table = MarkDownTable(headers, rows)
    
    if is_pgo:
        print("\nPGO builds:")
    else:
        print("\nDebug and release builds:")
    table.render(print)


def dump_details(builds, args):
    if not builds:
        return
    if hasattr(builds[0], "project_times"):
        attrs = ["project_times"]
    else:
        attrs = ["project_times_pginstrument", "project_times_pgupdate"]
    is_pgo = len(attrs) > 1
    for attr in attrs:
        proj_first_build = getattr(builds[0], attr)
        headers = [""]
        rows = []
        k = list(proj_first_build.keys())
        # dict is ordered :)
        # thus, we get here _freeze_module and python314 ...
        skip = set(k[0:2])
        # ... plus total
        skip.add(k[-1])
        cpy = {k: v for k, v in proj_first_build.items() if k not in skip}
        order = k[0:2] + list([name for name, val in sorted(
            cpy.items(), key=lambda item: item[1], reverse=True)])
        order.append(k[-1])
        for i in range(len(proj_first_build)):
            row = [i] * (len(builds) + 1)
            rows.append(row)

        for i, n in enumerate(order):
            rows[i][0] = n

        for x, build in enumerate(builds):
            headers.append(build.name)
            for y, n in enumerate(order):
                rows[y][x + 1] = "%6.1f" % getattr(build, attr)[n]

        if args.table_format == "rest":
            table = ReSTTable(headers, rows)
        else:
            table = MarkDownTable(headers, rows)
        
        if is_pgo:
            print("\nDetails %s:" % attr.replace("project_times_", ""))
        else:
            print("\nDetails:")
        table.render(print)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Display build times.')
    parser.add_argument(
        "--table-format", type=str, default="rest", choices=["rest", "md"],
        help="Format of table rendering")
    parser.add_argument(
        'filenames', metavar='build_log.txt', type=str, nargs='*',
        help='Build logs to process. Defaults to globbing build_*.txt')
    args = parser.parse_args()
    if len(args.filenames) == 0:
        filenames = sorted(glob.glob("build_*.txt"))
    elif len(args.filenames) == 1:
        filenames = sorted(glob.glob(args.filenames[0]))
    else:
        filenames = args.filenames

    builds = []
    pgo_builds = []
    proj_begin_ts = {}
    for build_name in filenames:
        is_pgo_build, lines = read_build(build_name)
        build_name = build_name.replace("build_", "")
        build_name = build_name.replace(".txt", "")
        if is_pgo_build:
            build = PgoBuild(build_name, [], {}, {})
            pgo_builds.append(build)
            project_times = build.project_times_pginstrument
        else:
            build = Build(build_name, [], {})
            builds.append(build)
            project_times = build.project_times

        for line in lines:
            line = line.strip()
            if len(build.build_times) > 1:
                project_times = build.project_times_pgupdate
            if line.startswith(msbuild_time_str):
                secs = get_secs_from_msbuild(line)
                build.build_times.append(secs)
            elif line.startswith(pgo_time_str):
                secs = get_secs_from_pgo(line)
                build.build_times.append(secs)
            elif line.startswith(begin_ts_str):
                proj, begin_ts = line[len(begin_ts_str):].strip().split(":")
                if proj.endswith("_d"):
                    proj = proj[:-2]
                proj_begin_ts[proj] = begin_ts
            elif line.startswith(end_ts_str):
                proj, end_ts = line[len(end_ts_str):].strip().split(":")
                if proj.endswith("_d"):
                    proj = proj[:-2]
                try:
                    begin_ts = proj_begin_ts.pop(proj)
                except KeyError:
                    raise Exception(
                        "end for %r but no begin" % proj)
                project_times[proj] = float(end_ts) - float(begin_ts)
        project_times["total"] = sum(project_times.values())
        if is_pgo_build:
            build.project_times_pginstrument["total"] = sum(
                build.project_times_pginstrument.values())
            build.build_times.append(sum(build.build_times))

    dump_summary(builds, args)
    dump_summary(pgo_builds, args)
    dump_details(builds, args)
    dump_details(pgo_builds, args)
