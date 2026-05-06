"""Tests for traceback timestamps configuration, output format, and utilities."""

import re
import textwrap
import traceback
import unittest
from traceback import TIMESTAMP_AFTER_EXC_MSG_RE_GROUP

from test.support import (
    force_not_colorized,
    no_traceback_timestamps,
    script_helper,
    swap_attr,
)


# Script that raises an exception and prints the traceback.
RAISE_SCRIPT = """\
import traceback
try:
    1/0
except Exception:
    traceback.print_exc()
"""

# Script that prints sys.flags.traceback_timestamps.
FLAGS_SCRIPT = """\
import sys
print(repr(sys.flags.traceback_timestamps))
"""


class ConfigurationTests(unittest.TestCase):
    """Test -X traceback_timestamps and PYTHON_TRACEBACK_TIMESTAMPS config."""

    def test_disabled_by_default(self):
        result = script_helper.assert_python_ok("-c", RAISE_SCRIPT)
        self.assertNotIn(b"<@", result.err)

    def test_env_var_enables(self):
        result = script_helper.assert_python_ok(
            "-c", RAISE_SCRIPT, PYTHON_TRACEBACK_TIMESTAMPS="ns"
        )
        self.assertIn(b"<@", result.err)

    def test_flag_precedence_over_env_var(self):
        result = script_helper.assert_python_ok(
            "-X", "traceback_timestamps=ns",
            "-c", FLAGS_SCRIPT,
            PYTHON_TRACEBACK_TIMESTAMPS="iso",
        )
        self.assertEqual(result.out.strip(), b"'ns'")

    def test_flag_no_value_defaults_to_ns(self):
        result = script_helper.assert_python_ok(
            "-X", "traceback_timestamps", "-c", FLAGS_SCRIPT
        )
        self.assertEqual(result.out.strip(), b"'ns'")

    def test_flag_values(self):
        """Test sys.flags reflects configured value for each valid option."""
        cases = [
            ([], {}, b"''"),                           # default
            (["-X", "traceback_timestamps=ns"], {}, b"'ns'"),
            (["-X", "traceback_timestamps=iso"], {}, b"'iso'"),
            (["-X", "traceback_timestamps=1"], {}, b"'ns'"),
            (["-X", "traceback_timestamps=0"], {}, b"''"),
            ([], {"PYTHON_TRACEBACK_TIMESTAMPS": "ns"}, b"'ns'"),
            ([], {"PYTHON_TRACEBACK_TIMESTAMPS": "1"}, b"'ns'"),
            ([], {"PYTHON_TRACEBACK_TIMESTAMPS": "0"}, b"''"),
        ]
        for args, env, expected in cases:
            with self.subTest(args=args, env=env):
                result = script_helper.assert_python_ok(
                    *args, "-c", FLAGS_SCRIPT, **env
                )
                self.assertEqual(result.out.strip(), expected)

    def test_invalid_env_var_silently_ignored(self):
        result = script_helper.assert_python_ok(
            "-c", FLAGS_SCRIPT, PYTHON_TRACEBACK_TIMESTAMPS="invalid"
        )
        self.assertEqual(result.out.strip(), b"''")

    def test_invalid_flag_errors(self):
        result = script_helper.assert_python_failure(
            "-X", "traceback_timestamps=invalid", "-c", FLAGS_SCRIPT
        )
        self.assertIn(b"Invalid -X traceback_timestamps=value option", result.err)

    def test_disabled_no_timestamps_in_output(self):
        result = script_helper.assert_python_ok(
            "-X", "traceback_timestamps=0", "-c", RAISE_SCRIPT
        )
        self.assertNotIn(b"<@", result.err)


class FormatTests(unittest.TestCase):
    """Test the timestamp output formats."""

    def test_ns_format(self):
        result = script_helper.assert_python_ok(
            "-X", "traceback_timestamps=ns", "-c", RAISE_SCRIPT
        )
        self.assertRegex(result.err, rb"<@\d+\.\d{9}>")

    def test_ns_format_exact(self):
        # Integer divmod must preserve all digits; float division would not.
        self.assertEqual(traceback._format_ns(1_776_017_178_687_320_256),
                         "<@1776017178.687320256>")
        self.assertEqual(traceback._format_ns(123), "<@0.000000123>")

    def test_iso_format_exact(self):
        self.assertEqual(traceback._format_iso(1_776_017_178_687_320_256),
                         "<@2026-04-12T18:06:18.687320Z>")

    def test_us_format_removed(self):
        result = script_helper.assert_python_failure(
            "-X", "traceback_timestamps=us", "-c", FLAGS_SCRIPT
        )
        self.assertIn(b"Invalid -X traceback_timestamps", result.err)

    def test_iso_format(self):
        result = script_helper.assert_python_ok(
            "-X", "traceback_timestamps=iso", "-c", RAISE_SCRIPT
        )
        self.assertRegex(
            result.err,
            rb"<@\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{6}Z>",
        )


class TimestampPresenceTests(unittest.TestCase):
    """Test that timestamps are collected on the right exception types."""

    PRESENCE_SCRIPT = """\
import sys, json
exc_name = sys.argv[1]
exc_class = getattr(__builtins__, exc_name, None) or getattr(
    __import__('builtins'), exc_name)
try:
    raise exc_class("test")
except BaseException as e:
    result = {
        "type": type(e).__name__,
        "ts": e.__timestamp_ns__,
    }
    print(json.dumps(result))
"""

    def test_timestamps_collected_when_enabled(self):
        for exc_name in ("ValueError", "OSError", "RuntimeError", "KeyError"):
            with self.subTest(exc_name):
                result = script_helper.assert_python_ok(
                    "-X", "traceback_timestamps=ns",
                    "-c", self.PRESENCE_SCRIPT, exc_name,
                )
                import json
                output = json.loads(result.out)
                self.assertGreater(output["ts"], 0,
                                   f"{exc_name} should have a positive timestamp")

    def test_memoryerror_freelist_stamped_on_reuse(self):
        # MemoryError instances recycled from the interpreter free list must
        # get a fresh timestamp at the time they are handed out, not retain a
        # stale one from a prior occupant of the slot nor report 0.
        script = textwrap.dedent("""
            e = MemoryError()
            old_ts = e.__timestamp_ns__
            del e
            # __new__ alone exercises the freelist path without __init__.
            e2 = MemoryError.__new__(MemoryError)
            assert e2.__timestamp_ns__ > 0, e2.__timestamp_ns__
            assert e2.__timestamp_ns__ != old_ts
            print("ok")
        """)
        result = script_helper.assert_python_ok(
            "-X", "traceback_timestamps=ns", "-c", script,
        )
        self.assertEqual(result.out.strip(), b"ok")

    def test_stop_iteration_excluded(self):
        for exc_name in ("StopIteration", "StopAsyncIteration"):
            with self.subTest(exc_name):
                result = script_helper.assert_python_ok(
                    "-X", "traceback_timestamps=ns",
                    "-c", self.PRESENCE_SCRIPT, exc_name,
                )
                import json
                output = json.loads(result.out)
                self.assertEqual(output["ts"], 0,
                                 f"{exc_name} should not have a timestamp")

    def test_no_timestamps_when_disabled(self):
        for exc_name in ("ValueError", "TypeError", "RuntimeError"):
            with self.subTest(exc_name):
                result = script_helper.assert_python_ok(
                    "-X", "traceback_timestamps=0",
                    "-c", self.PRESENCE_SCRIPT, exc_name,
                )
                import json
                output = json.loads(result.out)
                self.assertEqual(output["ts"], 0)


class StripTimestampsTests(unittest.TestCase):
    """Tests for traceback.strip_exc_timestamps()."""

    STRIP_SCRIPT = """\
import sys, traceback
# Generate a real traceback with timestamps, then strip it.
try:
    1/0
except Exception:
    raw = traceback.format_exc()

stripped = traceback.strip_exc_timestamps(raw)
# Print both separated by a marker.
sys.stdout.write(raw + "---MARKER---\\n" + stripped)
"""

    @force_not_colorized
    def test_strip_removes_timestamps(self):
        for mode in ("ns", "iso"):
            with self.subTest(mode=mode):
                result = script_helper.assert_python_ok(
                    "-X", f"traceback_timestamps={mode}",
                    "-c", self.STRIP_SCRIPT,
                )
                output = result.out.decode().replace('\r\n', '\n')
                parts = output.split("---MARKER---\n")
                raw, stripped = parts[0], parts[1]
                self.assertIn("<@", raw)
                self.assertNotIn("<@", stripped)
                self.assertIn("ZeroDivisionError: division by zero", stripped)

    @force_not_colorized
    def test_strip_noop_when_disabled(self):
        result = script_helper.assert_python_ok(
            "-X", "traceback_timestamps=0", "-c", self.STRIP_SCRIPT,
        )
        output = result.out.decode().replace('\r\n', '\n')
        parts = output.split("---MARKER---\n")
        raw, stripped = parts[0], parts[1]
        self.assertEqual(raw, stripped)

    def test_timestamp_regex_pattern(self):
        pattern = re.compile(TIMESTAMP_AFTER_EXC_MSG_RE_GROUP, re.MULTILINE)
        # Should match valid formats
        self.assertTrue(pattern.search(" <@1234567890.123456789>"))
        self.assertTrue(pattern.search(" <@0.000000001>"))
        self.assertTrue(pattern.search(" <@2023-04-13T12:34:56.789012Z>"))
        # Should not match invalid formats
        self.assertFalse(pattern.search("<@>"))
        self.assertFalse(pattern.search(" <1234567890.123456>"))
        self.assertFalse(pattern.search("<@abc>"))


class TimestampsParameterTests(unittest.TestCase):
    """Test the timestamps= tri-state parameter on the formatting APIs."""

    def setUp(self):
        try:
            raise ValueError("boom")
        except ValueError as e:
            self.exc = e
        # Guarantee a known non-zero value regardless of how the test
        # process itself was launched.
        self.exc.__timestamp_ns__ = 1_234_567_890_123_456_789

    def test_timestamps_false_suppresses(self):
        out = "".join(traceback.format_exception(self.exc, timestamps=False))
        self.assertNotIn("<@", out)

    def test_timestamps_true_forces_display(self):
        # Even with the global config off, timestamps=True must render any
        # non-zero __timestamp_ns__ using the ns format as a fallback.
        with no_traceback_timestamps():
            out = "".join(traceback.format_exception(self.exc, timestamps=True))
        self.assertIn("<@1234567890.123456789>", out)

    def test_timestamps_true_zero_value_hidden(self):
        self.exc.__timestamp_ns__ = 0
        out = "".join(traceback.format_exception(self.exc, timestamps=True))
        self.assertNotIn("<@", out)

    def test_timestamps_none_follows_global_off(self):
        with no_traceback_timestamps():
            out = "".join(traceback.format_exception(self.exc, timestamps=None))
        self.assertNotIn("<@", out)

    def test_timestamps_none_follows_global_on(self):
        with (
            swap_attr(traceback, "_TIMESTAMP_FORMAT", "ns"),
            swap_attr(traceback, "_timestamp_formatter", traceback._format_ns),
        ):
            out = "".join(traceback.format_exception(self.exc, timestamps=None))
        self.assertIn("<@1234567890.123456789>", out)

    def test_timestamps_propagates_to_cause(self):
        try:
            raise RuntimeError("outer") from self.exc
        except RuntimeError as e:
            outer = e
        outer.__timestamp_ns__ = 2_000_000_000_000_000_000
        out = "".join(traceback.format_exception(outer, timestamps=True))
        self.assertIn("<@1234567890.123456789>", out)
        self.assertIn("<@2000000000.000000000>", out)
        out = "".join(traceback.format_exception(outer, timestamps=False))
        self.assertNotIn("<@", out)

    def test_timestamps_propagates_to_context(self):
        try:
            try:
                raise self.exc
            except ValueError:
                raise RuntimeError("outer")
        except RuntimeError as e:
            outer = e
        outer.__timestamp_ns__ = 2_000_000_000_000_000_000
        out = "".join(traceback.format_exception(outer, timestamps=True))
        self.assertIn("<@1234567890.123456789>", out)
        self.assertIn("<@2000000000.000000000>", out)
        out = "".join(traceback.format_exception(outer, timestamps=False))
        self.assertNotIn("<@", out)

    def test_timestamps_propagates_to_exception_group(self):
        e1 = ValueError("a")
        e1.__timestamp_ns__ = 1_111_111_111_111_111_111
        e2 = TypeError("b")
        e2.__timestamp_ns__ = 2_222_222_222_222_222_222
        eg = ExceptionGroup("grp", [e1, e2])
        eg.__timestamp_ns__ = 3_333_333_333_333_333_333
        out = "".join(traceback.format_exception(eg, timestamps=True))
        self.assertIn("<@1111111111.111111111>", out)
        self.assertIn("<@2222222222.222222222>", out)
        self.assertIn("<@3333333333.333333333>", out)
        out = "".join(traceback.format_exception(eg, timestamps=False))
        self.assertNotIn("<@", out)


if __name__ == "__main__":
    unittest.main()
