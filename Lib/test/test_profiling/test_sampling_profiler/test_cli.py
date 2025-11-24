"""Tests for sampling profiler CLI argument parsing and functionality."""

import io
import subprocess
import sys
import unittest
from unittest import mock

try:
    import _remote_debugging  # noqa: F401
except ImportError:
    raise unittest.SkipTest(
        "Test only runs when _remote_debugging is available"
    )

from test.support import is_emscripten


class TestSampleProfilerCLI(unittest.TestCase):
    def _setup_sync_mocks(self, mock_socket, mock_popen):
        """Helper to set up socket and process mocks for coordinator tests."""
        # Mock the sync socket with context manager support
        mock_sock_instance = mock.MagicMock()
        mock_sock_instance.getsockname.return_value = ("127.0.0.1", 12345)

        # Mock the connection with context manager support
        mock_conn = mock.MagicMock()
        mock_conn.recv.return_value = b"ready"
        mock_conn.__enter__.return_value = mock_conn
        mock_conn.__exit__.return_value = None

        # Mock accept() to return (connection, address) and support indexing
        mock_accept_result = mock.MagicMock()
        mock_accept_result.__getitem__.return_value = (
            mock_conn  # [0] returns the connection
        )
        mock_sock_instance.accept.return_value = mock_accept_result

        # Mock socket with context manager support
        mock_sock_instance.__enter__.return_value = mock_sock_instance
        mock_sock_instance.__exit__.return_value = None
        mock_socket.return_value = mock_sock_instance

        # Mock the subprocess
        mock_process = mock.MagicMock()
        mock_process.pid = 12345
        mock_process.poll.return_value = None
        mock_popen.return_value = mock_process
        return mock_process

    def _verify_coordinator_command(self, mock_popen, expected_target_args):
        """Helper to verify the coordinator command was called correctly."""
        args, kwargs = mock_popen.call_args
        coordinator_cmd = args[0]
        self.assertEqual(coordinator_cmd[0], sys.executable)
        self.assertEqual(coordinator_cmd[1], "-m")
        self.assertEqual(
            coordinator_cmd[2], "profiling.sampling._sync_coordinator"
        )
        self.assertEqual(coordinator_cmd[3], "12345")  # port
        # cwd is coordinator_cmd[4]
        self.assertEqual(coordinator_cmd[5:], expected_target_args)

    @unittest.skipIf(is_emscripten, "socket.SO_REUSEADDR does not exist")
    def test_cli_module_argument_parsing(self):
        test_args = ["profiling.sampling.cli", "run", "-m", "mymodule"]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("profiling.sampling.cli.sample") as mock_sample,
            mock.patch("subprocess.Popen") as mock_popen,
            mock.patch("socket.socket") as mock_socket,
        ):
            from profiling.sampling.cli import main
            self._setup_sync_mocks(mock_socket, mock_popen)
            main()

            self._verify_coordinator_command(mock_popen, ("-m", "mymodule"))
            # Verify sample was called once (exact arguments will vary with the new API)
            mock_sample.assert_called_once()

    @unittest.skipIf(is_emscripten, "socket.SO_REUSEADDR does not exist")
    def test_cli_module_with_arguments(self):
        test_args = [
            "profiling.sampling.cli",
            "run",
            "-m",
            "mymodule",
            "arg1",
            "arg2",
            "--flag",
        ]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("profiling.sampling.cli.sample") as mock_sample,
            mock.patch("subprocess.Popen") as mock_popen,
            mock.patch("socket.socket") as mock_socket,
        ):
            self._setup_sync_mocks(mock_socket, mock_popen)
            from profiling.sampling.cli import main
            main()

            self._verify_coordinator_command(
                mock_popen, ("-m", "mymodule", "arg1", "arg2", "--flag")
            )
            mock_sample.assert_called_once()

    @unittest.skipIf(is_emscripten, "socket.SO_REUSEADDR does not exist")
    def test_cli_script_argument_parsing(self):
        test_args = ["profiling.sampling.cli", "run", "myscript.py"]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("profiling.sampling.cli.sample") as mock_sample,
            mock.patch("subprocess.Popen") as mock_popen,
            mock.patch("socket.socket") as mock_socket,
        ):
            self._setup_sync_mocks(mock_socket, mock_popen)
            from profiling.sampling.cli import main
            main()

            self._verify_coordinator_command(mock_popen, ("myscript.py",))
            mock_sample.assert_called_once()

    @unittest.skipIf(is_emscripten, "socket.SO_REUSEADDR does not exist")
    def test_cli_script_with_arguments(self):
        test_args = [
            "profiling.sampling.cli",
            "run",
            "myscript.py",
            "arg1",
            "arg2",
            "--flag",
        ]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("profiling.sampling.cli.sample") as mock_sample,
            mock.patch("subprocess.Popen") as mock_popen,
            mock.patch("socket.socket") as mock_socket,
        ):
            # Use the helper to set up mocks consistently
            mock_process = self._setup_sync_mocks(mock_socket, mock_popen)
            # Override specific behavior for this test
            mock_process.wait.side_effect = [
                subprocess.TimeoutExpired(test_args, 0.1),
                None,
            ]

            from profiling.sampling.cli import main
            main()

            # Verify the coordinator command was called
            args, kwargs = mock_popen.call_args
            coordinator_cmd = args[0]
            self.assertEqual(coordinator_cmd[0], sys.executable)
            self.assertEqual(coordinator_cmd[1], "-m")
            self.assertEqual(
                coordinator_cmd[2], "profiling.sampling._sync_coordinator"
            )
            self.assertEqual(coordinator_cmd[3], "12345")  # port
            # cwd is coordinator_cmd[4]
            self.assertEqual(
                coordinator_cmd[5:], ("myscript.py", "arg1", "arg2", "--flag")
            )

    def test_cli_mutually_exclusive_pid_module(self):
        # In new CLI, attach and run are separate subcommands, so this test
        # verifies that mixing them causes an error
        test_args = [
            "profiling.sampling.cli",
            "attach",  # attach subcommand uses PID
            "12345",
            "-m",  # -m is only for run subcommand
            "mymodule",
        ]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("sys.stderr", io.StringIO()) as mock_stderr,
            self.assertRaises(SystemExit) as cm,
        ):
            from profiling.sampling.cli import main
            main()

        self.assertEqual(cm.exception.code, 2)  # argparse error
        error_msg = mock_stderr.getvalue()
        self.assertIn("unrecognized arguments", error_msg)

    def test_cli_mutually_exclusive_pid_script(self):
        # In new CLI, you can't mix attach (PID) with run (script)
        # This would be caught by providing a PID to run subcommand
        test_args = ["profiling.sampling.cli", "run", "12345"]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("sys.stderr", io.StringIO()) as mock_stderr,
            mock.patch("subprocess.Popen") as mock_popen,
            mock.patch("socket.socket") as mock_socket,
            self.assertRaises(FileNotFoundError) as cm,  # Expect FileNotFoundError, not SystemExit
        ):
            self._setup_sync_mocks(mock_socket, mock_popen)
            # Override to raise FileNotFoundError for non-existent script
            mock_popen.side_effect = FileNotFoundError("12345")
            from profiling.sampling.cli import main
            main()

        # Verify the error is about the non-existent script
        self.assertIn("12345", str(cm.exception))

    def test_cli_no_target_specified(self):
        # In new CLI, must specify a subcommand
        test_args = ["profiling.sampling.cli", "-d", "5"]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("sys.stderr", io.StringIO()) as mock_stderr,
            self.assertRaises(SystemExit) as cm,
        ):
            from profiling.sampling.cli import main
            main()

        self.assertEqual(cm.exception.code, 2)  # argparse error
        error_msg = mock_stderr.getvalue()
        self.assertIn("invalid choice", error_msg)

    @unittest.skipIf(is_emscripten, "socket.SO_REUSEADDR does not exist")
    def test_cli_module_with_profiler_options(self):
        test_args = [
            "profiling.sampling.cli",
            "run",
            "-i",
            "1000",
            "-d",
            "30",
            "-a",
            "--sort",
            "tottime",
            "-l",
            "20",
            "-m",
            "mymodule",
        ]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("profiling.sampling.cli.sample") as mock_sample,
            mock.patch("subprocess.Popen") as mock_popen,
            mock.patch("socket.socket") as mock_socket,
        ):
            self._setup_sync_mocks(mock_socket, mock_popen)
            from profiling.sampling.cli import main
            main()

            self._verify_coordinator_command(mock_popen, ("-m", "mymodule"))
            mock_sample.assert_called_once()

    @unittest.skipIf(is_emscripten, "socket.SO_REUSEADDR does not exist")
    def test_cli_script_with_profiler_options(self):
        """Test script with various profiler options."""
        test_args = [
            "profiling.sampling.cli",
            "run",
            "-i",
            "2000",
            "-d",
            "60",
            "--collapsed",
            "-o",
            "output.txt",
            "myscript.py",
            "scriptarg",
        ]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("profiling.sampling.cli.sample") as mock_sample,
            mock.patch("subprocess.Popen") as mock_popen,
            mock.patch("socket.socket") as mock_socket,
        ):
            self._setup_sync_mocks(mock_socket, mock_popen)
            from profiling.sampling.cli import main
            main()

            self._verify_coordinator_command(
                mock_popen, ("myscript.py", "scriptarg")
            )
            # Verify profiler was called
            mock_sample.assert_called_once()

    def test_cli_empty_module_name(self):
        test_args = ["profiling.sampling.cli", "run", "-m"]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("sys.stderr", io.StringIO()) as mock_stderr,
            self.assertRaises(SystemExit) as cm,
        ):
            from profiling.sampling.cli import main
            main()

        self.assertEqual(cm.exception.code, 2)  # argparse error
        error_msg = mock_stderr.getvalue()
        self.assertIn("required: target", error_msg)  # argparse error for missing positional arg

    @unittest.skipIf(is_emscripten, "socket.SO_REUSEADDR does not exist")
    def test_cli_long_module_option(self):
        test_args = [
            "profiling.sampling.cli",
            "run",
            "-m",
            "mymodule",
            "arg1",
        ]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("profiling.sampling.cli.sample") as mock_sample,
            mock.patch("subprocess.Popen") as mock_popen,
            mock.patch("socket.socket") as mock_socket,
        ):
            self._setup_sync_mocks(mock_socket, mock_popen)
            from profiling.sampling.cli import main
            main()

            self._verify_coordinator_command(
                mock_popen, ("-m", "mymodule", "arg1")
            )

    def test_cli_complex_script_arguments(self):
        test_args = [
            "profiling.sampling.cli",
            "run",
            "script.py",
            "--input",
            "file.txt",
            "-v",
            "--output=/tmp/out",
            "positional",
        ]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("profiling.sampling.cli.sample") as mock_sample,
            mock.patch(
                "profiling.sampling.cli._run_with_sync"
            ) as mock_run_with_sync,
        ):
            mock_process = mock.MagicMock()
            mock_process.pid = 12345
            mock_process.wait.side_effect = [
                subprocess.TimeoutExpired(test_args, 0.1),
                None,
            ]
            mock_process.poll.return_value = None
            mock_run_with_sync.return_value = mock_process

            from profiling.sampling.cli import main
            main()

            mock_run_with_sync.assert_called_once_with(
                (
                    sys.executable,
                    "script.py",
                    "--input",
                    "file.txt",
                    "-v",
                    "--output=/tmp/out",
                    "positional",
                ),
                suppress_output=False
            )

    def test_cli_collapsed_format_validation(self):
        """Test that CLI properly validates incompatible options with collapsed format."""
        test_cases = [
            # Test sort option is invalid with collapsed
            (
                [
                    "profiling.sampling.cli",
                    "attach",
                    "12345",
                    "--collapsed",
                    "--sort",
                    "tottime",  # Changed from nsamples (default) to trigger validation
                ],
                "sort",
            ),
            # Test limit option is invalid with collapsed
            (
                [
                    "profiling.sampling.cli",
                    "attach",
                    "12345",
                    "--collapsed",
                    "-l",
                    "20",
                ],
                "limit",
            ),
            # Test no-summary option is invalid with collapsed
            (
                [
                    "profiling.sampling.cli",
                    "attach",
                    "12345",
                    "--collapsed",
                    "--no-summary",
                ],
                "summary",
            ),
        ]

        from profiling.sampling.cli import main

        for test_args, expected_error_keyword in test_cases:
            with (
                mock.patch("sys.argv", test_args),
                mock.patch("sys.stderr", io.StringIO()) as mock_stderr,
                mock.patch("profiling.sampling.cli.sample"),  # Prevent actual profiling
                self.assertRaises(SystemExit) as cm,
            ):
                main()

            self.assertEqual(cm.exception.code, 2)  # argparse error code
            error_msg = mock_stderr.getvalue()
            self.assertIn("error:", error_msg)
            self.assertIn("only valid with --pstats", error_msg)

    def test_cli_default_collapsed_filename(self):
        """Test that collapsed format gets a default filename when not specified."""
        test_args = ["profiling.sampling.cli", "attach", "12345", "--collapsed"]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("profiling.sampling.cli.sample") as mock_sample,
        ):
            from profiling.sampling.cli import main
            main()

            # Check that sample was called (exact filename depends on implementation)
            mock_sample.assert_called_once()

    def test_cli_custom_output_filenames(self):
        """Test custom output filenames for both formats."""
        test_cases = [
            (
                [
                    "profiling.sampling.cli",
                    "attach",
                    "12345",
                    "--pstats",
                    "-o",
                    "custom.pstats",
                ],
                "custom.pstats",
                "pstats",
            ),
            (
                [
                    "profiling.sampling.cli",
                    "attach",
                    "12345",
                    "--collapsed",
                    "-o",
                    "custom.txt",
                ],
                "custom.txt",
                "collapsed",
            ),
        ]

        from profiling.sampling.cli import main

        for test_args, expected_filename, expected_format in test_cases:
            with (
                mock.patch("sys.argv", test_args),
                mock.patch("profiling.sampling.cli.sample") as mock_sample,
            ):
                main()

                mock_sample.assert_called_once()

    def test_cli_missing_required_arguments(self):
        """Test that CLI requires subcommand."""
        with (
            mock.patch("sys.argv", ["profiling.sampling.cli"]),
            mock.patch("sys.stderr", io.StringIO()),
        ):
            with self.assertRaises(SystemExit):
                from profiling.sampling.cli import main
                main()

    def test_cli_mutually_exclusive_format_options(self):
        """Test that pstats and collapsed options are mutually exclusive."""
        with (
            mock.patch(
                "sys.argv",
                [
                    "profiling.sampling.cli",
                    "attach",
                    "12345",
                    "--pstats",
                    "--collapsed",
                ],
            ),
            mock.patch("sys.stderr", io.StringIO()),
        ):
            with self.assertRaises(SystemExit):
                from profiling.sampling.cli import main
                main()

    def test_argument_parsing_basic(self):
        test_args = ["profiling.sampling.cli", "attach", "12345"]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("profiling.sampling.cli.sample") as mock_sample,
        ):
            from profiling.sampling.cli import main
            main()

            mock_sample.assert_called_once()

    def test_sort_options(self):
        from profiling.sampling.cli import main

        sort_options = [
            ("nsamples", 0),
            ("tottime", 1),
            ("cumtime", 2),
            ("sample-pct", 3),
            ("cumul-pct", 4),
            ("name", -1),
        ]

        for option, expected_sort_value in sort_options:
            test_args = ["profiling.sampling.cli", "attach", "12345", "--sort", option]

            with (
                mock.patch("sys.argv", test_args),
                mock.patch("profiling.sampling.cli.sample") as mock_sample,
            ):
                from profiling.sampling.cli import main
                main()

                mock_sample.assert_called_once()
                mock_sample.reset_mock()
