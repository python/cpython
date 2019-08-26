import textwrap
import unittest

from .. import tool_imports_for_tests
with tool_imports_for_tests():
    from c_parser.info import Variable
    from c_analyzer_common.info import ID
    from c_analyzer_common.known import from_file


class FromFileTests(unittest.TestCase):

    maxDiff = None

    def test_typical(self):
        lines = textwrap.dedent('''
            filename	funcname	name	kind	declaration
            file1.c		var1	variable	static int
            file1.c	func1	local1	variable	static int
            file1.c		var2	variable	int
            file1.c	func2	local2	variable	char *
            file2.c		var1	variable	char *
            ''').strip().splitlines()

        known = from_file(lines, _open=None)

        self.assertEqual(known, {
            'variables': {v.id: v for v in [
                Variable.from_parts('file1.c', '', 'var1', 'static int'),
                Variable.from_parts('file1.c', 'func1', 'local1', 'static int'),
                Variable.from_parts('file1.c', '', 'var2', 'int'),
                Variable.from_parts('file1.c', 'func2', 'local2', 'char *'),
                Variable.from_parts('file2.c', '', 'var1', 'char *'),
                ]},
            })

    def test_empty(self):
        lines = textwrap.dedent('''
            filename	funcname	name	kind	declaration
            '''.strip()).splitlines()

        known = from_file(lines, _open=None)

        self.assertEqual(known, {
            'variables': {},
            })
