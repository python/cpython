"""
Tests to verify timestamp presence on exception types.
"""

import json
import unittest
from test.support import script_helper
from .shared_utils import get_builtin_exception_types, TIMESTAMP_TEST_SCRIPT


class TimestampPresenceTests(unittest.TestCase):
    """Test that timestamps show up when enabled on all exception types except StopIteration."""

    def test_timestamp_presence_when_enabled(self):
        """Test that timestamps are present on exceptions when feature is enabled."""
        exception_types = get_builtin_exception_types()
        skip_types = {"BaseException", "Exception", "Warning", "GeneratorExit"}
        exception_types = [
            exc for exc in exception_types if exc not in skip_types
        ]

        for exc_name in exception_types:
            with self.subTest(exception_type=exc_name):
                result = script_helper.assert_python_ok(
                    "-X",
                    "traceback_timestamps=us",
                    "-c",
                    TIMESTAMP_TEST_SCRIPT,
                    exc_name,
                )

                output = json.loads(result.out.decode())

                self.assertNotIn(
                    "error",
                    output,
                    f"Error testing {exc_name}: {output.get('error', 'Unknown')}",
                )
                self.assertEqual(output["exception_type"], exc_name)

                if exc_name in ("StopIteration", "StopAsyncIteration"):
                    self.assertFalse(
                        output["has_timestamp_attr"]
                        and output["timestamp_is_positive"],
                        f"{exc_name} should not have timestamp for performance reasons",
                    )
                    self.assertFalse(
                        output["traceback_has_timestamp"],
                        f"{exc_name} traceback should not show timestamp",
                    )
                else:
                    self.assertTrue(
                        output["has_timestamp_attr"],
                        f"{exc_name} should have __timestamp_ns__ attribute",
                    )
                    self.assertTrue(
                        output["timestamp_is_positive"],
                        f"{exc_name} should have positive timestamp value",
                    )
                    self.assertTrue(
                        output["traceback_has_timestamp"],
                        f"{exc_name} traceback should show timestamp",
                    )

    def test_no_timestamp_when_disabled(self):
        """Test that no timestamps are present when feature is disabled."""
        test_exceptions = [
            "ValueError",
            "TypeError",
            "RuntimeError",
            "KeyError",
        ]

        for exc_name in test_exceptions:
            with self.subTest(exception_type=exc_name):
                result = script_helper.assert_python_ok(
                    "-c", TIMESTAMP_TEST_SCRIPT, exc_name
                )

                output = json.loads(result.out.decode())

                self.assertNotIn("error", output)
                self.assertFalse(
                    output["has_timestamp_attr"]
                    and output["timestamp_is_positive"],
                    f"{exc_name} should not have timestamp when disabled",
                )
                self.assertFalse(
                    output["traceback_has_timestamp"],
                    f"{exc_name} traceback should not show timestamp when disabled",
                )

    def test_timestamp_formats(self):
        """Test that different timestamp formats work correctly."""
        formats = ["us", "ns", "iso"]

        for format_type in formats:
            with self.subTest(format=format_type):
                result = script_helper.assert_python_ok(
                    "-X",
                    f"traceback_timestamps={format_type}",
                    "-c",
                    TIMESTAMP_TEST_SCRIPT,
                    "ValueError",
                )

                output = json.loads(result.out.decode())

                self.assertNotIn("error", output)
                self.assertTrue(output["has_timestamp_attr"])
                self.assertTrue(output["timestamp_is_positive"])
                self.assertTrue(output["traceback_has_timestamp"])

                traceback_output = output["traceback_output"]
                if format_type == "us":
                    self.assertRegex(traceback_output, r"<@\d+\.\d{6}>")
                elif format_type == "ns":
                    self.assertRegex(traceback_output, r"<@\d+ns>")
                elif format_type == "iso":
                    self.assertRegex(
                        traceback_output,
                        r"<@\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{6}Z>",
                    )


if __name__ == "__main__":
    unittest.main()
