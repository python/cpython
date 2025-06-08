import unittest
import re
from traceback import TIMESTAMP_AFTER_EXC_MSG_RE_GROUP

from test.support import force_not_colorized, script_helper
from test.support.os_helper import EnvironmentVarGuard, TESTFN, unlink


class TracebackTimestampsTests(unittest.TestCase):
    def setUp(self):
        self.script = """
import sys
import traceback

def cause_exception():
    1/0

try:
    cause_exception()
except Exception as e:
    traceback.print_exc()
"""
        self.script_path = TESTFN + ".py"
        with open(self.script_path, "w") as script_file:
            script_file.write(self.script)
        self.addCleanup(unlink, self.script_path)

        # Script to check sys.flags.traceback_timestamps value
        self.flags_script = """
import sys
print(repr(sys.flags.traceback_timestamps))
"""
        self.flags_script_path = TESTFN + "_flag.py"
        with open(self.flags_script_path, "w") as script_file:
            script_file.write(self.flags_script)
        self.addCleanup(unlink, self.flags_script_path)

        self.env = EnvironmentVarGuard()
        self.env.set("PYTHONUTF8", "1")  # -X utf8=1
        self.addCleanup(self.env.__exit__)

    def test_no_traceback_timestamps(self):
        """Test that traceback timestamps are not shown by default"""
        result = script_helper.assert_python_ok(self.script_path)
        self.assertNotIn(b"<@", result.err)  # No timestamp should be present

    def test_traceback_timestamps_env_var(self):
        """Test that PYTHON_TRACEBACK_TIMESTAMPS env var enables timestamps"""
        result = script_helper.assert_python_ok(
            self.script_path, PYTHON_TRACEBACK_TIMESTAMPS="us"
        )
        self.assertIn(b"<@", result.err)  # Timestamp should be present

    def test_traceback_timestamps_flag_us(self):
        """Test -X traceback_timestamps=us flag"""
        result = script_helper.assert_python_ok(
            "-X", "traceback_timestamps=us", self.script_path
        )
        self.assertIn(b"<@", result.err)  # Timestamp should be present

    def test_traceback_timestamps_flag_ns(self):
        """Test -X traceback_timestamps=ns flag"""
        result = script_helper.assert_python_ok(
            "-X", "traceback_timestamps=ns", self.script_path
        )
        stderr = result.err
        self.assertIn(b"<@", result.err)  # Timestamp should be present
        self.assertIn(b"ns>", result.err)  # Should have ns format

    def test_traceback_timestamps_flag_iso(self):
        """Test -X traceback_timestamps=iso flag"""
        result = script_helper.assert_python_ok(
            "-X", "traceback_timestamps=iso", self.script_path
        )
        self.assertIn(b"<@", result.err)  # Timestamp should be present
        # ISO format with Z suffix for UTC
        self.assertRegex(
            result.err, rb"<@\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{6}Z>"
        )

    def test_traceback_timestamps_flag_value(self):
        """Test that sys.flags.traceback_timestamps shows the right value"""
        # Default should be empty string
        result = script_helper.assert_python_ok(self.flags_script_path)
        stdout = result.out.strip()
        self.assertEqual(stdout, b"''")

        # With us flag
        result = script_helper.assert_python_ok(
            "-X", "traceback_timestamps=us", self.flags_script_path
        )
        stdout = result.out.strip()
        self.assertEqual(stdout, b"'us'")

        # With ns flag
        result = script_helper.assert_python_ok(
            "-X", "traceback_timestamps=ns", self.flags_script_path
        )
        stdout = result.out.strip()
        self.assertEqual(stdout, b"'ns'")

        # With iso flag
        result = script_helper.assert_python_ok(
            "-X", "traceback_timestamps=iso", self.flags_script_path
        )
        stdout = result.out.strip()
        self.assertEqual(stdout, b"'iso'")

    def test_traceback_timestamps_env_var_precedence(self):
        """Test that -X flag takes precedence over env var"""
        result = script_helper.assert_python_ok(
            "-X",
            "traceback_timestamps=us",
            "-c",
            "import sys; print(repr(sys.flags.traceback_timestamps))",
            PYTHON_TRACEBACK_TIMESTAMPS="ns",
        )
        stdout = result.out.strip()
        self.assertEqual(stdout, b"'us'")

    def test_traceback_timestamps_flag_no_value(self):
        """Test -X traceback_timestamps with no value defaults to 'us'"""
        result = script_helper.assert_python_ok(
            "-X", "traceback_timestamps", self.flags_script_path
        )
        stdout = result.out.strip()
        self.assertEqual(stdout, b"'us'")

    def test_traceback_timestamps_flag_zero(self):
        """Test -X traceback_timestamps=0 disables the feature"""
        # Check that setting to 0 results in empty string in sys.flags
        result = script_helper.assert_python_ok(
            "-X", "traceback_timestamps=0", self.flags_script_path
        )
        stdout = result.out.strip()
        self.assertEqual(stdout, b"''")

        # Check that no timestamps appear in traceback
        result = script_helper.assert_python_ok(
            "-X", "traceback_timestamps=0", self.script_path
        )
        self.assertNotIn(b"<@", result.err)  # No timestamp should be present

    def test_traceback_timestamps_flag_one(self):
        """Test -X traceback_timestamps=1 is equivalent to 'us'"""
        result = script_helper.assert_python_ok(
            "-X", "traceback_timestamps=1", self.flags_script_path
        )
        stdout = result.out.strip()
        self.assertEqual(stdout, b"'us'")

    def test_traceback_timestamps_env_var_zero(self):
        """Test PYTHON_TRACEBACK_TIMESTAMPS=0 disables the feature"""
        result = script_helper.assert_python_ok(
            self.flags_script_path, PYTHON_TRACEBACK_TIMESTAMPS="0"
        )
        stdout = result.out.strip()
        self.assertEqual(stdout, b"''")

    def test_traceback_timestamps_env_var_one(self):
        """Test PYTHON_TRACEBACK_TIMESTAMPS=1 is equivalent to 'us'"""
        result = script_helper.assert_python_ok(
            self.flags_script_path, PYTHON_TRACEBACK_TIMESTAMPS="1"
        )
        stdout = result.out.strip()
        self.assertEqual(stdout, b"'us'")

    def test_traceback_timestamps_invalid_env_var(self):
        """Test that invalid env var values are silently ignored"""
        result = script_helper.assert_python_ok(
            self.flags_script_path, PYTHON_TRACEBACK_TIMESTAMPS="invalid"
        )
        stdout = result.out.strip()
        self.assertEqual(stdout, b"''")  # Should default to empty string

    def test_traceback_timestamps_invalid_flag(self):
        """Test that invalid flag values cause an error"""
        result = script_helper.assert_python_failure(
            "-X", "traceback_timestamps=invalid", self.flags_script_path
        )
        self.assertIn(
            b"Invalid -X traceback_timestamps=value option", result.err
        )


class StripExcTimestampsTests(unittest.TestCase):
    """Tests for traceback.strip_exc_timestamps function"""

    def setUp(self):
        self.script_strip_test = r"""
import sys
import traceback

def error():
    print("FakeError: not an exception <@1234567890.123456>\n")
    3/0

def strip(data):
    print(traceback.strip_exc_timestamps(data))

if __name__ == "__main__":
    if len(sys.argv) <= 1:
        error()
    else:
        strip(sys.argv[1])
"""
        self.script_strip_path = TESTFN + "_strip.py"
        with open(self.script_strip_path, "w") as script_file:
            script_file.write(self.script_strip_test)
        self.addCleanup(unlink, self.script_strip_path)

    @force_not_colorized
    def test_strip_exc_timestamps_function(self):
        """Test the strip_exc_timestamps function with various inputs"""
        for mode in ("us", "ns", "iso"):
            with self.subTest(mode):
                result = script_helper.assert_python_failure(
                    "-X",
                    f"traceback_timestamps={mode}",
                    self.script_strip_path,
                )
                output = result.out.decode() + result.err.decode(
                    errors="ignore"
                )

                # call strip_exc_timestamps in a process using the same mode as what generated our output.
                result = script_helper.assert_python_ok(
                    "-X",
                    f"traceback_timestamps={mode}",
                    self.script_strip_path,
                    output,
                )
                stripped_output = result.out.decode() + result.err.decode(
                    errors="ignore"
                )

                # Verify original strings have timestamps and stripped ones don't
                self.assertIn("ZeroDivisionError: division by zero <@", output)
                self.assertNotRegex(
                    output, "(?m)ZeroDivisionError: division by zero(\n|\r|$)"
                )
                self.assertRegex(
                    stripped_output,
                    "(?m)ZeroDivisionError: division by zero(\r|\n|$)",
                )
                self.assertRegex(
                    stripped_output, "(?m)FakeError: not an exception(\r|\n|$)"
                )

    @force_not_colorized
    def test_strip_exc_timestamps_with_disabled_timestamps(self):
        """Test the strip_exc_timestamps function when timestamps are disabled"""
        # Run with timestamps disabled
        result = script_helper.assert_python_failure(
            "-X", "traceback_timestamps=0", self.script_strip_path
        )
        output = result.out.decode() + result.err.decode(errors="ignore")

        # call strip_exc_timestamps in a process using the same mode as what generated our output.
        result = script_helper.assert_python_ok(
            "-X", "traceback_timestamps=0", self.script_strip_path, output
        )
        stripped_output = result.out.decode() + result.err.decode(
            errors="ignore"
        )

        # All strings should be unchanged by the strip function
        self.assertRegex(
            stripped_output, "(?m)ZeroDivisionError: division by zero(\r|\n|$)"
        )
        # it fits the pattern but traceback timestamps were disabled to strip_exc_timestamps does nothing.
        self.assertRegex(
            stripped_output,
            "(?m)FakeError: not an exception <@1234567890.123456>(\r|\n|$)",
        )

    def test_timestamp_regex_pattern(self):
        """Test the regex pattern used by strip_exc_timestamps"""
        pattern = re.compile(
            TIMESTAMP_AFTER_EXC_MSG_RE_GROUP, flags=re.MULTILINE
        )

        # Test microsecond format
        self.assertTrue(pattern.search(" <@1234567890.123456>"))
        # Test nanosecond format
        self.assertTrue(pattern.search(" <@1234567890123456789ns>"))
        # Test ISO format
        self.assertTrue(pattern.search(" <@2023-04-13T12:34:56.789012Z>"))

        # Test what should not match
        self.assertFalse(pattern.search("<@>"))  # Empty timestamp
        self.assertFalse(
            pattern.search(" <1234567890.123456>")
        )  # Missing @ sign
        self.assertFalse(pattern.search("<@abc>"))  # Non-numeric timestamp


if __name__ == "__main__":
    unittest.main()
