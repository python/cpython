import io
import json
import os
import signal
import socket
import subprocess
import sys
import tempfile
import threading
import unittest
import unittest.mock
from contextlib import contextmanager
from pathlib import Path
from test.support import os_helper
from test.support.os_helper import temp_dir, TESTFN, unlink
from typing import Dict, List, Optional, Tuple, Union, Any

import pdb
from pdb import _RemotePdb, _PdbClient, _InteractState


class MockSocketFile:
    """Mock socket file for testing _RemotePdb without actual socket connections."""

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


class RemotePdbTestCase(unittest.TestCase):
    """Tests for the _RemotePdb class."""

    def setUp(self):
        self.sockfile = MockSocketFile()
        self.pdb = _RemotePdb(self.sockfile)

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
        self.assertIsInstance(self.pdb._interact_state, _InteractState)

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


class PdbConnectTestCase(unittest.TestCase):
    """Tests for the _connect mechanism using direct socket communication."""

    def setUp(self):
        # Create a server socket that will wait for the debugger to connect
        self.server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_sock.bind(('127.0.0.1', 0))  # Let OS assign port
        self.server_sock.listen(1)
        self.port = self.server_sock.getsockname()[1]

        # Create a file for subprocess script
        self.script_path = TESTFN + "_connect_test.py"
        with open(self.script_path, 'w') as f:
            f.write(f"""
import pdb
import sys
import time

def connect_to_debugger():
    # Create a frame to debug
    def dummy_function():
        x = 42
        # Call connect to establish connection with the test server
        frame = sys._getframe()  # Get the current frame
        pdb._connect(
            host='127.0.0.1',
            port={self.port},
            frame=frame,
            commands="",
            version=pdb._RemotePdb.protocol_version(),
        )
        return x  # This line should not be reached in debugging

    return dummy_function()

result = connect_to_debugger()
print(f"Function returned: {{result}}")
""")

    def tearDown(self):
        self.server_sock.close()
        try:
            unlink(self.script_path)
        except OSError:
            pass

    def test_connect_and_basic_commands(self):
        """Test connecting to a remote debugger and sending basic commands."""
        # Start the subprocess that will connect to our socket
        with subprocess.Popen(
            [sys.executable, self.script_path],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        ) as process:
            # Accept the connection from the subprocess
            client_sock, _ = self.server_sock.accept()
            client_file = client_sock.makefile('rwb')
            self.addCleanup(client_file.close)
            self.addCleanup(client_sock.close)

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
            client_file.write(json.dumps({"reply": "bt"}).encode() + b"\n")
            client_file.flush()

            # Check for response - we should get some stack frames
            # We may get multiple messages so we need to read until we get a new prompt
            got_stack_info = False
            text_msg = []
            while True:
                data = client_file.readline()
                if not data:
                    break

                msg = json.loads(data.decode())
                if 'message' in msg and 'connect_to_debugger' in msg['message']:
                    got_stack_info = True
                    text_msg.append(msg['message'])

                if 'prompt' in msg:
                    break

            expected_stacks = [
                "<module>",
                "connect_to_debugger",
            ]

            for stack, msg in zip(expected_stacks, text_msg, strict=True):
                self.assertIn(stack, msg)

            self.assertTrue(got_stack_info, "Should have received stack trace information")

            # Send 'c' (continue) command to let the program finish
            client_file.write(json.dumps({"reply": "c"}).encode() + b"\n")
            client_file.flush()

            # Wait for process to finish
            stdout, _ = process.communicate(timeout=5)

            # Check if we got the expected output
            self.assertIn("Function returned: 42", stdout)
            self.assertEqual(process.returncode, 0)


if __name__ == "__main__":
    unittest.main()
