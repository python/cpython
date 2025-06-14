"""
Tests for pickle/unpickle of exception types with timestamp feature.
"""

import unittest
from test.support import script_helper
from .shared_utils import get_builtin_exception_types, PICKLE_TEST_SCRIPT


class ExceptionPickleTests(unittest.TestCase):
    """Test that exception types can be pickled and unpickled with timestamps intact."""

    def test_builtin_exception_pickle_without_timestamps(self):
        """Test that all built-in exception types can be pickled without timestamps."""
        exception_types = get_builtin_exception_types()

        for exc_name in exception_types:
            with self.subTest(exception_type=exc_name):
                result = script_helper.assert_python_ok(
                    "-c", PICKLE_TEST_SCRIPT, exc_name
                )

                import json

                output = json.loads(result.out.decode())

                self.assertNotIn(
                    "error",
                    output,
                    f"Error pickling {exc_name}: {output.get('error', 'Unknown')}",
                )
                self.assertEqual(output["exception_type"], exc_name)
                self.assertTrue(output["has_custom_attr"])
                self.assertEqual(output["custom_attr_value"], "custom_value")
                self.assertFalse(output["has_timestamp"])

    def test_builtin_exception_pickle_with_timestamps(self):
        """Test that all built-in exception types can be pickled with timestamps."""
        exception_types = get_builtin_exception_types()

        for exc_name in exception_types:
            with self.subTest(exception_type=exc_name):
                result = script_helper.assert_python_ok(
                    "-X",
                    "traceback_timestamps=us",
                    "-c",
                    PICKLE_TEST_SCRIPT,
                    exc_name,
                    "with_timestamps",
                )

                import json

                output = json.loads(result.out.decode())

                self.assertNotIn(
                    "error",
                    output,
                    f"Error pickling {exc_name}: {output.get('error', 'Unknown')}",
                )
                self.assertEqual(output["exception_type"], exc_name)
                self.assertTrue(output["has_custom_attr"])
                self.assertEqual(output["custom_attr_value"], "custom_value")
                self.assertTrue(output["has_timestamp"])
                self.assertEqual(
                    output["timestamp_value"], 1234567890123456789
                )


if __name__ == "__main__":
    unittest.main()
