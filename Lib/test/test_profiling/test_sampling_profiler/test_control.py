"""Tests for the sampling profiler control socket."""

import io
import os
import socket
import time
import unittest
from unittest import mock

from test.support import SHORT_TIMEOUT, os_helper, socket_helper

try:
    from profiling.sampling._control import (
        ControlServer,
        _MAX_INBUF_BYTES,
        parse_control_uri,
    )
    from profiling.sampling.cli import LiveStatsCollector, main
    from profiling.sampling.errors import ControlError, ControlURIError
except ImportError:
    raise unittest.SkipTest(
        "Test only runs when profiling.sampling is available"
    )


@socket_helper.skip_unless_bind_unix_socket
class ControlServerTests(unittest.TestCase):
    """Tests for ControlServer protocol, lifecycle and CLI integration."""

    def setUp(self):
        self.path = socket_helper.create_unix_domain_name()
        self.addCleanup(os_helper.unlink, self.path)

    def start_server(self):
        server = ControlServer(f"unix:{self.path}")
        server.start()
        self.addCleanup(server.stop)
        return server

    def connect(self):
        client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        client.connect(self.path)
        client.setblocking(False)
        self.addCleanup(client.close)
        return client

    def request(self, server, client, command):
        client.sendall(command)
        deadline = time.monotonic() + SHORT_TIMEOUT
        while time.monotonic() < deadline:
            server.poll(timeout=0.05)
            try:
                return client.recv(4096)
            except BlockingIOError:
                pass
        self.fail("timed out waiting for control reply")

    def test_parse_control_uri_valid_unix(self):
        """parse_control_uri returns (scheme, path) for unix: URIs."""
        self.assertEqual(
            parse_control_uri("unix:/tmp/tachyon.sock"),
            ("unix", "/tmp/tachyon.sock"),
        )

    def test_parse_control_uri_rejects_invalids(self):
        """parse_control_uri raises ControlURIError on malformed URIs."""
        cases = [
            ("/tmp/x", "must include a scheme"),
            ("unix:", "path must not be empty"),
            ("fifo:/tmp/x", "unsupported control URI scheme"),
            ("", "must include a scheme"),
        ]
        for uri, expected in cases:
            with self.subTest(uri=uri):
                with self.assertRaisesRegex(ControlURIError, expected):
                    parse_control_uri(uri)

    def test_start_creates_and_stop_unlinks_socket(self):
        """start() binds the socket on disk; stop() removes it."""
        server = ControlServer(f"unix:{self.path}")
        server.start()
        try:
            self.assertTrue(os.path.exists(self.path))
        finally:
            server.stop()
        self.assertFalse(os.path.exists(self.path))

    def test_start_fails_on_occupied_path(self):
        """start() raises ControlError when the path is already bound."""
        squatter = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        socket_helper.bind_unix_socket(squatter, self.path)
        self.addCleanup(squatter.close)
        with self.assertRaisesRegex(ControlError, "failed to start"):
            ControlServer(f"unix:{self.path}").start()

    def test_dispatch_basic_commands(self):
        """enable/disable/ping/unknown produce the documented replies."""
        server = self.start_server()
        client = self.connect()
        cases = [
            (b"enable\n", b"ok\n", True),
            (b"disable\n", b"ok\n", False),
            (b"ping\n", b"ok\n", False),
            (b"pingu nut nut\n", b"err unknown_command\n", False),
        ]
        for command, reply, expected_enabled in cases:
            with self.subTest(command=command):
                self.assertEqual(self.request(server, client, command), reply)
                if command in (b"enable\n", b"disable\n"):
                    self.assertEqual(server.control.enabled, expected_enabled)

    def test_dispatch_status_format(self):
        """status reply exposes the enabled flag."""
        server = self.start_server()
        client = self.connect()
        self.assertEqual(
            self.request(server, client, b"status\n"),
            b"ok enabled=True\n",
        )

    def test_dispatch_quit_sets_running_and_closes(self):
        """quit replies ok, sets running=False, and closes the connection."""
        server = self.start_server()
        client = self.connect()
        self.assertEqual(self.request(server, client, b"quit\n"), b"ok\n")
        self.assertFalse(server.control.running)
        deadline = time.monotonic() + SHORT_TIMEOUT
        while time.monotonic() < deadline:
            server.poll(timeout=0.05)
            try:
                chunk = client.recv(4096)
            except BlockingIOError:
                continue
            self.assertEqual(chunk, b"")
            return
        self.fail("server did not close the connection after quit")

    def test_inbuf_overflow_drops_connection(self):
        """A client filling the input buffer without a newline is dropped."""
        server = self.start_server()
        client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        client.connect(self.path)
        self.addCleanup(client.close)
        client.sendall(b"x" * (_MAX_INBUF_BYTES + 1))
        deadline = time.monotonic() + SHORT_TIMEOUT
        while time.monotonic() < deadline:
            server.poll(timeout=0.05)
            if not server._connections:
                return
        self.fail("server did not drop client over the inbuf cap")

    def test_close_listener_preserves_replaced_path(self):
        """stop() refuses to unlink a different file at the same path."""
        server = self.start_server()
        os.unlink(self.path)
        replacement = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        socket_helper.bind_unix_socket(replacement, self.path)
        self.addCleanup(replacement.close)
        server.stop()
        self.assertTrue(os.path.exists(self.path))

    def test_cli_rejects_control_with_subprocesses(self):
        """--control and --subprocesses are mutually exclusive."""
        argv = [
            "profiling.sampling.cli",
            "attach",
            "--subprocesses",
            "--control",
            "unix:/tmp/x.sock",
            "123",
        ]
        with (
            mock.patch("sys.argv", argv),
            mock.patch("sys.stderr", io.StringIO()) as stderr,
            self.assertRaises(SystemExit) as cm,
        ):
            main()
        self.assertEqual(cm.exception.code, 2)
        self.assertIn(
            "--control is incompatible with --subprocesses", stderr.getvalue()
        )

    def test_cli_rejects_bad_uri(self):
        """An unsupported control URI scheme is rejected during validation."""
        argv = [
            "profiling.sampling.cli",
            "attach",
            "--control",
            "fifo:/tmp/x",
            "123",
        ]
        with (
            mock.patch("sys.argv", argv),
            mock.patch("sys.stderr", io.StringIO()) as stderr,
            self.assertRaises(SystemExit) as cm,
        ):
            main()
        self.assertEqual(cm.exception.code, 2)
        self.assertIn("unsupported control URI scheme", stderr.getvalue())

    @unittest.skipUnless(LiveStatsCollector is not None,
                         "requires curses for --live")
    def test_cli_accepts_control_with_live(self):
        """--control and --live coexist after the mutex was dropped."""
        argv = [
            "profiling.sampling.cli",
            "attach",
            "--live",
            "--control",
            "unix:/tmp/ignored.sock",
            "123",
        ]
        with (
            mock.patch("sys.argv", argv),
            mock.patch("sys.stderr", io.StringIO()) as stderr,
            mock.patch(
                "profiling.sampling.cli._is_process_running", return_value=True
            ),
            mock.patch("profiling.sampling.cli._handle_live_attach") as live,
        ):
            main()
        self.assertNotIn("incompatible with --live", stderr.getvalue())
        live.assert_called_once()
        (forwarded_args, forwarded_pid), _ = live.call_args
        self.assertEqual(forwarded_args.control, "unix:/tmp/ignored.sock")
        self.assertTrue(forwarded_args.live)


if __name__ == "__main__":
    unittest.main()
