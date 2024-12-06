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

from .frames import select_frame
from .server import capability, client_bool_capability, request
from .startup import DAPException, in_gdb_thread, parse_and_eval
from .varref import VariableReference, apply_format, find_variable


class EvaluateResult(VariableReference):
    def __init__(self, value):
        super().__init__(None, value, "result")


# Helper function to evaluate an expression in a certain frame.
@in_gdb_thread
def _evaluate(expr, frame_id, value_format):
    with apply_format(value_format):
        global_context = True
        if frame_id is not None:
            select_frame(frame_id)
            global_context = False
        val = parse_and_eval(expr, global_context=global_context)
        ref = EvaluateResult(val)
        return ref.to_object()


# Like _evaluate but ensure that the expression cannot cause side
# effects.
@in_gdb_thread
def _eval_for_hover(expr, frame_id, value_format):
    with gdb.with_parameter("may-write-registers", "off"):
        with gdb.with_parameter("may-write-memory", "off"):
            with gdb.with_parameter("may-call-functions", "off"):
                return _evaluate(expr, frame_id, value_format)


class _SetResult(VariableReference):
    def __init__(self, value):
        super().__init__(None, value, "value")


# Helper function to evaluate a gdb command in a certain frame.
@in_gdb_thread
def _repl(command, frame_id):
    if frame_id is not None:
        select_frame(frame_id)
    val = gdb.execute(command, from_tty=True, to_string=True)
    return {
        "result": val,
        "variablesReference": 0,
    }


@request("evaluate")
@capability("supportsEvaluateForHovers")
@capability("supportsValueFormattingOptions")
def eval_request(
    *,
    expression: str,
    frameId: Optional[int] = None,
    context: str = "variables",
    format=None,
    **args,
):
    if context in ("watch", "variables"):
        # These seem to be expression-like.
        return _evaluate(expression, frameId, format)
    elif context == "hover":
        return _eval_for_hover(expression, frameId, format)
    elif context == "repl":
        # Ignore the format for repl evaluation.
        return _repl(expression, frameId)
    else:
        raise DAPException('unknown evaluate context "' + context + '"')


@request("variables")
# Note that we ignore the 'filter' field.  That seems to be
# specific to javascript.
def variables(
    *, variablesReference: int, start: int = 0, count: int = 0, format=None, **args
):
    # This behavior was clarified here:
    # https://github.com/microsoft/debug-adapter-protocol/pull/394
    if not client_bool_capability("supportsVariablePaging"):
        start = 0
        count = 0
    with apply_format(format):
        var = find_variable(variablesReference)
        children = var.fetch_children(start, count)
        return {"variables": [x.to_object() for x in children]}


@capability("supportsSetExpression")
@request("setExpression")
def set_expression(
    *, expression: str, value: str, frameId: Optional[int] = None, format=None, **args
):
    with apply_format(format):
        global_context = True
        if frameId is not None:
            select_frame(frameId)
            global_context = False
        lhs = parse_and_eval(expression, global_context=global_context)
        rhs = parse_and_eval(value, global_context=global_context)
        lhs.assign(rhs)
        return _SetResult(lhs).to_object()


@capability("supportsSetVariable")
@request("setVariable")
def set_variable(
    *, variablesReference: int, name: str, value: str, format=None, **args
):
    with apply_format(format):
        var = find_variable(variablesReference)
        lhs = var.find_child_by_name(name)
        rhs = parse_and_eval(value)
        lhs.assign(rhs)
        return lhs.to_object()
