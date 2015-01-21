"""Unittests for test.script_helper.  Who tests the test helper?"""

from test import script_helper
import unittest


class TestScriptHelper(unittest.TestCase):
    def test_assert_python_expect_success(self):
        t = script_helper._assert_python(True, '-c', 'import sys; sys.exit(0)')
        self.assertEqual(0, t[0], 'return code was not 0')

    def test_assert_python_expect_failure(self):
        # I didn't import the sys module so this child will fail.
        rc, out, err = script_helper._assert_python(False, '-c', 'sys.exit(0)')
        self.assertNotEqual(0, rc, 'return code should not be 0')

    def test_assert_python_raises_expect_success(self):
        # I didn't import the sys module so this child will fail.
        with self.assertRaises(AssertionError) as error_context:
            script_helper._assert_python(True, '-c', 'sys.exit(0)')
        error_msg = str(error_context.exception)
        self.assertIn('command line was:', error_msg)
        self.assertIn('sys.exit(0)', error_msg, msg='unexpected command line')

    def test_assert_python_raises_expect_failure(self):
        with self.assertRaises(AssertionError) as error_context:
            script_helper._assert_python(False, '-c', 'import sys; sys.exit(0)')
        error_msg = str(error_context.exception)
        self.assertIn('Process return code is 0,', error_msg)
        self.assertIn('import sys; sys.exit(0)', error_msg,
                      msg='unexpected command line.')


if __name__ == '__main__':
    unittest.main()
