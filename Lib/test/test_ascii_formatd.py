# PyOS_ascii_formatd is deprecated and not called from anywhere in
#  Python itself. So this module is the only place it gets tested.
# Test that it works, and test that it's deprecated.

import unittest
from test.support import check_warnings, run_unittest, cpython_only, import_module

import_module('ctypes')

class FormatDeprecationTests(unittest.TestCase):

    @cpython_only
    def testFormatDeprecation(self):
        # delay importing ctypes until we know we're in CPython
        from ctypes import (pythonapi, create_string_buffer, sizeof, byref,
                            c_double)
        PyOS_ascii_formatd = pythonapi.PyOS_ascii_formatd
        buf = create_string_buffer(100)

        with check_warnings() as w:
            PyOS_ascii_formatd(byref(buf), sizeof(buf), b'%+.10f',
                               c_double(10.0))
            self.assertEqual(buf.value, b'+10.0000000000')

        self.assertEqual(str(w.message), 'PyOS_ascii_formatd is deprecated, '
                         'use PyOS_double_to_string instead')

class FormatTests(unittest.TestCase):
    # ensure that, for the restricted set of format codes,
    # %-formatting returns the same values os PyOS_ascii_formatd
    @cpython_only
    def testFormat(self):
        # delay importing ctypes until we know we're in CPython
        from ctypes import (pythonapi, create_string_buffer, sizeof, byref,
                            c_double)
        PyOS_ascii_formatd = pythonapi.PyOS_ascii_formatd
        buf = create_string_buffer(100)

        tests = [
            ('%f', 100.0),
            ('%g', 100.0),
            ('%#g', 100.0),
            ('%#.2g', 100.0),
            ('%#.2g', 123.4567),
            ('%#.2g', 1.234567e200),
            ('%e', 1.234567e200),
            ('%e', 1.234),
            ('%+e', 1.234),
            ('%-e', 1.234),
            ]

        with check_warnings():
            for format, val in tests:
                PyOS_ascii_formatd(byref(buf), sizeof(buf),
                                   bytes(format, 'ascii'),
                                   c_double(val))
                self.assertEqual(buf.value, bytes(format % val, 'ascii'))


def test_main():
    run_unittest(FormatDeprecationTests, FormatTests)

if __name__ == '__main__':
    test_main()
