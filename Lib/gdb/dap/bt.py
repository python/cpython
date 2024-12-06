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

# This is deprecated in 3.9, but required in older versions.
from typing import Optional

import gdb

from .frames import dap_frame_generator
from .modules import module_id
from .scopes import symbol_value
from .server import capability, request
from .sources import make_source
from .startup import in_gdb_thread
from .state import set_thread
from .typecheck import type_check
from .varref import apply_format


# Helper function to compute parameter information for a stack frame.
@in_gdb_thread
def _compute_parameters(frame, stack_format):
    arg_iter = frame.frame_args()
    if arg_iter is None:
        return ""
    result = []
    for arg in arg_iter:
        desc = []
        name, val = symbol_value(arg, frame)
        # We don't try to use any particular language's syntax for the
        # output here.
        if stack_format["parameterTypes"]:
            desc.append("[" + str(val.type) + "]")
        if stack_format["parameterNames"]:
            desc.append(name)
            # If both the name and the value are requested, insert an
            # '=' for clarity.
            if stack_format["parameterValues"]:
                desc.append("=")
        if stack_format["parameterValues"]:
            desc.append(val.format_string(summary=True))
        result.append(" ".join(desc))
    return ", ".join(result)


# Helper function to compute a stack trace.
@in_gdb_thread
def _backtrace(thread_id, levels, startFrame, stack_format):
    with apply_format(stack_format):
        set_thread(thread_id)
        frames = []
        frame_iter = dap_frame_generator(startFrame, levels, stack_format["includeAll"])
        for frame_id, current_frame in frame_iter:
            pc = current_frame.address()
            # The stack frame format affects the name, so we build it up
            # piecemeal and assign it at the end.
            name = current_frame.function()
            # The meaning of StackFrameFormat.parameters was clarified
            # in https://github.com/microsoft/debug-adapter-protocol/issues/411.
            if stack_format["parameters"] and (
                stack_format["parameterTypes"]
                or stack_format["parameterNames"]
                or stack_format["parameterValues"]
            ):
                name += "(" + _compute_parameters(current_frame, stack_format) + ")"
            newframe = {
                "id": frame_id,
                # This must always be supplied, but we will set it
                # correctly later if that is possible.
                "line": 0,
                # GDB doesn't support columns.
                "column": 0,
                "instructionPointerReference": hex(pc),
            }
            line = current_frame.line()
            if line is not None:
                newframe["line"] = line
                if stack_format["line"]:
                    name += ", line " + str(line)
            objfile = gdb.current_progspace().objfile_for_address(pc)
            if objfile is not None:
                newframe["moduleId"] = module_id(objfile)
                if stack_format["module"]:
                    name += ", module " + objfile.username
            filename = current_frame.filename()
            if filename is not None:
                newframe["source"] = make_source(filename)
            newframe["name"] = name
            frames.append(newframe)
        # Note that we do not calculate totalFrames here.  Its absence
        # tells the client that it may simply ask for frames until a
        # response yields fewer frames than requested.
        return {
            "stackFrames": frames,
        }


# A helper function that checks the types of the elements of a
# StackFrameFormat, and converts this to a dict where all the members
# are set.  This simplifies the implementation code a bit.
@type_check
def check_stack_frame(
    *,
    # Note that StackFrameFormat extends ValueFormat, which is why
    # "hex" appears here.
    hex: Optional[bool] = False,
    parameters: Optional[bool] = False,
    parameterTypes: Optional[bool] = False,
    parameterNames: Optional[bool] = False,
    parameterValues: Optional[bool] = False,
    line: Optional[bool] = False,
    module: Optional[bool] = False,
    includeAll: Optional[bool] = False,
    **rest
):
    return {
        "hex": hex,
        "parameters": parameters,
        "parameterTypes": parameterTypes,
        "parameterNames": parameterNames,
        "parameterValues": parameterValues,
        "line": line,
        "module": module,
        "includeAll": includeAll,
    }


@request("stackTrace")
@capability("supportsDelayedStackTraceLoading")
def stacktrace(
    *, levels: int = 0, startFrame: int = 0, threadId: int, format=None, **extra
):
    # It's simpler if the format is always set.
    if format is None:
        format = {}
    format = check_stack_frame(**format)
    return _backtrace(threadId, levels, startFrame, format)
