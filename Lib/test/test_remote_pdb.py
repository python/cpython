import io
import time
import itertools
import json
import os
import signal
import socket
import subprocess
import sys
import tempfile
import textwrap
import threading
import unittest
import unittest.mock
from contextlib import contextmanager, redirect_stdout, ExitStack
from pathlib import Path
from test.support import is_wasi, os_helper, requires_subprocess, SHORT_TIMEOUT
from test.support.os_helper import temp_dir, TESTFN, unlink
from typing import Dict, List, Optional, Tuple, Union, Any

import pdb
from pdb import _PdbServer, _PdbClient


if not sys.is_remote_debug_enabled():
    raise unittest.SkipTest('remote debugging is disabled')


@contextmanager
def kill_on_error(proc):
    """Context manager killing the subprocess if a Python exception is raised."""
    with proc:
        try:
            yield proc
        except:
            proc.kill()
            raise


class MockSocketFile:
    """Mock socket file for testing _PdbServer without actual socket connections."""

    def __init__(self):
        self.input_queue = []
        self.output_buffer = []

    def write(self, data: bytes) -> None:
        """Simulate write to socket."""
        self.output_buffer.append(data)

    def flush(self) -> None:
        """No-op flush implementation."""
        pass

    def readline(self) -> bytes:
        """Read a line from the prepared input queue."""
        if not self.input_queue:
            return b""
        return self.input_queue.pop(0)

    def close(self) -> None:
        """Close the mock socket file."""
        pass

    def add_input(self, data: dict) -> None:
        """Add input that will be returned by readline."""
        self.input_queue.append(json.dumps(data).encode() + b"\n")

    def get_output(self) -> List[dict]:
        """Get the output that was written by the object being tested."""
        results = []
        for data in self.output_buffer:
            if isinstance(data, bytes) and data.endswith(b"\n"):
                try:
                    results.append(json.loads(data.decode().strip()))
                except json.JSONDecodeError:
                    pass  # Ignore non-JSON output
        self.output_buffer = []
        return results


class MockDebuggerSocket:
    """Mock file-like simulating a connection to a _RemotePdb instance"""

    def __init__(self, incoming):
        self.incoming = iter(incoming)
        self.outgoing = []
        self.buffered = bytearray()

    def write(self, data: bytes) -> None:
        """Simulate write to socket."""
        self.buffered += data

    def flush(self) -> None:
        """Ensure each line is valid JSON."""
        lines = self.buffered.splitlines(keepends=True)
        self.buffered.clear()
        for line in lines:
            assert line.endswith(b"\n")
            self.outgoing.append(json.loads(line))

    def readline(self) -> bytes:
        """Read a line from the prepared input queue."""
        # Anything written must be flushed before trying to read,
        # since the read will be dependent upon the last write.
        assert not self.buffered
        try:
            item = next(self.incoming)
            if not isinstance(item, bytes):
                item = json.dumps(item).encode()
            return item + b"\n"
        except StopIteration:
            return b""

    def close(self) -> None:
        """No-op close implementation."""
        pass


class PdbClientTestCase(unittest.TestCase):
    """Tests for the _PdbClient class."""

    def do_test(
        self,
        *,
        incoming,
        simulate_failure=None,
        expected_outgoing=None,
        expected_completions=None,
        expected_exception=None,
        expected_stdout="",
        expected_stdout_substring="",
        expected_state=None,
    ):
        if expected_outgoing is None:
            expected_outgoing = []
        if expected_completions is None:
            expected_completions = []
        if expected_state is None:
            expected_state = {}

        expected_state.setdefault("write_failed", False)
        messages = [m for source, m in incoming if source == "server"]
        prompts = [m["prompt"] for source, m in incoming if source == "user"]
        sockfile = MockDebuggerSocket(messages)
        stdout = io.StringIO()

        if simulate_failure:
            sockfile.write = unittest.mock.Mock()
            sockfile.flush = unittest.mock.Mock()
            if simulate_failure == "write":
                sockfile.write.side_effect = OSError("write failed")
            elif simulate_failure == "flush":
                sockfile.flush.side_effect = OSError("flush failed")

        input_iter = (m for source, m in incoming if source == "user")
        completions = []

        def mock_input(prompt):
            message = next(input_iter, None)
            if message is None:
                raise EOFError

            if req := message.get("completion_request"):
                readline_mock = unittest.mock.Mock()
                readline_mock.get_line_buffer.return_value = req["line"]
                readline_mock.get_begidx.return_value = req["begidx"]
                readline_mock.get_endidx.return_value = req["endidx"]
                unittest.mock.seal(readline_mock)
                with unittest.mock.patch.dict(sys.modules, {"readline": readline_mock}):
                    for param in itertools.count():
                        prefix = req["line"][req["begidx"] : req["endidx"]]
                        completion = client.complete(prefix, param)
                        if completion is None:
                            break
                        completions.append(completion)

            reply = message["input"]
            if isinstance(reply, BaseException):
                raise reply
            return reply

        with ExitStack() as stack:
            input_mock = stack.enter_context(
                unittest.mock.patch("pdb.input", side_effect=mock_input)
            )
            stack.enter_context(redirect_stdout(stdout))

            client = _PdbClient(
                pid=0,
                sockfile=sockfile,
                interrupt_script="/a/b.py",
            )

            if expected_exception is not None:
                exception = expected_exception["exception"]
                msg = expected_exception["msg"]
                stack.enter_context(self.assertRaises(exception, msg=msg))

            client.cmdloop()

        actual_outgoing = sockfile.outgoing
        if simulate_failure:
            actual_outgoing += [
                json.loads(msg.args[0]) for msg in sockfile.write.mock_calls
            ]

        self.assertEqual(sockfile.outgoing, expected_outgoing)
        self.assertEqual(completions, expected_completions)
        if expected_stdout_substring and not expected_stdout:
            self.assertIn(expected_stdout_substring, stdout.getvalue())
        else:
            self.assertEqual(stdout.getvalue(), expected_stdout)
        input_mock.assert_has_calls([unittest.mock.call(p) for p in prompts])
        actual_state = {k: getattr(client, k) for k in expected_state}
        self.assertEqual(actual_state, expected_state)

    def test_remote_immediately_closing_the_connection(self):
        """Test the behavior when the remote closes the connection immediately."""
        incoming = []
        expected_outgoing = []
        self.do_test(
            incoming=incoming,
            expected_outgoing=expected_outgoing,
        )

    def test_handling_command_list(self):
        """Test handling the command_list message."""
        incoming = [
            ("server", {"command_list": ["help", "list", "continue"]}),
        ]
        self.do_test(
            incoming=incoming,
            expected_outgoing=[],
            expected_state={
                "pdb_commands": {"help", "list", "continue"},
            },
        )

    def test_handling_info_message(self):
        """Test handling a message payload with type='info'."""
        incoming = [
            ("server", {"message": "Some message or other\n", "type": "info"}),
        ]
        self.do_test(
            incoming=incoming,
            expected_outgoing=[],
            expected_stdout="Some message or other\n",
        )

    def test_handling_error_message(self):
        """Test handling a message payload with type='error'."""
        incoming = [
            ("server", {"message": "Some message or other.", "type": "error"}),
        ]
        self.do_test(
            incoming=incoming,
            expected_outgoing=[],
            expected_stdout="*** Some message or other.\n",
        )

    def test_handling_other_message(self):
        """Test handling a message payload with an unrecognized type."""
        incoming = [
            ("server", {"message": "Some message.\n", "type": "unknown"}),
        ]
        self.do_test(
            incoming=incoming,
            expected_outgoing=[],
            expected_stdout="Some message.\n",
        )

    def test_handling_help_for_command(self):
        """Test handling a request to display help for a command."""
        incoming = [
            ("server", {"help": "ll"}),
        ]
        self.do_test(
            incoming=incoming,
            expected_outgoing=[],
            expected_stdout_substring="Usage: ll | longlist",
        )

    def test_handling_help_without_a_specific_topic(self):
        """Test handling a request to display a help overview."""
        incoming = [
            ("server", {"help": ""}),
        ]
        self.do_test(
            incoming=incoming,
            expected_outgoing=[],
            expected_stdout_substring="type help <topic>",
        )

    def test_handling_help_pdb(self):
        """Test handling a request to display the full PDB manual."""
        incoming = [
            ("server", {"help": "pdb"}),
        ]
        self.do_test(
            incoming=incoming,
            expected_outgoing=[],
            expected_stdout_substring=">>> import pdb",
        )

    def test_handling_pdb_prompts(self):
        """Test responding to pdb's normal prompts."""
        incoming = [
            ("server", {"command_list": ["b"]}),
            ("server", {"prompt": "(Pdb) ", "state": "pdb"}),
            ("user", {"prompt": "(Pdb) ", "input": "lst ["}),
            ("user", {"prompt": "...   ", "input": "0 ]"}),
            ("server", {"prompt": "(Pdb) ", "state": "pdb"}),
            ("user", {"prompt": "(Pdb) ", "input": ""}),
            ("server", {"prompt": "(Pdb) ", "state": "pdb"}),
            ("user", {"prompt": "(Pdb) ", "input": "b ["}),
            ("server", {"prompt": "(Pdb) ", "state": "pdb"}),
            ("user", {"prompt": "(Pdb) ", "input": "! b ["}),
            ("user", {"prompt": "...   ", "input": "b ]"}),
        ]
        self.do_test(
            incoming=incoming,
            expected_outgoing=[
                {"reply": "lst [\n0 ]"},
                {"reply": ""},
                {"reply": "b ["},
                {"reply": "!b [\nb ]"},
            ],
            expected_state={"state": "pdb"},
        )

    def test_handling_interact_prompts(self):
        """Test responding to pdb's interact mode prompts."""
        incoming = [
            ("server", {"command_list": ["b"]}),
            ("server", {"prompt": ">>> ", "state": "interact"}),
            ("user", {"prompt": ">>> ", "input": "lst ["}),
            ("user", {"prompt": "... ", "input": "0 ]"}),
            ("server", {"prompt": ">>> ", "state": "interact"}),
            ("user", {"prompt": ">>> ", "input": ""}),
            ("server", {"prompt": ">>> ", "state": "interact"}),
            ("user", {"prompt": ">>> ", "input": "b ["}),
            ("user", {"prompt": "... ", "input": "b ]"}),
        ]
        self.do_test(
            incoming=incoming,
            expected_outgoing=[
                {"reply": "lst [\n0 ]"},
                {"reply": ""},
                {"reply": "b [\nb ]"},
            ],
            expected_state={"state": "interact"},
        )

    def test_retry_pdb_prompt_on_syntax_error(self):
        """Test re-prompting after a SyntaxError in a Python expression."""
        incoming = [
            ("server", {"prompt": "(Pdb) ", "state": "pdb"}),
            ("user", {"prompt": "(Pdb) ", "input": " lst ["}),
            ("user", {"prompt": "(Pdb) ", "input": "lst ["}),
            ("user", {"prompt": "...   ", "input": " 0 ]"}),
        ]
        self.do_test(
            incoming=incoming,
            expected_outgoing=[
                {"reply": "lst [\n 0 ]"},
            ],
            expected_stdout_substring="*** IndentationError",
            expected_state={"state": "pdb"},
        )

    def test_retry_interact_prompt_on_syntax_error(self):
        """Test re-prompting after a SyntaxError in a Python expression."""
        incoming = [
            ("server", {"prompt": ">>> ", "state": "interact"}),
            ("user", {"prompt": ">>> ", "input": "!lst ["}),
            ("user", {"prompt": ">>> ", "input": "lst ["}),
            ("user", {"prompt": "... ", "input": " 0 ]"}),
        ]
        self.do_test(
            incoming=incoming,
            expected_outgoing=[
                {"reply": "lst [\n 0 ]"},
            ],
            expected_stdout_substring="*** SyntaxError",
            expected_state={"state": "interact"},
        )

    def test_handling_unrecognized_prompt_type(self):
        """Test fallback to "dumb" single-line mode for unknown states."""
        incoming = [
            ("server", {"prompt": "Do it? ", "state": "confirm"}),
            ("user", {"prompt": "Do it? ", "input": "! ["}),
            ("server", {"prompt": "Do it? ", "state": "confirm"}),
            ("user", {"prompt": "Do it? ", "input": "echo hello"}),
            ("server", {"prompt": "Do it? ", "state": "confirm"}),
            ("user", {"prompt": "Do it? ", "input": ""}),
            ("server", {"prompt": "Do it? ", "state": "confirm"}),
            ("user", {"prompt": "Do it? ", "input": "echo goodbye"}),
        ]
        self.do_test(
            incoming=incoming,
            expected_outgoing=[
                {"reply": "! ["},
                {"reply": "echo hello"},
                {"reply": ""},
                {"reply": "echo goodbye"},
            ],
            expected_state={"state": "dumb"},
        )

    def test_keyboard_interrupt_at_prompt(self):
        """Test signaling when a prompt gets a KeyboardInterrupt."""
        incoming = [
            ("server", {"prompt": "(Pdb) ", "state": "pdb"}),
            ("user", {"prompt": "(Pdb) ", "input": KeyboardInterrupt()}),
        ]
        self.do_test(
            incoming=incoming,
            expected_outgoing=[
                {"signal": "INT"},
            ],
            expected_state={"state": "pdb"},
        )

    def test_eof_at_prompt(self):
        """Test signaling when a prompt gets an EOFError."""
        incoming = [
            ("server", {"prompt": "(Pdb) ", "state": "pdb"}),
            ("user", {"prompt": "(Pdb) ", "input": EOFError()}),
        ]
        self.do_test(
            incoming=incoming,
            expected_outgoing=[
                {"signal": "EOF"},
            ],
            expected_state={"state": "pdb"},
        )

    def test_unrecognized_json_message(self):
        """Test failing after getting an unrecognized payload."""
        incoming = [
            ("server", {"monty": "python"}),
            ("server", {"message": "Some message or other\n", "type": "info"}),
        ]
        self.do_test(
            incoming=incoming,
            expected_outgoing=[],
            expected_exception={
                "exception": RuntimeError,
                "msg": 'Unrecognized payload b\'{"monty": "python"}\'',
            },
        )

    def test_continuing_after_getting_a_non_json_payload(self):
        """Test continuing after getting a non JSON payload."""
        incoming = [
            ("server", b"spam"),
            ("server", {"message": "Something", "type": "info"}),
        ]
        self.do_test(
            incoming=incoming,
            expected_outgoing=[],
            expected_stdout="\n".join(
                [
                    "*** Invalid JSON from remote: b'spam\\n'",
                    "Something",
                ]
            ),
        )

    def test_write_failing(self):
        """Test terminating if write fails due to a half closed socket."""
        incoming = [
            ("server", {"prompt": "(Pdb) ", "state": "pdb"}),
            ("user", {"prompt": "(Pdb) ", "input": KeyboardInterrupt()}),
        ]
        self.do_test(
            incoming=incoming,
            expected_outgoing=[{"signal": "INT"}],
            simulate_failure="write",
            expected_state={"write_failed": True},
        )

    def test_flush_failing(self):
        """Test terminating if flush fails due to a half closed socket."""
        incoming = [
            ("server", {"prompt": "(Pdb) ", "state": "pdb"}),
            ("user", {"prompt": "(Pdb) ", "input": KeyboardInterrupt()}),
        ]
        self.do_test(
            incoming=incoming,
            expected_outgoing=[{"signal": "INT"}],
            simulate_failure="flush",
            expected_state={"write_failed": True},
        )

    def test_completion_in_pdb_state(self):
        """Test requesting tab completions at a (Pdb) prompt."""
        # GIVEN
        incoming = [
            ("server", {"prompt": "(Pdb) ", "state": "pdb"}),
            (
                "user",
                {
                    "prompt": "(Pdb) ",
                    "completion_request": {
                        "line": "    mod._",
                        "begidx": 8,
                        "endidx": 9,
                    },
                    "input": "print(\n    mod.__name__)",
                },
            ),
            ("server", {"completions": ["__name__", "__file__"]}),
        ]
        self.do_test(
            incoming=incoming,
            expected_outgoing=[
                {
                    "complete": {
                        "text": "_",
                        "line": "mod._",
                        "begidx": 4,
                        "endidx": 5,
                    }
                },
                {"reply": "print(\n    mod.__name__)"},
            ],
            expected_completions=["__name__", "__file__"],
            expected_state={"state": "pdb"},
        )

    def test_completion_in_interact_state(self):
        """Test requesting tab completions at a >>> prompt."""
        incoming = [
            ("server", {"prompt": ">>> ", "state": "interact"}),
            (
                "user",
                {
                    "prompt": ">>> ",
                    "completion_request": {
                        "line": "    mod.__",
                        "begidx": 8,
                        "endidx": 10,
                    },
                    "input": "print(\n    mod.__name__)",
                },
            ),
            ("server", {"completions": ["__name__", "__file__"]}),
        ]
        self.do_test(
            incoming=incoming,
            expected_outgoing=[
                {
                    "complete": {
                        "text": "__",
                        "line": "mod.__",
                        "begidx": 4,
                        "endidx": 6,
                    }
                },
                {"reply": "print(\n    mod.__name__)"},
            ],
            expected_completions=["__name__", "__file__"],
            expected_state={"state": "interact"},
        )

    def test_completion_in_unknown_state(self):
        """Test requesting tab completions at an unrecognized prompt."""
        incoming = [
            ("server", {"command_list": ["p"]}),
            ("server", {"prompt": "Do it? ", "state": "confirm"}),
            (
                "user",
                {
                    "prompt": "Do it? ",
                    "completion_request": {
                        "line": "_",
                        "begidx": 0,
                        "endidx": 1,
                    },
                    "input": "__name__",
                },
            ),
        ]
        self.do_test(
            incoming=incoming,
            expected_outgoing=[
                {"reply": "__name__"},
            ],
            expected_state={"state": "dumb"},
        )

    def test_write_failure_during_completion(self):
        """Test failing to write to the socket to request tab completions."""
        incoming = [
            ("server", {"prompt": ">>> ", "state": "interact"}),
            (
                "user",
                {
                    "prompt": ">>> ",
                    "completion_request": {
                        "line": "xy",
                        "begidx": 0,
                        "endidx": 2,
                    },
                    "input": "xyz",
                },
            ),
        ]
        self.do_test(
            incoming=incoming,
            expected_outgoing=[
                {
                    "complete": {
                        "text": "xy",
                        "line": "xy",
                        "begidx": 0,
                        "endidx": 2,
                    }
                },
                {"reply": "xyz"},
            ],
            simulate_failure="write",
            expected_completions=[],
            expected_state={"state": "interact", "write_failed": True},
        )

    def test_flush_failure_during_completion(self):
        """Test failing to flush to the socket to request tab completions."""
        incoming = [
            ("server", {"prompt": ">>> ", "state": "interact"}),
            (
                "user",
                {
                    "prompt": ">>> ",
                    "completion_request": {
                        "line": "xy",
                        "begidx": 0,
                        "endidx": 2,
                    },
                    "input": "xyz",
                },
            ),
        ]
        self.do_test(
            incoming=incoming,
            expected_outgoing=[
                {
                    "complete": {
                        "text": "xy",
                        "line": "xy",
                        "begidx": 0,
                        "endidx": 2,
                    }
                },
                {"reply": "xyz"},
            ],
            simulate_failure="flush",
            expected_completions=[],
            expected_state={"state": "interact", "write_failed": True},
        )

    def test_read_failure_during_completion(self):
        """Test failing to read tab completions from the socket."""
        incoming = [
            ("server", {"prompt": ">>> ", "state": "interact"}),
            (
                "user",
                {
                    "prompt": ">>> ",
                    "completion_request": {
                        "line": "xy",
                        "begidx": 0,
                        "endidx": 2,
                    },
                    "input": "xyz",
                },
            ),
        ]
        self.do_test(
            incoming=incoming,
            expected_outgoing=[
                {
                    "complete": {
                        "text": "xy",
                        "line": "xy",
                        "begidx": 0,
                        "endidx": 2,
                    }
                },
                {"reply": "xyz"},
            ],
            expected_completions=[],
            expected_state={"state": "interact"},
        )

    def test_reading_invalid_json_during_completion(self):
        """Test receiving invalid JSON when getting tab completions."""
        incoming = [
            ("server", {"prompt": ">>> ", "state": "interact"}),
            (
                "user",
                {
                    "prompt": ">>> ",
                    "completion_request": {
                        "line": "xy",
                        "begidx": 0,
                        "endidx": 2,
                    },
                    "input": "xyz",
                },
            ),
            ("server", b'{"completions": '),
            ("user", {"prompt": ">>> ", "input": "xyz"}),
        ]
        self.do_test(
            incoming=incoming,
            expected_outgoing=[
                {
                    "complete": {
                        "text": "xy",
                        "line": "xy",
                        "begidx": 0,
                        "endidx": 2,
                    }
                },
                {"reply": "xyz"},
            ],
            expected_stdout_substring="*** json.decoder.JSONDecodeError",
            expected_completions=[],
            expected_state={"state": "interact"},
        )

    def test_reading_empty_json_during_completion(self):
        """Test receiving an empty JSON object when getting tab completions."""
        incoming = [
            ("server", {"prompt": ">>> ", "state": "interact"}),
            (
                "user",
                {
                    "prompt": ">>> ",
                    "completion_request": {
                        "line": "xy",
                        "begidx": 0,
                        "endidx": 2,
                    },
                    "input": "xyz",
                },
            ),
            ("server", {}),
            ("user", {"prompt": ">>> ", "input": "xyz"}),
        ]
        self.do_test(
            incoming=incoming,
            expected_outgoing=[
                {
                    "complete": {
                        "text": "xy",
                        "line": "xy",
                        "begidx": 0,
                        "endidx": 2,
                    }
                },
                {"reply": "xyz"},
            ],
            expected_stdout=(
                "*** RuntimeError: Failed to get valid completions."
                " Got: {}\n"
            ),
            expected_completions=[],
            expected_state={"state": "interact"},
        )


class RemotePdbTestCase(unittest.TestCase):
    """Tests for the _PdbServer class."""

    def setUp(self):
        self.sockfile = MockSocketFile()
        self.pdb = _PdbServer(self.sockfile)

        # Mock some Bdb attributes that are lazily created when tracing starts
        self.pdb.botframe = None
        self.pdb.quitting = False

        # Create a frame for testing
        self.test_globals = {'a': 1, 'b': 2, '__pdb_convenience_variables': {'x': 100}}
        self.test_locals = {'c': 3, 'd': 4}

        # Create a simple test frame
        frame_info = unittest.mock.Mock()
        frame_info.f_globals = self.test_globals
        frame_info.f_locals = self.test_locals
        frame_info.f_lineno = 42
        frame_info.f_code = unittest.mock.Mock()
        frame_info.f_code.co_filename = "test_file.py"
        frame_info.f_code.co_name = "test_function"

        self.pdb.curframe = frame_info

    def test_message_and_error(self):
        """Test message and error methods send correct JSON."""
        self.pdb.message("Test message")
        self.pdb.error("Test error")

        outputs = self.sockfile.get_output()
        self.assertEqual(len(outputs), 2)
        self.assertEqual(outputs[0], {"message": "Test message\n", "type": "info"})
        self.assertEqual(outputs[1], {"message": "Test error", "type": "error"})

    def test_read_command(self):
        """Test reading commands from the socket."""
        # Add test input
        self.sockfile.add_input({"reply": "help"})

        # Read the command
        cmd = self.pdb._read_reply()
        self.assertEqual(cmd, "help")

    def test_read_command_EOF(self):
        """Test reading EOF command."""
        # Simulate socket closure
        self.pdb._write_failed = True
        with self.assertRaises(EOFError):
            self.pdb._read_reply()

    def test_completion(self):
        """Test handling completion requests."""
        # Mock completenames to return specific values
        with unittest.mock.patch.object(self.pdb, 'completenames',
                                       return_value=["continue", "clear"]):

            # Add a completion request
            self.sockfile.add_input({
                "complete": {
                    "text": "c",
                    "line": "c",
                    "begidx": 0,
                    "endidx": 1
                }
            })

            # Add a regular command to break the loop
            self.sockfile.add_input({"reply": "help"})

            # Read command - this should process the completion request first
            cmd = self.pdb._read_reply()

            # Verify completion response was sent
            outputs = self.sockfile.get_output()
            self.assertEqual(len(outputs), 1)
            self.assertEqual(outputs[0], {"completions": ["continue", "clear"]})

            # The actual command should be returned
            self.assertEqual(cmd, "help")

    def test_do_help(self):
        """Test that do_help sends the help message."""
        self.pdb.do_help("break")

        outputs = self.sockfile.get_output()
        self.assertEqual(len(outputs), 1)
        self.assertEqual(outputs[0], {"help": "break"})

    def test_interact_mode(self):
        """Test interaction mode setup and execution."""
        # First set up interact mode
        self.pdb.do_interact("")

        # Verify _interact_state is properly initialized
        self.assertIsNotNone(self.pdb._interact_state)
        self.assertIsInstance(self.pdb._interact_state, dict)

        # Test running code in interact mode
        with unittest.mock.patch.object(self.pdb, '_error_exc') as mock_error:
            self.pdb._run_in_python_repl("print('test')")
            mock_error.assert_not_called()

            # Test with syntax error
            self.pdb._run_in_python_repl("if:")
            mock_error.assert_called_once()

    def test_registering_commands(self):
        """Test registering breakpoint commands."""
        # Mock get_bpbynumber
        with unittest.mock.patch.object(self.pdb, 'get_bpbynumber'):
            # Queue up some input to send
            self.sockfile.add_input({"reply": "commands 1"})
            self.sockfile.add_input({"reply": "silent"})
            self.sockfile.add_input({"reply": "print('hi')"})
            self.sockfile.add_input({"reply": "end"})
            self.sockfile.add_input({"signal": "EOF"})

            # Run the PDB command loop
            self.pdb.cmdloop()

            outputs = self.sockfile.get_output()
            self.assertIn('command_list', outputs[0])
            self.assertEqual(outputs[1], {"prompt": "(Pdb) ", "state": "pdb"})
            self.assertEqual(outputs[2], {"prompt": "(com) ", "state": "commands"})
            self.assertEqual(outputs[3], {"prompt": "(com) ", "state": "commands"})
            self.assertEqual(outputs[4], {"prompt": "(com) ", "state": "commands"})
            self.assertEqual(outputs[5], {"prompt": "(Pdb) ", "state": "pdb"})
            self.assertEqual(outputs[6], {"message": "\n", "type": "info"})
            self.assertEqual(len(outputs), 7)

            self.assertEqual(
                self.pdb.commands[1],
                ["_pdbcmd_silence_frame_status", "print('hi')"],
            )

    def test_detach(self):
        """Test the detach method."""
        with unittest.mock.patch.object(self.sockfile, 'close') as mock_close:
            self.pdb.detach()
            mock_close.assert_called_once()
            self.assertFalse(self.pdb.quitting)

    def test_cmdloop(self):
        """Test the command loop with various commands."""
        # Mock onecmd to track command execution
        with unittest.mock.patch.object(self.pdb, 'onecmd', return_value=False) as mock_onecmd:
            # Add commands to the queue
            self.pdb.cmdqueue = ['help', 'list']

            # Add a command from the socket for when cmdqueue is empty
            self.sockfile.add_input({"reply": "next"})

            # Add a second command to break the loop
            self.sockfile.add_input({"reply": "quit"})

            # Configure onecmd to exit the loop on "quit"
            def side_effect(line):
                return line == 'quit'
            mock_onecmd.side_effect = side_effect

            # Run the command loop
            self.pdb.quitting = False # Set this by hand because we don't want to really call set_trace()
            self.pdb.cmdloop()

            # Should have processed 4 commands: 2 from cmdqueue, 2 from socket
            self.assertEqual(mock_onecmd.call_count, 4)
            mock_onecmd.assert_any_call('help')
            mock_onecmd.assert_any_call('list')
            mock_onecmd.assert_any_call('next')
            mock_onecmd.assert_any_call('quit')

            # Check if prompt was sent to client
            outputs = self.sockfile.get_output()
            prompts = [o for o in outputs if 'prompt' in o]
            self.assertEqual(len(prompts), 2)  # Should have sent 2 prompts


@requires_subprocess()
@unittest.skipIf(is_wasi, "WASI does not support TCP sockets")
class PdbConnectTestCase(unittest.TestCase):
    """Tests for the _connect mechanism using direct socket communication."""

    def setUp(self):
        # Create a server socket that will wait for the debugger to connect
        self.server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_sock.bind(('127.0.0.1', 0))  # Let OS assign port
        self.server_sock.listen(1)
        self.port = self.server_sock.getsockname()[1]

    def _create_script(self, script=None):
        # Create a file for subprocess script
        if script is None:
            script = textwrap.dedent(
                f"""
                import pdb
                import sys
                import time

                def foo():
                    x = 42
                    return bar()

                def bar():
                    return 42

                def connect_to_debugger():
                    # Create a frame to debug
                    def dummy_function():
                        x = 42
                        # Call connect to establish connection
                        # with the test server
                        frame = sys._getframe()  # Get the current frame
                        pdb._connect(
                            host='127.0.0.1',
                            port={self.port},
                            frame=frame,
                            commands="",
                            version=pdb._PdbServer.protocol_version(),
                        )
                        return x  # This line won't be reached in debugging

                    return dummy_function()

                result = connect_to_debugger()
                foo()
                print(f"Function returned: {{result}}")
                """)

        self.script_path = TESTFN + "_connect_test.py"
        with open(self.script_path, 'w') as f:
            f.write(script)

    def tearDown(self):
        self.server_sock.close()
        try:
            unlink(self.script_path)
        except OSError:
            pass

    def _connect_and_get_client_file(self):
        """Helper to start subprocess and get connected client file."""
        # Start the subprocess that will connect to our socket
        process = subprocess.Popen(
            [sys.executable, self.script_path],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        # Accept the connection from the subprocess
        client_sock, _ = self.server_sock.accept()
        client_file = client_sock.makefile('rwb')
        self.addCleanup(client_file.close)
        self.addCleanup(client_sock.close)

        return process, client_file

    def _read_until_prompt(self, client_file):
        """Helper to read messages until a prompt is received."""
        messages = []
        while True:
            data = client_file.readline()
            if not data:
                break
            msg = json.loads(data.decode())
            messages.append(msg)
            if 'prompt' in msg:
                break
        return messages

    def _send_command(self, client_file, command):
        """Helper to send a command to the debugger."""
        client_file.write(json.dumps({"reply": command}).encode() + b"\n")
        client_file.flush()

    def _send_interrupt(self, pid):
        """Helper to send an interrupt signal to the debugger."""
        # with tempfile.NamedTemporaryFile("w", delete_on_close=False) as interrupt_script:
        interrupt_script = TESTFN + "_interrupt_script.py"
        with open(interrupt_script, 'w') as f:
            f.write(
                'import pdb, sys\n'
                'print("Hello, world!")\n'
                'if inst := pdb.Pdb._last_pdb_instance:\n'
                '    inst.set_trace(sys._getframe(1))\n'
            )
        self.addCleanup(unlink, interrupt_script)
        try:
            sys.remote_exec(pid, interrupt_script)
        except PermissionError:
            self.skipTest("Insufficient permissions to execute code in remote process")

    def test_connect_and_basic_commands(self):
        """Test connecting to a remote debugger and sending basic commands."""
        self._create_script()
        process, client_file = self._connect_and_get_client_file()

        with kill_on_error(process):
            # We should receive initial data from the debugger
            data = client_file.readline()
            initial_data = json.loads(data.decode())
            self.assertIn('message', initial_data)
            self.assertIn('pdb._connect', initial_data['message'])

            # First, look for command_list message
            data = client_file.readline()
            command_list = json.loads(data.decode())
            self.assertIn('command_list', command_list)

            # Then, look for the first prompt
            data = client_file.readline()
            prompt_data = json.loads(data.decode())
            self.assertIn('prompt', prompt_data)
            self.assertEqual(prompt_data['state'], 'pdb')

            # Send 'bt' (backtrace) command
            self._send_command(client_file, "bt")

            # Check for response - we should get some stack frames
            messages = self._read_until_prompt(client_file)

            # Extract text messages containing stack info
            text_msg = [msg['message'] for msg in messages
                    if 'message' in msg and 'connect_to_debugger' in msg['message']]
            got_stack_info = bool(text_msg)

            expected_stacks = [
                "<module>",
                "connect_to_debugger",
            ]

            for stack, msg in zip(expected_stacks, text_msg, strict=True):
                self.assertIn(stack, msg)

            self.assertTrue(got_stack_info, "Should have received stack trace information")

            # Send 'c' (continue) command to let the program finish
            self._send_command(client_file, "c")

            # Wait for process to finish
            stdout, _ = process.communicate(timeout=SHORT_TIMEOUT)

            # Check if we got the expected output
            self.assertIn("Function returned: 42", stdout)
            self.assertEqual(process.returncode, 0)

    def test_breakpoints(self):
        """Test setting and hitting breakpoints."""
        self._create_script()
        process, client_file = self._connect_and_get_client_file()
        with kill_on_error(process):
            # Skip initial messages until we get to the prompt
            self._read_until_prompt(client_file)

            # Set a breakpoint at the return statement
            self._send_command(client_file, "break bar")
            messages = self._read_until_prompt(client_file)
            bp_msg = next(msg['message'] for msg in messages if 'message' in msg)
            self.assertIn("Breakpoint", bp_msg)

            # Continue execution until breakpoint
            self._send_command(client_file, "c")
            messages = self._read_until_prompt(client_file)

            # Verify we hit the breakpoint
            hit_msg = next(msg['message'] for msg in messages if 'message' in msg)
            self.assertIn("bar()", hit_msg)

            # Check breakpoint list
            self._send_command(client_file, "b")
            messages = self._read_until_prompt(client_file)
            list_msg = next(msg['message'] for msg in reversed(messages) if 'message' in msg)
            self.assertIn("1   breakpoint", list_msg)
            self.assertIn("breakpoint already hit 1 time", list_msg)

            # Clear breakpoint
            self._send_command(client_file, "clear 1")
            messages = self._read_until_prompt(client_file)
            clear_msg = next(msg['message'] for msg in reversed(messages) if 'message' in msg)
            self.assertIn("Deleted breakpoint", clear_msg)

            # Continue to end
            self._send_command(client_file, "c")
            stdout, _ = process.communicate(timeout=SHORT_TIMEOUT)

            self.assertIn("Function returned: 42", stdout)
            self.assertEqual(process.returncode, 0)

    def test_keyboard_interrupt(self):
        """Test that sending keyboard interrupt breaks into pdb."""

        script = textwrap.dedent(f"""
            import time
            import sys
            import socket
            import pdb
            def bar():
                frame = sys._getframe()  # Get the current frame
                pdb._connect(
                    host='127.0.0.1',
                    port={self.port},
                    frame=frame,
                    commands="",
                    version=pdb._PdbServer.protocol_version(),
                )
                print("Connected to debugger")
                iterations = 50
                while iterations > 0:
                    print("Iteration", iterations, flush=True)
                    time.sleep(0.2)
                    iterations -= 1
                return 42

            if __name__ == "__main__":
                print("Function returned:", bar())
            """)
        self._create_script(script=script)
        process, client_file = self._connect_and_get_client_file()

        with kill_on_error(process):
            # Skip initial messages until we get to the prompt
            self._read_until_prompt(client_file)

            # Continue execution
            self._send_command(client_file, "c")

            # Confirm that the remote is already in the while loop. We know
            # it's in bar() and we can exit the loop immediately by setting
            # iterations to 0.
            while line := process.stdout.readline():
                if line.startswith("Iteration"):
                    break

            # Inject a script to interrupt the running process
            self._send_interrupt(process.pid)
            messages = self._read_until_prompt(client_file)

            # Verify we got the keyboard interrupt message.
            interrupt_msgs = [msg['message'] for msg in messages if 'message' in msg]
            expected_msg = [msg for msg in interrupt_msgs if "bar()" in msg]
            self.assertGreater(len(expected_msg), 0)

            # Continue to end as fast as we can
            self._send_command(client_file, "iterations = 0")
            self._send_command(client_file, "c")
            stdout, _ = process.communicate(timeout=SHORT_TIMEOUT)
            self.assertIn("Function returned: 42", stdout)
            self.assertEqual(process.returncode, 0)

    def test_handle_eof(self):
        """Test that EOF signal properly exits the debugger."""
        self._create_script()
        process, client_file = self._connect_and_get_client_file()

        with kill_on_error(process):
            # Skip initial messages until we get to the prompt
            self._read_until_prompt(client_file)

            # Send EOF signal to exit the debugger
            client_file.write(json.dumps({"signal": "EOF"}).encode() + b"\n")
            client_file.flush()

            # The process should complete normally after receiving EOF
            stdout, stderr = process.communicate(timeout=SHORT_TIMEOUT)

            # Verify process completed correctly
            self.assertIn("Function returned: 42", stdout)
            self.assertEqual(process.returncode, 0)
            self.assertEqual(stderr, "")

    def test_protocol_version(self):
        """Test that incompatible protocol versions are properly detected."""
        # Create a script using an incompatible protocol version
        script = textwrap.dedent(f'''
            import sys
            import pdb

            def run_test():
                frame = sys._getframe()

                # Use a fake version number that's definitely incompatible
                fake_version = 0x01010101 # A fake version that doesn't match any real Python version

                # Connect with the wrong version
                pdb._connect(
                    host='127.0.0.1',
                    port={self.port},
                    frame=frame,
                    commands="",
                    version=fake_version,
                )

                # This should print if the debugger detaches correctly
                print("Debugger properly detected version mismatch")
                return True

            if __name__ == "__main__":
                print("Test result:", run_test())
            ''')
        self._create_script(script=script)
        process, client_file = self._connect_and_get_client_file()

        with kill_on_error(process):
            # First message should be an error about protocol version mismatch
            data = client_file.readline()
            message = json.loads(data.decode())

            self.assertIn('message', message)
            self.assertEqual(message['type'], 'error')
            self.assertIn('incompatible', message['message'])
            self.assertIn('protocol version', message['message'])

            # The process should complete normally
            stdout, stderr = process.communicate(timeout=SHORT_TIMEOUT)

            # Verify the process completed successfully
            self.assertIn("Test result: True", stdout)
            self.assertIn("Debugger properly detected version mismatch", stdout)
            self.assertEqual(process.returncode, 0)

    def test_help_system(self):
        """Test that the help system properly sends help text to the client."""
        self._create_script()
        process, client_file = self._connect_and_get_client_file()

        with kill_on_error(process):
            # Skip initial messages until we get to the prompt
            self._read_until_prompt(client_file)

            # Request help for different commands
            help_commands = ["help", "help break", "help continue", "help pdb"]

            for cmd in help_commands:
                self._send_command(client_file, cmd)

                # Look for help message
                data = client_file.readline()
                message = json.loads(data.decode())

                self.assertIn('help', message)

                if cmd == "help":
                    # Should just contain the command itself
                    self.assertEqual(message['help'], "")
                else:
                    # Should contain the specific command we asked for help with
                    command = cmd.split()[1]
                    self.assertEqual(message['help'], command)

                # Skip to the next prompt
                self._read_until_prompt(client_file)

            # Continue execution to finish the program
            self._send_command(client_file, "c")

            stdout, stderr = process.communicate(timeout=SHORT_TIMEOUT)
            self.assertIn("Function returned: 42", stdout)
            self.assertEqual(process.returncode, 0)

    def test_multi_line_commands(self):
        """Test that multi-line commands work properly over remote connection."""
        self._create_script()
        process, client_file = self._connect_and_get_client_file()

        with kill_on_error(process):
            # Skip initial messages until we get to the prompt
            self._read_until_prompt(client_file)

            # Send a multi-line command
            multi_line_commands = [
                # Define a function
                "def test_func():\n    return 42",

                # For loop
                "for i in range(3):\n    print(i)",

                # If statement
                "if True:\n    x = 42\nelse:\n    x = 0",

                # Try/except
                "try:\n    result = 10/2\n    print(result)\nexcept ZeroDivisionError:\n    print('Error')",

                # Class definition
                "class TestClass:\n    def __init__(self):\n        self.value = 100\n    def get_value(self):\n        return self.value"
            ]

            for cmd in multi_line_commands:
                self._send_command(client_file, cmd)
                self._read_until_prompt(client_file)

            # Test executing the defined function
            self._send_command(client_file, "test_func()")
            messages = self._read_until_prompt(client_file)

            # Find the result message
            result_msg = next(msg['message'] for msg in messages if 'message' in msg)
            self.assertIn("42", result_msg)

            # Test creating an instance of the defined class
            self._send_command(client_file, "obj = TestClass()")
            self._read_until_prompt(client_file)

            # Test calling a method on the instance
            self._send_command(client_file, "obj.get_value()")
            messages = self._read_until_prompt(client_file)

            # Find the result message
            result_msg = next(msg['message'] for msg in messages if 'message' in msg)
            self.assertIn("100", result_msg)

            # Continue execution to finish
            self._send_command(client_file, "c")

            stdout, stderr = process.communicate(timeout=SHORT_TIMEOUT)
            self.assertIn("Function returned: 42", stdout)
            self.assertEqual(process.returncode, 0)

if __name__ == "__main__":
    unittest.main()
