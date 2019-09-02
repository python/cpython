import unittest, os, subprocess
from test.test_tools import import_tool, scriptsdir
import sys
from test import support


class TestPathfixFunctional(unittest.TestCase):
    script = os.path.join(scriptsdir, 'pathfix.py')

    def setUp(self):
        self.temp_file = support.TESTFN
        self.addCleanup(support.unlink, support.TESTFN)

    def pathfix(self, shebang, pathfix_flags):
        with open(self.temp_file, 'w') as f:
            f.write(f'{shebang}\n' + 'print("Hello world")\n')
        subprocess.call([sys.executable, self.script, *pathfix_flags, self.temp_file])
        with open(self.temp_file) as f:
            output = f.read()
        print(output)
        return output

    def test_pathfix_keeping_flags(self):
        self.assertEqual(
            self.pathfix(
                '#! /usr/bin/env python -R',
                ['-i', '/usr/bin/python', '-k', '-n']),
            '#! /usr/bin/python -R\n' + 'print("Hello world")\n'
        )

    def test_pathfix_without_keeping_flags(self):
        self.assertEqual(
            self.pathfix(
                '#! /usr/bin/env python -R',
                ['-i', '/usr/bin/python', '-n']),
            '#! /usr/bin/python\n' + 'print("Hello world")\n'
        )

    def test_pathfix(self):
        self.assertEqual(
            self.pathfix(
                '#! /usr/bin/env python',
                ['-i', '/usr/bin/python', '-n']),
            '#! /usr/bin/python\n' + 'print("Hello world")\n'
        )

if __name__ == '__main__':
    unittest.main()
