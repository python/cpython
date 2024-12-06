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

import gdb

from .modules import is_module, make_module
from .scopes import set_finish_value
from .server import send_event
from .startup import exec_and_log, in_gdb_thread, log

# True when the inferior is thought to be running, False otherwise.
# This may be accessed from any thread, which can be racy.  However,
# this unimportant because this global is only used for the
# 'notStopped' response, which itself is inherently racy.
inferior_running = False


@in_gdb_thread
def _on_exit(event):
    global inferior_running
    inferior_running = False
    code = 0
    if hasattr(event, "exit_code"):
        code = event.exit_code
    send_event(
        "exited",
        {
            "exitCode": code,
        },
    )
    send_event("terminated")


# When None, a "process" event has already been sent.  When a string,
# it is the "startMethod" for that event.
_process_event_kind = None


@in_gdb_thread
def send_process_event_once():
    global _process_event_kind
    if _process_event_kind is not None:
        inf = gdb.selected_inferior()
        is_local = inf.connection.type == "native"
        data = {
            "isLocalProcess": is_local,
            "startMethod": _process_event_kind,
            # Could emit 'pointerSize' here too if we cared to.
        }
        if inf.progspace.filename:
            data["name"] = inf.progspace.filename
        if is_local:
            data["systemProcessId"] = inf.pid
        send_event("process", data)
        _process_event_kind = None


@in_gdb_thread
def expect_process(reason):
    """Indicate that DAP is starting or attaching to a process.

    REASON is the "startMethod" to include in the "process" event.
    """
    global _process_event_kind
    _process_event_kind = reason


@in_gdb_thread
def thread_event(event, reason):
    send_process_event_once()
    send_event(
        "thread",
        {
            "reason": reason,
            "threadId": event.inferior_thread.global_num,
        },
    )


@in_gdb_thread
def _new_thread(event):
    global inferior_running
    inferior_running = True
    thread_event(event, "started")


@in_gdb_thread
def _thread_exited(event):
    thread_event(event, "exited")


@in_gdb_thread
def _new_objfile(event):
    if is_module(event.new_objfile):
        send_event(
            "module",
            {
                "reason": "new",
                "module": make_module(event.new_objfile),
            },
        )


@in_gdb_thread
def _objfile_removed(event):
    send_process_event_once()
    if is_module(event.objfile):
        send_event(
            "module",
            {
                "reason": "removed",
                "module": make_module(event.objfile),
            },
        )


_suppress_cont = False


@in_gdb_thread
def _cont(event):
    global inferior_running
    inferior_running = True
    global _suppress_cont
    if _suppress_cont:
        log("_suppress_cont case")
        _suppress_cont = False
    else:
        send_event(
            "continued",
            {
                "threadId": gdb.selected_thread().global_num,
                "allThreadsContinued": True,
            },
        )


_expected_stop_reason = None


@in_gdb_thread
def expect_stop(reason: str):
    """Indicate that the next stop should be for REASON."""
    global _expected_stop_reason
    _expected_stop_reason = reason


_expected_pause = False


@in_gdb_thread
def exec_and_expect_stop(cmd, expected_pause=False):
    """A wrapper for exec_and_log that sets the continue-suppression flag.

    When EXPECTED_PAUSE is True, a stop that looks like a pause (e.g.,
    a SIGINT) will be reported as "pause" instead.
    """
    global _expected_pause
    _expected_pause = expected_pause
    global _suppress_cont
    # If we're expecting a pause, then we're definitely not
    # continuing.
    _suppress_cont = not expected_pause
    # FIXME if the call fails should we clear _suppress_cont?
    exec_and_log(cmd)


# Map from gdb stop reasons to DAP stop reasons.  Some of these can't
# be seen ordinarily in DAP -- only if the client lets the user toggle
# some settings (e.g. stop-on-solib-events) or enter commands (e.g.,
# 'until').
stop_reason_map = {
    "breakpoint-hit": "breakpoint",
    "watchpoint-trigger": "data breakpoint",
    "read-watchpoint-trigger": "data breakpoint",
    "access-watchpoint-trigger": "data breakpoint",
    "function-finished": "step",
    "location-reached": "step",
    "watchpoint-scope": "data breakpoint",
    "end-stepping-range": "step",
    "exited-signalled": "exited",
    "exited": "exited",
    "exited-normally": "exited",
    "signal-received": "signal",
    "solib-event": "solib",
    "fork": "fork",
    "vfork": "vfork",
    "syscall-entry": "syscall-entry",
    "syscall-return": "syscall-return",
    "exec": "exec",
    "no-history": "no-history",
}


@in_gdb_thread
def _on_stop(event):
    global inferior_running
    inferior_running = False

    log("entering _on_stop: " + repr(event))
    if hasattr(event, "details"):
        log("   details: " + repr(event.details))
    obj = {
        "threadId": gdb.selected_thread().global_num,
        "allThreadsStopped": True,
    }
    if isinstance(event, gdb.BreakpointEvent):
        obj["hitBreakpointIds"] = [x.number for x in event.breakpoints]
    if hasattr(event, "details") and "finish-value" in event.details:
        set_finish_value(event.details["finish-value"])

    global _expected_pause
    global _expected_stop_reason
    if _expected_stop_reason is not None:
        obj["reason"] = _expected_stop_reason
        _expected_stop_reason = None
    elif "reason" not in event.details:
        # This can only really happen via a "repl" evaluation of
        # something like "attach".  In this case just emit a generic
        # stop.
        obj["reason"] = "stopped"
    elif (
        _expected_pause
        and event.details["reason"] == "signal-received"
        and event.details["signal-name"] in ("SIGINT", "SIGSTOP")
    ):
        obj["reason"] = "pause"
    else:
        global stop_reason_map
        obj["reason"] = stop_reason_map[event.details["reason"]]
    _expected_pause = False
    send_event("stopped", obj)


# This keeps a bit of state between the start of an inferior call and
# the end.  If the inferior was already running when the call started
# (as can happen if a breakpoint condition calls a function), then we
# do not want to emit 'continued' or 'stop' events for the call.  Note
# that, for some reason, gdb.events.cont does not fire for an infcall.
_infcall_was_running = False


@in_gdb_thread
def _on_inferior_call(event):
    global _infcall_was_running
    global inferior_running
    if isinstance(event, gdb.InferiorCallPreEvent):
        _infcall_was_running = inferior_running
        if not _infcall_was_running:
            _cont(None)
    else:
        # If the inferior is already marked as stopped here, then that
        # means that the call caused some other stop, and we don't
        # want to double-report it.
        if not _infcall_was_running and inferior_running:
            inferior_running = False
            obj = {
                "threadId": gdb.selected_thread().global_num,
                "allThreadsStopped": True,
                # DAP says any string is ok.
                "reason": "function call",
            }
            global _expected_pause
            _expected_pause = False
            send_event("stopped", obj)


gdb.events.stop.connect(_on_stop)
gdb.events.exited.connect(_on_exit)
gdb.events.new_thread.connect(_new_thread)
gdb.events.thread_exited.connect(_thread_exited)
gdb.events.cont.connect(_cont)
gdb.events.new_objfile.connect(_new_objfile)
gdb.events.free_objfile.connect(_objfile_removed)
gdb.events.inferior_call.connect(_on_inferior_call)
