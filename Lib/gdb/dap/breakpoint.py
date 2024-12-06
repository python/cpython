# Copyright 2022-2024 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import re
from contextlib import contextmanager

# These are deprecated in 3.9, but required in older versions.
from typing import Optional, Sequence

import gdb

from .server import capability, request, send_event
from .sources import make_source
from .startup import DAPException, LogLevel, in_gdb_thread, log_stack, parse_and_eval
from .typecheck import type_check

# True when suppressing new breakpoint events.
_suppress_bp = False


@contextmanager
def suppress_new_breakpoint_event():
    """Return a new context manager that suppresses new breakpoint events."""
    global _suppress_bp
    saved = _suppress_bp
    _suppress_bp = True
    try:
        yield None
    finally:
        _suppress_bp = saved


@in_gdb_thread
def _bp_modified(event):
    global _suppress_bp
    if not _suppress_bp:
        send_event(
            "breakpoint",
            {
                "reason": "changed",
                "breakpoint": _breakpoint_descriptor(event),
            },
        )


@in_gdb_thread
def _bp_created(event):
    global _suppress_bp
    if not _suppress_bp:
        send_event(
            "breakpoint",
            {
                "reason": "new",
                "breakpoint": _breakpoint_descriptor(event),
            },
        )


@in_gdb_thread
def _bp_deleted(event):
    global _suppress_bp
    if not _suppress_bp:
        send_event(
            "breakpoint",
            {
                "reason": "removed",
                "breakpoint": _breakpoint_descriptor(event),
            },
        )


gdb.events.breakpoint_created.connect(_bp_created)
gdb.events.breakpoint_modified.connect(_bp_modified)
gdb.events.breakpoint_deleted.connect(_bp_deleted)


# Map from the breakpoint "kind" (like "function") to a second map, of
# breakpoints of that type.  The second map uses the breakpoint spec
# as a key, and the gdb.Breakpoint itself as a value.  This is used to
# implement the clearing behavior specified by the protocol, while
# allowing for reuse when a breakpoint can be kept.
breakpoint_map = {}


@in_gdb_thread
def _breakpoint_descriptor(bp):
    "Return the Breakpoint object descriptor given a gdb Breakpoint."
    result = {
        "id": bp.number,
        "verified": not bp.pending,
    }
    if bp.pending:
        result["reason"] = "pending"
    if bp.locations:
        # Just choose the first location, because DAP doesn't allow
        # multiple locations.  See
        # https://github.com/microsoft/debug-adapter-protocol/issues/13
        loc = bp.locations[0]
        if loc.source:
            (filename, line) = loc.source
            if loc.fullname is not None:
                filename = loc.fullname

            result.update(
                {
                    "source": make_source(filename),
                    "line": line,
                }
            )

        if loc.address:
            result["instructionReference"] = hex(loc.address)

    return result


# Extract entries from a hash table and return a list of them.  Each
# entry is a string.  If a key of that name appears in the hash table,
# it is removed and pushed on the result list; if it does not appear,
# None is pushed on the list.
def _remove_entries(table, *names):
    return [table.pop(name, None) for name in names]


# Helper function to set some breakpoints according to a list of
# specifications and a callback function to do the work of creating
# the breakpoint.
@in_gdb_thread
def _set_breakpoints_callback(kind, specs, creator):
    global breakpoint_map
    # Try to reuse existing breakpoints if possible.
    if kind in breakpoint_map:
        saved_map = breakpoint_map[kind]
    else:
        saved_map = {}
    breakpoint_map[kind] = {}
    result = []
    with suppress_new_breakpoint_event():
        for spec in specs:
            # It makes sense to reuse a breakpoint even if the condition
            # or ignore count differs, so remove these entries from the
            # spec first.
            (condition, hit_condition) = _remove_entries(
                spec, "condition", "hitCondition"
            )
            keyspec = frozenset(spec.items())

            # Create or reuse a breakpoint.  If asked, set the condition
            # or the ignore count.  Catch errors coming from gdb and
            # report these as an "unverified" breakpoint.
            bp = None
            try:
                if keyspec in saved_map:
                    bp = saved_map.pop(keyspec)
                else:
                    bp = creator(**spec)

                bp.condition = condition
                if hit_condition is None:
                    bp.ignore_count = 0
                else:
                    bp.ignore_count = int(
                        parse_and_eval(hit_condition, global_context=True)
                    )

                # Reaching this spot means success.
                breakpoint_map[kind][keyspec] = bp
                result.append(_breakpoint_descriptor(bp))
            # Exceptions other than gdb.error are possible here.
            except Exception as e:
                # Don't normally want to see this, as it interferes with
                # the test suite.
                log_stack(LogLevel.FULL)
                # Maybe the breakpoint was made but setting an attribute
                # failed.  We still want this to fail.
                if bp is not None:
                    bp.delete()
                # Breakpoint creation failed.
                result.append(
                    {
                        "verified": False,
                        "reason": "failed",
                        "message": str(e),
                    }
                )

    # Delete any breakpoints that were not reused.
    for entry in saved_map.values():
        entry.delete()
    return result


class _PrintBreakpoint(gdb.Breakpoint):
    def __init__(self, logMessage, **args):
        super().__init__(**args)
        # Split the message up for easier processing.
        self.message = re.split("{(.*?)}", logMessage)

    def stop(self):
        output = ""
        for idx, item in enumerate(self.message):
            if idx % 2 == 0:
                # Even indices are plain text.
                output += item
            else:
                # Odd indices are expressions to substitute.  The {}
                # have already been stripped by the placement of the
                # regex capture in the 'split' call.
                try:
                    # No real need to use the DAP parse_and_eval here.
                    val = gdb.parse_and_eval(item)
                    output += str(val)
                except Exception as e:
                    output += "<" + str(e) + ">"
        send_event(
            "output",
            {
                "category": "console",
                "output": output,
            },
        )
        # Do not stop.
        return False


# Set a single breakpoint or a log point.  Returns the new breakpoint.
# Note that not every spec will pass logMessage, so here we use a
# default.
@in_gdb_thread
def _set_one_breakpoint(*, logMessage=None, **args):
    if logMessage is not None:
        return _PrintBreakpoint(logMessage, **args)
    else:
        return gdb.Breakpoint(**args)


# Helper function to set ordinary breakpoints according to a list of
# specifications.
@in_gdb_thread
def _set_breakpoints(kind, specs):
    return _set_breakpoints_callback(kind, specs, _set_one_breakpoint)


# A helper function that rewrites a SourceBreakpoint into the internal
# form passed to the creator.  This function also allows for
# type-checking of each SourceBreakpoint.
@type_check
def _rewrite_src_breakpoint(
    *,
    # This is a Source but we don't type-check it.
    source,
    line: int,
    condition: Optional[str] = None,
    hitCondition: Optional[str] = None,
    logMessage: Optional[str] = None,
    **args,
):
    return {
        "source": source["path"],
        "line": line,
        "condition": condition,
        "hitCondition": hitCondition,
        "logMessage": logMessage,
    }


@request("setBreakpoints")
@capability("supportsHitConditionalBreakpoints")
@capability("supportsConditionalBreakpoints")
@capability("supportsLogPoints")
def set_breakpoint(*, source, breakpoints: Sequence = (), **args):
    if "path" not in source:
        result = []
    else:
        # Setting 'source' in BP avoids any Python error if BP already
        # has a 'source' parameter.  Setting this isn't in the spec,
        # but it is better to be safe.  See PR dap/30820.
        specs = []
        for bp in breakpoints:
            bp["source"] = source
            specs.append(_rewrite_src_breakpoint(**bp))
        # Be sure to include the path in the key, so that we only
        # clear out breakpoints coming from this same source.
        key = "source:" + source["path"]
        result = _set_breakpoints(key, specs)
    return {
        "breakpoints": result,
    }


# A helper function that rewrites a FunctionBreakpoint into the
# internal form passed to the creator.  This function also allows for
# type-checking of each FunctionBreakpoint.
@type_check
def _rewrite_fn_breakpoint(
    *,
    name: str,
    condition: Optional[str] = None,
    hitCondition: Optional[str] = None,
    **args,
):
    return {
        "function": name,
        "condition": condition,
        "hitCondition": hitCondition,
    }


@request("setFunctionBreakpoints")
@capability("supportsFunctionBreakpoints")
def set_fn_breakpoint(*, breakpoints: Sequence, **args):
    specs = [_rewrite_fn_breakpoint(**bp) for bp in breakpoints]
    return {
        "breakpoints": _set_breakpoints("function", specs),
    }


# A helper function that rewrites an InstructionBreakpoint into the
# internal form passed to the creator.  This function also allows for
# type-checking of each InstructionBreakpoint.
@type_check
def _rewrite_insn_breakpoint(
    *,
    instructionReference: str,
    offset: Optional[int] = None,
    condition: Optional[str] = None,
    hitCondition: Optional[str] = None,
    **args,
):
    # There's no way to set an explicit address breakpoint from
    # Python, so we rely on "spec" instead.
    val = "*" + instructionReference
    if offset is not None:
        val = val + " + " + str(offset)
    return {
        "spec": val,
        "condition": condition,
        "hitCondition": hitCondition,
    }


@request("setInstructionBreakpoints")
@capability("supportsInstructionBreakpoints")
def set_insn_breakpoints(
    *, breakpoints: Sequence, offset: Optional[int] = None, **args
):
    specs = [_rewrite_insn_breakpoint(**bp) for bp in breakpoints]
    return {
        "breakpoints": _set_breakpoints("instruction", specs),
    }


@in_gdb_thread
def _catch_exception(filterId, **args):
    if filterId in ("assert", "exception", "throw", "rethrow", "catch"):
        cmd = "-catch-" + filterId
    else:
        raise DAPException("Invalid exception filterID: " + str(filterId))
    result = gdb.execute_mi(cmd)
    # A little lame that there's no more direct way.
    for bp in gdb.breakpoints():
        if bp.number == result["bkptno"]:
            return bp
    # Not a DAPException because this is definitely unexpected.
    raise Exception("Could not find catchpoint after creating")


@in_gdb_thread
def _set_exception_catchpoints(filter_options):
    return _set_breakpoints_callback("exception", filter_options, _catch_exception)


# A helper function that rewrites an ExceptionFilterOptions into the
# internal form passed to the creator.  This function also allows for
# type-checking of each ExceptionFilterOptions.
@type_check
def _rewrite_exception_breakpoint(
    *,
    filterId: str,
    condition: Optional[str] = None,
    # Note that exception breakpoints do not support a hit count.
    **args,
):
    return {
        "filterId": filterId,
        "condition": condition,
    }


@request("setExceptionBreakpoints")
@capability("supportsExceptionFilterOptions")
@capability(
    "exceptionBreakpointFilters",
    (
        {
            "filter": "assert",
            "label": "Ada assertions",
            "supportsCondition": True,
        },
        {
            "filter": "exception",
            "label": "Ada exceptions",
            "supportsCondition": True,
        },
        {
            "filter": "throw",
            "label": "C++ exceptions, when thrown",
            "supportsCondition": True,
        },
        {
            "filter": "rethrow",
            "label": "C++ exceptions, when re-thrown",
            "supportsCondition": True,
        },
        {
            "filter": "catch",
            "label": "C++ exceptions, when caught",
            "supportsCondition": True,
        },
    ),
)
def set_exception_breakpoints(
    *, filters: Sequence[str], filterOptions: Sequence = (), **args
):
    # Convert the 'filters' to the filter-options style.
    options = [{"filterId": filter} for filter in filters]
    options.extend(filterOptions)
    options = [_rewrite_exception_breakpoint(**bp) for bp in options]
    return {
        "breakpoints": _set_exception_catchpoints(options),
    }
