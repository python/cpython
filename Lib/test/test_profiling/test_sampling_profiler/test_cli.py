"""Tests for sampling profiler CLI argument parsing and functionality."""

import io
import subprocess
import sys
import unittest
from unittest import mock

try:
    import _remote_debugging  # noqa: F401
    import profiling.sampling
    import profiling.sampling.sample
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
        test_args = ["profiling.sampling.sample", "-m", "mymodule"]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("profiling.sampling.sample.sample") as mock_sample,
            mock.patch("subprocess.Popen") as mock_popen,
            mock.patch("socket.socket") as mock_socket,
        ):
            self._setup_sync_mocks(mock_socket, mock_popen)
            profiling.sampling.sample.main()

            self._verify_coordinator_command(mock_popen, ("-m", "mymodule"))
            mock_sample.assert_called_once_with(
                12345,
                sort=2,  # default sort (sort_value from args.sort)
                sample_interval_usec=100,
                duration_sec=10,
                filename=None,
                all_threads=False,
                limit=15,
                show_summary=True,
                output_format="pstats",
                realtime_stats=False,
                mode=0,
                native=False,
                gc=True,
            )

    @unittest.skipIf(is_emscripten, "socket.SO_REUSEADDR does not exist")
    def test_cli_module_with_arguments(self):
        test_args = [
            "profiling.sampling.sample",
            "-m",
            "mymodule",
            "arg1",
            "arg2",
            "--flag",
        ]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("profiling.sampling.sample.sample") as mock_sample,
            mock.patch("subprocess.Popen") as mock_popen,
            mock.patch("socket.socket") as mock_socket,
        ):
            self._setup_sync_mocks(mock_socket, mock_popen)
            profiling.sampling.sample.main()

            self._verify_coordinator_command(
                mock_popen, ("-m", "mymodule", "arg1", "arg2", "--flag")
            )
            mock_sample.assert_called_once_with(
                12345,
                sort=2,
                sample_interval_usec=100,
                duration_sec=10,
                filename=None,
                all_threads=False,
                limit=15,
                show_summary=True,
                output_format="pstats",
                realtime_stats=False,
                mode=0,
                native=False,
                gc=True,
            )

    @unittest.skipIf(is_emscripten, "socket.SO_REUSEADDR does not exist")
    def test_cli_script_argument_parsing(self):
        test_args = ["profiling.sampling.sample", "myscript.py"]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("profiling.sampling.sample.sample") as mock_sample,
            mock.patch("subprocess.Popen") as mock_popen,
            mock.patch("socket.socket") as mock_socket,
        ):
            self._setup_sync_mocks(mock_socket, mock_popen)
            profiling.sampling.sample.main()

            self._verify_coordinator_command(mock_popen, ("myscript.py",))
            mock_sample.assert_called_once_with(
                12345,
                sort=2,
                sample_interval_usec=100,
                duration_sec=10,
                filename=None,
                all_threads=False,
                limit=15,
                show_summary=True,
                output_format="pstats",
                realtime_stats=False,
                mode=0,
                native=False,
                gc=True,
            )

    @unittest.skipIf(is_emscripten, "socket.SO_REUSEADDR does not exist")
    def test_cli_script_with_arguments(self):
        test_args = [
            "profiling.sampling.sample",
            "myscript.py",
            "arg1",
            "arg2",
            "--flag",
        ]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("profiling.sampling.sample.sample") as mock_sample,
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

            profiling.sampling.sample.main()

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
        test_args = [
            "profiling.sampling.sample",
            "-p",
            "12345",
            "-m",
            "mymodule",
        ]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("sys.stderr", io.StringIO()) as mock_stderr,
            self.assertRaises(SystemExit) as cm,
        ):
            profiling.sampling.sample.main()

        self.assertEqual(cm.exception.code, 2)  # argparse error
        error_msg = mock_stderr.getvalue()
        self.assertIn("not allowed with argument", error_msg)

    def test_cli_mutually_exclusive_pid_script(self):
        test_args = ["profiling.sampling.sample", "-p", "12345", "myscript.py"]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("sys.stderr", io.StringIO()) as mock_stderr,
            self.assertRaises(SystemExit) as cm,
        ):
            profiling.sampling.sample.main()

        self.assertEqual(cm.exception.code, 2)  # argparse error
        error_msg = mock_stderr.getvalue()
        self.assertIn("only one target type can be specified", error_msg)

    def test_cli_no_target_specified(self):
        test_args = ["profiling.sampling.sample", "-d", "5"]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("sys.stderr", io.StringIO()) as mock_stderr,
            self.assertRaises(SystemExit) as cm,
        ):
            profiling.sampling.sample.main()

        self.assertEqual(cm.exception.code, 2)  # argparse error
        error_msg = mock_stderr.getvalue()
        self.assertIn("one of the arguments", error_msg)

    @unittest.skipIf(is_emscripten, "socket.SO_REUSEADDR does not exist")
    def test_cli_module_with_profiler_options(self):
        test_args = [
            "profiling.sampling.sample",
            "-i",
            "1000",
            "-d",
            "30",
            "-a",
            "--sort-tottime",
            "-l",
            "20",
            "-m",
            "mymodule",
        ]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("profiling.sampling.sample.sample") as mock_sample,
            mock.patch("subprocess.Popen") as mock_popen,
            mock.patch("socket.socket") as mock_socket,
        ):
            self._setup_sync_mocks(mock_socket, mock_popen)
            profiling.sampling.sample.main()

            self._verify_coordinator_command(mock_popen, ("-m", "mymodule"))
            mock_sample.assert_called_once_with(
                12345,
                sort=1,  # sort-tottime
                sample_interval_usec=1000,
                duration_sec=30,
                filename=None,
                all_threads=True,
                limit=20,
                show_summary=True,
                output_format="pstats",
                realtime_stats=False,
                mode=0,
                native=False,
                gc=True,
            )

    @unittest.skipIf(is_emscripten, "socket.SO_REUSEADDR does not exist")
    def test_cli_script_with_profiler_options(self):
        """Test script with various profiler options."""
        test_args = [
            "profiling.sampling.sample",
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
            mock.patch("profiling.sampling.sample.sample") as mock_sample,
            mock.patch("subprocess.Popen") as mock_popen,
            mock.patch("socket.socket") as mock_socket,
        ):
            self._setup_sync_mocks(mock_socket, mock_popen)
            profiling.sampling.sample.main()

            self._verify_coordinator_command(
                mock_popen, ("myscript.py", "scriptarg")
            )
            # Verify profiler options were passed correctly
            mock_sample.assert_called_once_with(
                12345,
                sort=2,  # default sort
                sample_interval_usec=2000,
                duration_sec=60,
                filename="output.txt",
                all_threads=False,
                limit=15,
                show_summary=True,
                output_format="collapsed",
                realtime_stats=False,
                mode=0,
                native=False,
                gc=True,
            )

    def test_cli_empty_module_name(self):
        test_args = ["profiling.sampling.sample", "-m"]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("sys.stderr", io.StringIO()) as mock_stderr,
            self.assertRaises(SystemExit) as cm,
        ):
            profiling.sampling.sample.main()

        self.assertEqual(cm.exception.code, 2)  # argparse error
        error_msg = mock_stderr.getvalue()
        self.assertIn("argument -m/--module: expected one argument", error_msg)

    @unittest.skipIf(is_emscripten, "socket.SO_REUSEADDR does not exist")
    def test_cli_long_module_option(self):
        test_args = [
            "profiling.sampling.sample",
            "--module",
            "mymodule",
            "arg1",
        ]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("profiling.sampling.sample.sample") as mock_sample,
            mock.patch("subprocess.Popen") as mock_popen,
            mock.patch("socket.socket") as mock_socket,
        ):
            self._setup_sync_mocks(mock_socket, mock_popen)
            profiling.sampling.sample.main()

            self._verify_coordinator_command(
                mock_popen, ("-m", "mymodule", "arg1")
            )

    def test_cli_complex_script_arguments(self):
        test_args = [
            "profiling.sampling.sample",
            "script.py",
            "--input",
            "file.txt",
            "-v",
            "--output=/tmp/out",
            "positional",
        ]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("profiling.sampling.sample.sample") as mock_sample,
            mock.patch(
                "profiling.sampling.sample._run_with_sync"
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

            profiling.sampling.sample.main()

            mock_run_with_sync.assert_called_once_with(
                (
                    sys.executable,
                    "script.py",
                    "--input",
                    "file.txt",
                    "-v",
                    "--output=/tmp/out",
                    "positional",
                )
            )

    def test_cli_collapsed_format_validation(self):
        """Test that CLI properly validates incompatible options with collapsed format."""
        test_cases = [
            # Test sort options are invalid with collapsed
            (
                [
                    "profiling.sampling.sample",
                    "--collapsed",
                    "--sort-nsamples",
                    "-p",
                    "12345",
                ],
                "sort",
            ),
            (
                [
                    "profiling.sampling.sample",
                    "--collapsed",
                    "--sort-tottime",
                    "-p",
                    "12345",
                ],
                "sort",
            ),
            (
                [
                    "profiling.sampling.sample",
                    "--collapsed",
                    "--sort-cumtime",
                    "-p",
                    "12345",
                ],
                "sort",
            ),
            (
                [
                    "profiling.sampling.sample",
                    "--collapsed",
                    "--sort-sample-pct",
                    "-p",
                    "12345",
                ],
                "sort",
            ),
            (
                [
                    "profiling.sampling.sample",
                    "--collapsed",
                    "--sort-cumul-pct",
                    "-p",
                    "12345",
                ],
                "sort",
            ),
            (
                [
                    "profiling.sampling.sample",
                    "--collapsed",
                    "--sort-name",
                    "-p",
                    "12345",
                ],
                "sort",
            ),
            # Test limit option is invalid with collapsed
            (
                [
                    "profiling.sampling.sample",
                    "--collapsed",
                    "-l",
                    "20",
                    "-p",
                    "12345",
                ],
                "limit",
            ),
            (
                [
                    "profiling.sampling.sample",
                    "--collapsed",
                    "--limit",
                    "20",
                    "-p",
                    "12345",
                ],
                "limit",
            ),
            # Test no-summary option is invalid with collapsed
            (
                [
                    "profiling.sampling.sample",
                    "--collapsed",
                    "--no-summary",
                    "-p",
                    "12345",
                ],
                "summary",
            ),
        ]

        for test_args, expected_error_keyword in test_cases:
            with (
                mock.patch("sys.argv", test_args),
                mock.patch("sys.stderr", io.StringIO()) as mock_stderr,
                self.assertRaises(SystemExit) as cm,
            ):
                profiling.sampling.sample.main()

            self.assertEqual(cm.exception.code, 2)  # argparse error code
            error_msg = mock_stderr.getvalue()
            self.assertIn("error:", error_msg)
            self.assertIn("--pstats format", error_msg)

    def test_cli_default_collapsed_filename(self):
        """Test that collapsed format gets a default filename when not specified."""
        test_args = ["profiling.sampling.sample", "--collapsed", "-p", "12345"]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("profiling.sampling.sample.sample") as mock_sample,
        ):
            profiling.sampling.sample.main()

            # Check that filename was set to default collapsed format
            mock_sample.assert_called_once()
            call_args = mock_sample.call_args[1]
            self.assertEqual(call_args["output_format"], "collapsed")
            self.assertEqual(call_args["filename"], "collapsed.12345.txt")

    def test_cli_custom_output_filenames(self):
        """Test custom output filenames for both formats."""
        test_cases = [
            (
                [
                    "profiling.sampling.sample",
                    "--pstats",
                    "-o",
                    "custom.pstats",
                    "-p",
                    "12345",
                ],
                "custom.pstats",
                "pstats",
            ),
            (
                [
                    "profiling.sampling.sample",
                    "--collapsed",
                    "-o",
                    "custom.txt",
                    "-p",
                    "12345",
                ],
                "custom.txt",
                "collapsed",
            ),
        ]

        for test_args, expected_filename, expected_format in test_cases:
            with (
                mock.patch("sys.argv", test_args),
                mock.patch("profiling.sampling.sample.sample") as mock_sample,
            ):
                profiling.sampling.sample.main()

                mock_sample.assert_called_once()
                call_args = mock_sample.call_args[1]
                self.assertEqual(call_args["filename"], expected_filename)
                self.assertEqual(call_args["output_format"], expected_format)

    def test_cli_missing_required_arguments(self):
        """Test that CLI requires PID argument."""
        with (
            mock.patch("sys.argv", ["profiling.sampling.sample"]),
            mock.patch("sys.stderr", io.StringIO()),
        ):
            with self.assertRaises(SystemExit):
                profiling.sampling.sample.main()

    def test_cli_mutually_exclusive_format_options(self):
        """Test that pstats and collapsed options are mutually exclusive."""
        with (
            mock.patch(
                "sys.argv",
                [
                    "profiling.sampling.sample",
                    "--pstats",
                    "--collapsed",
                    "-p",
                    "12345",
                ],
            ),
            mock.patch("sys.stderr", io.StringIO()),
        ):
            with self.assertRaises(SystemExit):
                profiling.sampling.sample.main()

    def test_argument_parsing_basic(self):
        test_args = ["profiling.sampling.sample", "-p", "12345"]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("profiling.sampling.sample.sample") as mock_sample,
        ):
            profiling.sampling.sample.main()

            mock_sample.assert_called_once_with(
                12345,
                sample_interval_usec=100,
                duration_sec=10,
                filename=None,
                all_threads=False,
                limit=15,
                sort=2,
                show_summary=True,
                output_format="pstats",
                realtime_stats=False,
                mode=0,
                native=False,
                gc=True,
            )

    def test_sort_options(self):
        sort_options = [
            ("--sort-nsamples", 0),
            ("--sort-tottime", 1),
            ("--sort-cumtime", 2),
            ("--sort-sample-pct", 3),
            ("--sort-cumul-pct", 4),
            ("--sort-name", -1),
        ]

        for option, expected_sort_value in sort_options:
            test_args = ["profiling.sampling.sample", option, "-p", "12345"]

            with (
                mock.patch("sys.argv", test_args),
                mock.patch("profiling.sampling.sample.sample") as mock_sample,
            ):
                profiling.sampling.sample.main()

                mock_sample.assert_called_once()
                call_args = mock_sample.call_args[1]
                self.assertEqual(
                    call_args["sort"],
                    expected_sort_value,
                )
                mock_sample.reset_mock()
