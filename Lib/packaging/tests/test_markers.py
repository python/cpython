"""Tests for packaging.markers."""
import os
import sys
import platform
from packaging.markers import interpret

from packaging.tests import unittest
from packaging.tests.support import LoggingCatcher


class MarkersTestCase(LoggingCatcher,
                      unittest.TestCase):

    def test_interpret(self):
        sys_platform = sys.platform
        version = sys.version.split()[0]
        os_name = os.name
        platform_version = platform.version()
        platform_machine = platform.machine()
        platform_python_implementation = platform.python_implementation()

        self.assertTrue(interpret("sys.platform == '%s'" % sys_platform))
        self.assertTrue(interpret(
            "sys.platform == '%s' or python_version == '2.4'" % sys_platform))
        self.assertTrue(interpret(
            "sys.platform == '%s' and python_full_version == '%s'" %
            (sys_platform, version)))
        self.assertTrue(interpret("'%s' == sys.platform" % sys_platform))
        self.assertTrue(interpret('os.name == "%s"' % os_name))
        self.assertTrue(interpret(
            'platform.version == "%s" and platform.machine == "%s"' %
            (platform_version, platform_machine)))
        self.assertTrue(interpret('platform.python_implementation == "%s"' %
            platform_python_implementation))

        # stuff that need to raise a syntax error
        ops = ('os.name == os.name', 'os.name == 2', "'2' == '2'",
               'okpjonon', '', 'os.name ==', 'python_version == 2.4')
        for op in ops:
            self.assertRaises(SyntaxError, interpret, op)

        # combined operations
        OP = 'os.name == "%s"' % os_name
        AND = ' and '
        OR = ' or '
        self.assertTrue(interpret(OP + AND + OP))
        self.assertTrue(interpret(OP + AND + OP + AND + OP))
        self.assertTrue(interpret(OP + OR + OP))
        self.assertTrue(interpret(OP + OR + OP + OR + OP))

        # other operators
        self.assertTrue(interpret("os.name != 'buuuu'"))
        self.assertTrue(interpret("python_version > '1.0'"))
        self.assertTrue(interpret("python_version < '5.0'"))
        self.assertTrue(interpret("python_version <= '5.0'"))
        self.assertTrue(interpret("python_version >= '1.0'"))
        self.assertTrue(interpret("'%s' in os.name" % os_name))
        self.assertTrue(interpret("'buuuu' not in os.name"))
        self.assertTrue(interpret(
            "'buuuu' not in os.name and '%s' in os.name" % os_name))

        # execution context
        self.assertTrue(interpret('python_version == "0.1"',
                                  {'python_version': '0.1'}))


def test_suite():
    return unittest.makeSuite(MarkersTestCase)

if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
