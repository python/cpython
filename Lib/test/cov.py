"""A minimal hook for gathering line coverage of the standard library."""

import sys

mon = sys.monitoring
mon.use_tool_id(mon.COVERAGE_ID, "regrtest coverage")

FileName = str
LineNo = int
Location = tuple[FileName, LineNo]
COVERAGE: set[Location] = set()

# `types` and `typing` not imported to avoid invalid coverage
def add_line(
    code: "types.CodeType",
    lineno: int,
) -> "typing.Literal[sys.monitoring.DISABLE]":
    COVERAGE.add((code.co_filename, lineno))
    return mon.DISABLE

mon.register_callback(mon.COVERAGE_ID, mon.events.LINE, add_line)
mon.set_events(mon.COVERAGE_ID, mon.events.LINE)
