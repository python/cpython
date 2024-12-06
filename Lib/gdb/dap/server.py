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

import functools
import heapq
import inspect
import json
import threading
from contextlib import contextmanager

import gdb

from .io import read_json, start_json_writer
from .startup import (
    DAPException,
    DAPQueue,
    LogLevel,
    exec_and_log,
    in_dap_thread,
    in_gdb_thread,
    log,
    log_stack,
    start_thread,
    thread_log,
)
from .typecheck import type_check

# Map capability names to values.
_capabilities = {}

# Map command names to callables.
_commands = {}

# The global server.
_server = None


# A subclass of Exception that is used solely for reporting that a
# request needs the inferior to be stopped, but it is not stopped.
class NotStoppedException(Exception):
    pass


# This is used to handle cancellation requests.  It tracks all the
# needed state, so that we can cancel both requests that are in flight
# as well as queued requests.
class CancellationHandler:
    def __init__(self):
        # Methods on this class acquire this lock before proceeding.
        self.lock = threading.Lock()
        # The request currently being handled, or None.
        self.in_flight_dap_thread = None
        self.in_flight_gdb_thread = None
        self.reqs = []

    def starting(self, req):
        """Call at the start of the given request."""
        with self.lock:
            self.in_flight_dap_thread = req

    def done(self, req):
        """Indicate that the request is done."""
        with self.lock:
            self.in_flight_dap_thread = None

    def cancel(self, req):
        """Call to cancel a request.

        If the request has already finished, this is ignored.
        If the request is in flight, it is interrupted.
        If the request has not yet been seen, the cancellation is queued."""
        with self.lock:
            if req == self.in_flight_gdb_thread:
                gdb.interrupt()
            else:
                # We don't actually ignore the request here, but in
                # the 'starting' method.  This way we don't have to
                # track as much state.  Also, this implementation has
                # the weird property that a request can be cancelled
                # before it is even sent.  It didn't seem worthwhile
                # to try to check for this.
                heapq.heappush(self.reqs, req)

    @contextmanager
    def interruptable_region(self, req):
        """Return a new context manager that sets in_flight_gdb_thread to
        REQ."""
        if req is None:
            # No request is handled in the region, just execute the region.
            yield
            return
        try:
            with self.lock:
                # If the request is cancelled, don't execute the region.
                while len(self.reqs) > 0 and self.reqs[0] <= req:
                    if heapq.heappop(self.reqs) == req:
                        raise KeyboardInterrupt()
                # Request is being handled by the gdb thread.
                self.in_flight_gdb_thread = req
            # Execute region.  This may be interrupted by gdb.interrupt.
            yield
        finally:
            with self.lock:
                # Request is no longer handled by the gdb thread.
                self.in_flight_gdb_thread = None


class Server:
    """The DAP server class."""

    def __init__(self, in_stream, out_stream, child_stream):
        self.in_stream = in_stream
        self.out_stream = out_stream
        self.child_stream = child_stream
        self.delayed_events = []
        # This queue accepts JSON objects that are then sent to the
        # DAP client.  Writing is done in a separate thread to avoid
        # blocking the read loop.
        self.write_queue = DAPQueue()
        # Reading is also done in a separate thread, and a queue of
        # requests is kept.
        self.read_queue = DAPQueue()
        self.done = False
        self.canceller = CancellationHandler()
        global _server
        _server = self

    # Treat PARAMS as a JSON-RPC request and perform its action.
    # PARAMS is just a dictionary from the JSON.
    @in_dap_thread
    def _handle_command(self, params):
        req = params["seq"]
        result = {
            "request_seq": req,
            "type": "response",
            "command": params["command"],
        }
        try:
            self.canceller.starting(req)
            if "arguments" in params:
                args = params["arguments"]
            else:
                args = {}
            global _commands
            body = _commands[params["command"]](**args)
            if body is not None:
                result["body"] = body
            result["success"] = True
        except NotStoppedException:
            # This is an expected exception, and the result is clearly
            # visible in the log, so do not log it.
            result["success"] = False
            result["message"] = "notStopped"
        except KeyboardInterrupt:
            # This can only happen when a request has been canceled.
            result["success"] = False
            result["message"] = "cancelled"
        except DAPException as e:
            # Don't normally want to see this, as it interferes with
            # the test suite.
            log_stack(LogLevel.FULL)
            result["success"] = False
            result["message"] = str(e)
        except BaseException as e:
            log_stack()
            result["success"] = False
            result["message"] = str(e)
        self.canceller.done(req)
        return result

    # Read inferior output and sends OutputEvents to the client.  It
    # is run in its own thread.
    def _read_inferior_output(self):
        while True:
            line = self.child_stream.readline()
            self.send_event(
                "output",
                {
                    "category": "stdout",
                    "output": line,
                },
            )

    # Send OBJ to the client, logging first if needed.
    def _send_json(self, obj):
        log("WROTE: <<<" + json.dumps(obj) + ">>>")
        self.write_queue.put(obj)

    # This is run in a separate thread and simply reads requests from
    # the client and puts them into a queue.  A separate thread is
    # used so that 'cancel' requests can be handled -- the DAP thread
    # will normally block, waiting for each request to complete.
    def _reader_thread(self):
        while True:
            cmd = read_json(self.in_stream)
            if cmd is None:
                break
            log("READ: <<<" + json.dumps(cmd) + ">>>")
            # Be extra paranoid about the form here.  If anything is
            # missing, it will be put in the queue and then an error
            # issued by ordinary request processing.
            if (
                "command" in cmd
                and cmd["command"] == "cancel"
                and "arguments" in cmd
                # gdb does not implement progress, so there's no need
                # to check for progressId.
                and "requestId" in cmd["arguments"]
            ):
                self.canceller.cancel(cmd["arguments"]["requestId"])
            self.read_queue.put(cmd)
        # When we hit EOF, signal it with None.
        self.read_queue.put(None)

    @in_dap_thread
    def main_loop(self):
        """The main loop of the DAP server."""
        # Before looping, start the thread that writes JSON to the
        # client, and the thread that reads output from the inferior.
        start_thread("output reader", self._read_inferior_output)
        json_writer = start_json_writer(self.out_stream, self.write_queue)
        start_thread("JSON reader", self._reader_thread)
        while not self.done:
            cmd = self.read_queue.get()
            # A None value here means the reader hit EOF.
            if cmd is None:
                break
            result = self._handle_command(cmd)
            self._send_json(result)
            events = self.delayed_events
            self.delayed_events = []
            for event, body in events:
                self.send_event(event, body)
        # Got the terminate request.  This is handled by the
        # JSON-writing thread, so that we can ensure that all
        # responses are flushed to the client before exiting.
        self.write_queue.put(None)
        json_writer.join()
        send_gdb("quit")

    @in_dap_thread
    def send_event_later(self, event, body=None):
        """Send a DAP event back to the client, but only after the
        current request has completed."""
        self.delayed_events.append((event, body))

    # Note that this does not need to be run in any particular thread,
    # because it just creates an object and writes it to a thread-safe
    # queue.
    def send_event(self, event, body=None):
        """Send an event to the DAP client.
        EVENT is the name of the event, a string.
        BODY is the body of the event, an arbitrary object."""
        obj = {
            "type": "event",
            "event": event,
        }
        if body is not None:
            obj["body"] = body
        self._send_json(obj)

    def shutdown(self):
        """Request that the server shut down."""
        # Just set a flag.  This operation is complicated because we
        # want to write the result of the request before exiting.  See
        # main_loop.
        self.done = True


def send_event(event, body=None):
    """Send an event to the DAP client.
    EVENT is the name of the event, a string.
    BODY is the body of the event, an arbitrary object."""
    global _server
    _server.send_event(event, body)


# A helper decorator that checks whether the inferior is running.
def _check_not_running(func):
    @functools.wraps(func)
    def check(*args, **kwargs):
        # Import this as late as possible.  This is done to avoid
        # circular imports.
        from .events import inferior_running

        if inferior_running:
            raise NotStoppedException()
        return func(*args, **kwargs)

    return check


def request(
    name: str,
    *,
    response: bool = True,
    on_dap_thread: bool = False,
    expect_stopped: bool = True
):
    """A decorator for DAP requests.

    This registers the function as the implementation of the DAP
    request NAME.  By default, the function is invoked in the gdb
    thread, and its result is returned as the 'body' of the DAP
    response.

    Some keyword arguments are provided as well:

    If RESPONSE is False, the result of the function will not be
    waited for and no 'body' will be in the response.

    If ON_DAP_THREAD is True, the function will be invoked in the DAP
    thread.  When ON_DAP_THREAD is True, RESPONSE may not be False.

    If EXPECT_STOPPED is True (the default), then the request will
    fail with the 'notStopped' reason if it is processed while the
    inferior is running.  When EXPECT_STOPPED is False, the request
    will proceed regardless of the inferior's state.
    """

    # Validate the parameters.
    assert not on_dap_thread or response

    def wrap(func):
        code = func.__code__
        # We don't permit requests to have positional arguments.
        try:
            assert code.co_posonlyargcount == 0
        except AttributeError:
            # Attribute co_posonlyargcount is supported starting python 3.8.
            pass
        assert code.co_argcount == 0
        # A request must have a **args parameter.
        assert code.co_flags & inspect.CO_VARKEYWORDS

        # Type-check the calls.
        func = type_check(func)

        # Verify that the function is run on the correct thread.
        if on_dap_thread:
            cmd = in_dap_thread(func)
        else:
            func = in_gdb_thread(func)

            if response:

                def sync_call(**args):
                    return send_gdb_with_response(lambda: func(**args))

                cmd = sync_call
            else:

                def non_sync_call(**args):
                    return send_gdb(lambda: func(**args))

                cmd = non_sync_call

        # If needed, check that the inferior is not running.  This
        # wrapping is done last, so the check is done first, before
        # trying to dispatch the request to another thread.
        if expect_stopped:
            cmd = _check_not_running(cmd)

        global _commands
        assert name not in _commands
        _commands[name] = cmd
        return cmd

    return wrap


def capability(name, value=True):
    """A decorator that indicates that the wrapper function implements
    the DAP capability NAME."""

    def wrap(func):
        global _capabilities
        assert name not in _capabilities
        _capabilities[name] = value
        return func

    return wrap


def client_bool_capability(name):
    """Return the value of a boolean client capability.

    If the capability was not specified, or did not have boolean type,
    False is returned."""
    global _server
    if name in _server.config and isinstance(_server.config[name], bool):
        return _server.config[name]
    return False


@request("initialize", on_dap_thread=True)
def initialize(**args):
    global _server, _capabilities
    _server.config = args
    _server.send_event_later("initialized")
    return _capabilities.copy()


@request("terminate", expect_stopped=False)
@capability("supportsTerminateRequest")
def terminate(**args):
    exec_and_log("kill")


@request("disconnect", on_dap_thread=True, expect_stopped=False)
@capability("supportTerminateDebuggee")
def disconnect(*, terminateDebuggee: bool = False, **args):
    if terminateDebuggee:
        send_gdb_with_response("kill")
    _server.shutdown()


@request("cancel", on_dap_thread=True, expect_stopped=False)
@capability("supportsCancelRequest")
def cancel(**args):
    # If a 'cancel' request can actually be satisfied, it will be
    # handled specially in the reader thread.  However, in order to
    # construct a proper response, the request is also added to the
    # command queue and so ends up here.  Additionally, the spec says:
    #    The cancel request may return an error if it could not cancel
    #    an operation but a client should refrain from presenting this
    #    error to end users.
    # ... which gdb takes to mean that it is fine for all cancel
    # requests to report success.
    return None


class Invoker(object):
    """A simple class that can invoke a gdb command."""

    def __init__(self, cmd):
        self.cmd = cmd

    # This is invoked in the gdb thread to run the command.
    @in_gdb_thread
    def __call__(self):
        exec_and_log(self.cmd)


class Cancellable(object):

    def __init__(self, fn, result_q=None):
        self.fn = fn
        self.result_q = result_q
        with _server.canceller.lock:
            self.req = _server.canceller.in_flight_dap_thread

    # This is invoked in the gdb thread to run self.fn.
    @in_gdb_thread
    def __call__(self):
        try:
            with _server.canceller.interruptable_region(self.req):
                val = self.fn()
                if self.result_q is not None:
                    self.result_q.put(val)
        except (Exception, KeyboardInterrupt) as e:
            if self.result_q is not None:
                # Pass result or exception to caller.
                self.result_q.put(e)
            elif isinstance(e, KeyboardInterrupt):
                # Fn was cancelled.
                pass
            else:
                # Exception happened.  Ignore and log it.
                err_string = "%s, %s" % (e, type(e))
                thread_log("caught exception: " + err_string)
                log_stack()


def send_gdb(cmd):
    """Send CMD to the gdb thread.
    CMD can be either a function or a string.
    If it is a string, it is passed to gdb.execute."""
    if isinstance(cmd, str):
        cmd = Invoker(cmd)

    # Post the event and don't wait for the result.
    gdb.post_event(Cancellable(cmd))


def send_gdb_with_response(fn):
    """Send FN to the gdb thread and return its result.
    If FN is a string, it is passed to gdb.execute and None is
    returned as the result.
    If FN throws an exception, this function will throw the
    same exception in the calling thread.
    """
    if isinstance(fn, str):
        fn = Invoker(fn)

    # Post the event and wait for the result in result_q.
    result_q = DAPQueue()
    gdb.post_event(Cancellable(fn, result_q))
    val = result_q.get()

    if isinstance(val, (Exception, KeyboardInterrupt)):
        raise val
    return val
