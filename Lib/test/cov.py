"""A minimal hook for gathering line coverage of the standard library.

Designed to be used with -Xpresite= which means:
* it installs itself on import
* it's not imported as `__main__` so can't use the ifmain idiom
* it can't import anything besides `sys` to avoid tainting gathered coverage
* filenames are not normalized

To get gathered coverage back, look for 'test.cov' in `sys.modules`
instead of importing directly. That way you can determine if the module
was already in use.

If you need to disable the hook, call the `disable()` function.
"""

import sys

mon = sys.monitoring

FileName = str
LineNo = int
Location = tuple[FileName, LineNo]

coverage: set[Location] = set()


# `types` and `typing` aren't imported to avoid invalid coverage
def add_line(
    code: "types.CodeType",
    lineno: int,
) -> "typing.Literal[sys.monitoring.DISABLE]":
    coverage.add((code.co_filename, lineno))
    return mon.DISABLE


def enable():
    mon.use_tool_id(mon.COVERAGE_ID, "regrtest coverage")
    mon.register_callback(mon.COVERAGE_ID, mon.events.LINE, add_line)
    mon.set_events(mon.COVERAGE_ID, mon.events.LINE)


def disable():
    mon.set_events(mon.COVERAGE_ID, 0)
    mon.register_callback(mon.COVERAGE_ID, mon.events.LINE, None)
    mon.free_tool_id(mon.COVERAGE_ID)


enable()
