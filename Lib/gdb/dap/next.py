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

from .events import exec_and_expect_stop
from .server import capability, request, send_gdb, send_gdb_with_response
from .startup import in_gdb_thread
from .state import set_thread


# Helper function to set the current thread and the scheduler-locking
# mode.  Returns True if scheduler-locking was successfully set to
# 'on', False in all other cases, including error.  When SELECT is
# True, also select that thread's newest frame.
@in_gdb_thread
def _handle_thread_step(thread_id, single_thread, select=False):
    # Ensure we're going to step the correct thread.
    set_thread(thread_id)
    if single_thread:
        result = True
        arg = "on"
    else:
        result = False
        arg = "off"
    try:
        # This can fail, depending on the target, so catch the error
        # and report to our caller.  We can't use exec_and_log because
        # that does not propagate exceptions.
        gdb.execute("set scheduler-locking " + arg, from_tty=True, to_string=True)
    except gdb.error:
        result = False
    # Other DAP code may select a frame, and the "finish" command uses
    # the selected frame.
    if select:
        gdb.newest_frame().select()
    return result


@request("next", response=False)
def next(
    *, threadId: int, singleThread: bool = False, granularity: str = "statement", **args
):
    _handle_thread_step(threadId, singleThread)
    cmd = "next"
    if granularity == "instruction":
        cmd += "i"
    exec_and_expect_stop(cmd)


@capability("supportsSteppingGranularity")
@capability("supportsSingleThreadExecutionRequests")
@request("stepIn", response=False)
def step_in(
    *, threadId: int, singleThread: bool = False, granularity: str = "statement", **args
):
    _handle_thread_step(threadId, singleThread)
    cmd = "step"
    if granularity == "instruction":
        cmd += "i"
    exec_and_expect_stop(cmd)


@request("stepOut", response=False)
def step_out(*, threadId: int, singleThread: bool = False, **args):
    _handle_thread_step(threadId, singleThread, True)
    exec_and_expect_stop("finish")


# This is a server-side request because it is funny: it wants to
# 'continue' but also return a result, which precludes using
# response=False.  Using 'continue &' would mostly work ok, but this
# yields races when a stop occurs before the response is sent back to
# the client.
@request("continue", on_dap_thread=True)
def continue_request(*, threadId: int, singleThread: bool = False, **args):
    locked = send_gdb_with_response(lambda: _handle_thread_step(threadId, singleThread))
    send_gdb(lambda: exec_and_expect_stop("continue"))
    return {"allThreadsContinued": not locked}
