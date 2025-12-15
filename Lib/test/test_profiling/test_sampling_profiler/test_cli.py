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

from test.support import is_emscripten, requires_remote_subprocess_debugging

from profiling.sampling.cli import main


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
    @requires_remote_subprocess_debugging()
    def test_cli_module_argument_parsing(self):
        test_args = ["profiling.sampling.cli", "run", "-m", "mymodule"]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("profiling.sampling.cli.sample") as mock_sample,
            mock.patch("subprocess.Popen") as mock_popen,
            mock.patch("socket.socket") as mock_socket,
            mock.patch("profiling.sampling.cli._wait_for_ready_signal"),
            mock.patch("importlib.util.find_spec", return_value=True),
        ):
            self._setup_sync_mocks(mock_socket, mock_popen)
            main()

            self._verify_coordinator_command(mock_popen, ("-m", "mymodule"))
            # Verify sample was called once (exact arguments will vary with the new API)
            mock_sample.assert_called_once()

    @unittest.skipIf(is_emscripten, "socket.SO_REUSEADDR does not exist")
    @requires_remote_subprocess_debugging()
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
            mock.patch("profiling.sampling.cli._wait_for_ready_signal"),
            mock.patch("importlib.util.find_spec", return_value=True),
        ):
            self._setup_sync_mocks(mock_socket, mock_popen)
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
            mock.patch("profiling.sampling.cli._wait_for_ready_signal"),
            mock.patch("os.path.exists", return_value=True),
        ):
            self._setup_sync_mocks(mock_socket, mock_popen)
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
            mock.patch("profiling.sampling.cli._wait_for_ready_signal"),
            mock.patch("os.path.exists", return_value=True),
        ):
            # Use the helper to set up mocks consistently
            mock_process = self._setup_sync_mocks(mock_socket, mock_popen)
            # Override specific behavior for this test
            mock_process.wait.side_effect = [
                subprocess.TimeoutExpired(test_args, 0.1),
                None,
            ]

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
            self.assertRaises(SystemExit) as cm,
        ):
            main()

        # Verify the error is about the non-existent script
        self.assertIn("12345", str(cm.exception.code))

    def test_cli_no_target_specified(self):
        # In new CLI, must specify a subcommand
        test_args = ["profiling.sampling.cli", "-d", "5"]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("sys.stderr", io.StringIO()) as mock_stderr,
            self.assertRaises(SystemExit) as cm,
        ):
            main()

        self.assertEqual(cm.exception.code, 2)  # argparse error
        error_msg = mock_stderr.getvalue()
        self.assertIn("invalid choice", error_msg)

    @unittest.skipIf(is_emscripten, "socket.SO_REUSEADDR does not exist")
    @requires_remote_subprocess_debugging()
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
            mock.patch("profiling.sampling.cli._wait_for_ready_signal"),
            mock.patch("importlib.util.find_spec", return_value=True),
        ):
            self._setup_sync_mocks(mock_socket, mock_popen)
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
            mock.patch("profiling.sampling.cli._wait_for_ready_signal"),
            mock.patch("os.path.exists", return_value=True),
        ):
            self._setup_sync_mocks(mock_socket, mock_popen)
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
            main()

        self.assertEqual(cm.exception.code, 2)  # argparse error
        error_msg = mock_stderr.getvalue()
        self.assertIn("required: target", error_msg)  # argparse error for missing positional arg

    @unittest.skipIf(is_emscripten, "socket.SO_REUSEADDR does not exist")
    @requires_remote_subprocess_debugging()
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
            mock.patch("profiling.sampling.cli._wait_for_ready_signal"),
            mock.patch("importlib.util.find_spec", return_value=True),
        ):
            self._setup_sync_mocks(mock_socket, mock_popen)
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
            mock.patch("os.path.exists", return_value=True),
        ):
            mock_process = mock.MagicMock()
            mock_process.pid = 12345
            mock_process.wait.side_effect = [
                subprocess.TimeoutExpired(test_args, 0.1),
                None,
            ]
            mock_process.poll.return_value = None
            mock_run_with_sync.return_value = mock_process

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
            mock.patch("profiling.sampling.sample._is_process_running", return_value=True),
            mock.patch("profiling.sampling.cli.sample") as mock_sample,
        ):
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

        for test_args, expected_filename, expected_format in test_cases:
            with (
                mock.patch("sys.argv", test_args),
                mock.patch("profiling.sampling.sample._is_process_running", return_value=True),
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
                main()

    def test_argument_parsing_basic(self):
        test_args = ["profiling.sampling.cli", "attach", "12345"]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("profiling.sampling.sample._is_process_running", return_value=True),
            mock.patch("profiling.sampling.cli.sample") as mock_sample,
        ):
            main()

            mock_sample.assert_called_once()

    def test_sort_options(self):
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
                mock.patch("profiling.sampling.sample._is_process_running", return_value=True),
                mock.patch("profiling.sampling.cli.sample") as mock_sample,
            ):
                main()

                mock_sample.assert_called_once()
                mock_sample.reset_mock()

    def test_async_aware_flag_defaults_to_running(self):
        """Test --async-aware flag enables async profiling with default 'running' mode."""
        test_args = ["profiling.sampling.cli", "attach", "12345", "--async-aware"]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("profiling.sampling.sample._is_process_running", return_value=True),
            mock.patch("profiling.sampling.cli.sample") as mock_sample,
        ):
            main()

            mock_sample.assert_called_once()
            # Verify async_aware was passed with default "running" mode
            call_kwargs = mock_sample.call_args[1]
            self.assertEqual(call_kwargs.get("async_aware"), "running")

    def test_async_aware_with_async_mode_all(self):
        """Test --async-aware with --async-mode all."""
        test_args = ["profiling.sampling.cli", "attach", "12345", "--async-aware", "--async-mode", "all"]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("profiling.sampling.sample._is_process_running", return_value=True),
            mock.patch("profiling.sampling.cli.sample") as mock_sample,
        ):
            main()

            mock_sample.assert_called_once()
            call_kwargs = mock_sample.call_args[1]
            self.assertEqual(call_kwargs.get("async_aware"), "all")

    def test_async_aware_default_is_none(self):
        """Test async_aware defaults to None when --async-aware not specified."""
        test_args = ["profiling.sampling.cli", "attach", "12345"]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("profiling.sampling.sample._is_process_running", return_value=True),
            mock.patch("profiling.sampling.cli.sample") as mock_sample,
        ):
            main()

            mock_sample.assert_called_once()
            call_kwargs = mock_sample.call_args[1]
            self.assertIsNone(call_kwargs.get("async_aware"))

    def test_async_mode_invalid_choice(self):
        """Test --async-mode with invalid choice raises error."""
        test_args = ["profiling.sampling.cli", "attach", "12345", "--async-aware", "--async-mode", "invalid"]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("sys.stderr", io.StringIO()),
            self.assertRaises(SystemExit) as cm,
        ):
            main()

        self.assertEqual(cm.exception.code, 2)  # argparse error

    def test_async_mode_requires_async_aware(self):
        """Test --async-mode without --async-aware raises error."""
        test_args = ["profiling.sampling.cli", "attach", "12345", "--async-mode", "all"]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("sys.stderr", io.StringIO()) as mock_stderr,
            self.assertRaises(SystemExit) as cm,
        ):
            main()

        self.assertEqual(cm.exception.code, 2)  # argparse error
        error_msg = mock_stderr.getvalue()
        self.assertIn("--async-mode requires --async-aware", error_msg)

    def test_async_aware_incompatible_with_native(self):
        """Test --async-aware is incompatible with --native."""
        test_args = ["profiling.sampling.cli", "attach", "12345", "--async-aware", "--native"]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("sys.stderr", io.StringIO()) as mock_stderr,
            self.assertRaises(SystemExit) as cm,
        ):
            main()

        self.assertEqual(cm.exception.code, 2)  # argparse error
        error_msg = mock_stderr.getvalue()
        self.assertIn("--native", error_msg)
        self.assertIn("incompatible with --async-aware", error_msg)

    def test_async_aware_incompatible_with_no_gc(self):
        """Test --async-aware is incompatible with --no-gc."""
        test_args = ["profiling.sampling.cli", "attach", "12345", "--async-aware", "--no-gc"]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("sys.stderr", io.StringIO()) as mock_stderr,
            self.assertRaises(SystemExit) as cm,
        ):
            main()

        self.assertEqual(cm.exception.code, 2)  # argparse error
        error_msg = mock_stderr.getvalue()
        self.assertIn("--no-gc", error_msg)
        self.assertIn("incompatible with --async-aware", error_msg)

    def test_async_aware_incompatible_with_both_native_and_no_gc(self):
        """Test --async-aware is incompatible with both --native and --no-gc."""
        test_args = ["profiling.sampling.cli", "attach", "12345", "--async-aware", "--native", "--no-gc"]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("sys.stderr", io.StringIO()) as mock_stderr,
            self.assertRaises(SystemExit) as cm,
        ):
            main()

        self.assertEqual(cm.exception.code, 2)  # argparse error
        error_msg = mock_stderr.getvalue()
        self.assertIn("--native", error_msg)
        self.assertIn("--no-gc", error_msg)
        self.assertIn("incompatible with --async-aware", error_msg)

    def test_async_aware_incompatible_with_mode(self):
        """Test --async-aware is incompatible with --mode (non-wall)."""
        test_args = ["profiling.sampling.cli", "attach", "12345", "--async-aware", "--mode", "cpu"]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("sys.stderr", io.StringIO()) as mock_stderr,
            self.assertRaises(SystemExit) as cm,
        ):
            main()

        self.assertEqual(cm.exception.code, 2)  # argparse error
        error_msg = mock_stderr.getvalue()
        self.assertIn("--mode=cpu", error_msg)
        self.assertIn("incompatible with --async-aware", error_msg)

    def test_async_aware_incompatible_with_all_threads(self):
        """Test --async-aware is incompatible with --all-threads."""
        test_args = ["profiling.sampling.cli", "attach", "12345", "--async-aware", "--all-threads"]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("sys.stderr", io.StringIO()) as mock_stderr,
            self.assertRaises(SystemExit) as cm,
        ):
            main()

        self.assertEqual(cm.exception.code, 2)  # argparse error
        error_msg = mock_stderr.getvalue()
        self.assertIn("--all-threads", error_msg)
        self.assertIn("incompatible with --async-aware", error_msg)

    @unittest.skipIf(is_emscripten, "subprocess not available")
    def test_run_nonexistent_script_exits_cleanly(self):
        """Test that running a non-existent script exits with a clean error."""
        with mock.patch("sys.argv", ["profiling.sampling.cli", "run", "/nonexistent/script.py"]):
            with self.assertRaises(SystemExit) as cm:
                main()
        self.assertIn("Script not found", str(cm.exception.code))

    @unittest.skipIf(is_emscripten, "subprocess not available")
    def test_run_nonexistent_module_exits_cleanly(self):
        """Test that running a non-existent module exits with a clean error."""
        with mock.patch("sys.argv", ["profiling.sampling.cli", "run", "-m", "nonexistent_module_xyz"]):
            with self.assertRaises(SystemExit) as cm:
                main()
        self.assertIn("Module not found", str(cm.exception.code))
