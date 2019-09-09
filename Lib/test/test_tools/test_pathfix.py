import os
import subprocess
import sys
import unittest
from test import support
from test.test_tools import import_tool, scriptsdir, skip_if_missing


# need Tools/script/ directory: skip if run on Python installed on the system
skip_if_missing()


class TestPathfixFunctional(unittest.TestCase):
    script = os.path.join(scriptsdir, 'pathfix.py')

    def setUp(self):
        self.temp_file = support.TESTFN
        self.addCleanup(support.unlink, support.TESTFN)

    def pathfix(self, shebang, pathfix_flags, expected_returncode=0):
        with open(self.temp_file, 'w', encoding='utf8') as f:
            f.write(f'{shebang}\n' + 'print("Hello world")\n')

        proc = subprocess.run(
            [sys.executable, self.script,
             *pathfix_flags, '-n', self.temp_file],
            capture_output=True)
        self.assertEqual(proc.returncode, expected_returncode, proc)

        with open(self.temp_file, 'r', encoding='utf8') as f:
            output = f.read()

        lines = output.split('\n')
        self.assertEqual(lines[1:], ['print("Hello world")', ''])
        shebang = lines[0]
        return shebang

    def test_pathfix(self):
        self.assertEqual(
            self.pathfix(
                '#! /usr/bin/env python',
                ['-i', '/usr/bin/python3']),
            '#! /usr/bin/python3',
        )
        self.assertEqual(
            self.pathfix(
                '#! /usr/bin/env python -R',
                ['-i', '/usr/bin/python3']),
            '#! /usr/bin/python3',
        )

    def test_pathfix_keeping_flags(self):
        self.assertEqual(
            self.pathfix(
                '#! /usr/bin/env python -R',
                ['-i', '/usr/bin/python3', '-k']),
            '#! /usr/bin/python3 -R',
        )
        self.assertEqual(
            self.pathfix(
                '#! /usr/bin/env python',
                ['-i', '/usr/bin/python3', '-k']),
            '#! /usr/bin/python3',
        )

    def test_pathfix_adding_flag(self):
        self.assertEqual(
            self.pathfix(
                '#! /usr/bin/env python',
                ['-i', '/usr/bin/python3', '-a', 's']),
            '#! /usr/bin/python3 -s',
        )
        self.assertEqual(
            self.pathfix(
                '#! /usr/bin/env python -s',
                ['-i', '/usr/bin/python3', '-a', 's']),
            '#! /usr/bin/python3 -s',
        )
        self.assertEqual(
            self.pathfix(
                '#! /usr/bin/env python -v',
                ['-i', '/usr/bin/python3', '-a', 'v', '-k']),
            '#! /usr/bin/python3 -vv',
        )
        self.assertEqual(
            self.pathfix(
                '#! /usr/bin/env python',
                ['-i', '/usr/bin/python3', '-a', 'Rs']),
            '#! /usr/bin/python3 -Rs'
        )
        self.assertEqual(
            self.pathfix(
                '#! /usr/bin/env python -W something',
                ['-i', '/usr/bin/python3', '-a', 's', '-k']),
            '#! /usr/bin/python3 -sW something',
        )


if __name__ == '__main__':
    unittest.main()
