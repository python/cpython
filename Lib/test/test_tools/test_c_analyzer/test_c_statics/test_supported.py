import textwrap
import unittest

from .. import tool_imports_for_tests
with tool_imports_for_tests():
    from c_analyzer_common.info import ID
    from c_parser import info
    from c_statics.supported import is_supported, ignored_from_file


class IsSupportedTests(unittest.TestCase):

    @unittest.expectedFailure
    def test_supported(self):
        statics = [
                info.StaticVar('src1/spam.c', None, 'var1', 'const char *'),
                info.StaticVar('src1/spam.c', None, 'var1', 'int'),
                ]
        for static in statics:
            with self.subTest(static):
                result = is_supported(static)

                self.assertTrue(result)

    @unittest.expectedFailure
    def test_not_supported(self):
        statics = [
                info.StaticVar('src1/spam.c', None, 'var1', 'PyObject *'),
                info.StaticVar('src1/spam.c', None, 'var1', 'PyObject[10]'),
                ]
        for static in statics:
            with self.subTest(static):
                result = is_supported(static)

                self.assertFalse(result)


class IgnoredFromFileTests(unittest.TestCase):

    maxDiff = None

    def test_typical(self):
        lines = textwrap.dedent('''
            filename	funcname	name	kind	reason
            file1.c		var1	variable	...
            file1.c	func1	local1	variable	
            file1.c		var2	variable	???
            file1.c	func2	local2	variable	    
            file2.c		var1	variable	reasons
            ''').strip().splitlines()

        ignored = ignored_from_file(lines, _open=None)

        self.assertEqual(ignored, {
            'variables': {
                ID('file1.c', '', 'var1'): '...',
                ID('file1.c', 'func1', 'local1'): '',
                ID('file1.c', '', 'var2'): '???',
                ID('file1.c', 'func2', 'local2'): '',
                ID('file2.c', '', 'var1'): 'reasons',
                },
            })

    def test_empty(self):
        lines = textwrap.dedent('''
            filename	funcname	name	kind	reason
            '''.strip()).splitlines()

        ignored = ignored_from_file(lines, _open=None)

        self.assertEqual(ignored, {
            'variables': {},
            })
