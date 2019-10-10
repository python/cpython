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

    def pathfix(self, shebang, pathfix_flags, exitcode=0, stdout='', stderr=''):
        with open(self.temp_file, 'w', encoding='utf8') as f:
            f.write(f'{shebang}\n' + 'print("Hello world")\n')

        proc = subprocess.run(
            [sys.executable, self.script,
             *pathfix_flags, '-n', self.temp_file],
            capture_output=True, text=1)

        if stdout == '' and proc.returncode == 0:
            stdout = f'{self.temp_file}: updating\n'
        self.assertEqual(proc.returncode, exitcode, proc)
        self.assertEqual(proc.stdout, stdout, proc)
        self.assertEqual(proc.stderr, stderr, proc)

        with open(self.temp_file, 'r', encoding='utf8') as f:
            output = f.read()

        lines = output.split('\n')
        self.assertEqual(lines[1:], ['print("Hello world")', ''])
        new_shebang = lines[0]

        if proc.returncode != 0:
            self.assertEqual(shebang, new_shebang)

        return new_shebang

    def test_pathfix(self):
        self.assertEqual(
            self.pathfix(
                '#! /usr/bin/env python',
                ['-i', '/usr/bin/python3']),
            '#! /usr/bin/python3')
        self.assertEqual(
            self.pathfix(
                '#! /usr/bin/env python -R',
                ['-i', '/usr/bin/python3']),
            '#! /usr/bin/python3')

    def test_pathfix_keeping_flags(self):
        self.assertEqual(
            self.pathfix(
                '#! /usr/bin/env python -R',
                ['-i', '/usr/bin/python3', '-k']),
            '#! /usr/bin/python3 -R')
        self.assertEqual(
            self.pathfix(
                '#! /usr/bin/env python',
                ['-i', '/usr/bin/python3', '-k']),
            '#! /usr/bin/python3')

    def test_pathfix_adding_flag(self):
        self.assertEqual(
            self.pathfix(
                '#! /usr/bin/env python',
                ['-i', '/usr/bin/python3', '-a', 's']),
            '#! /usr/bin/python3 -s')
        self.assertEqual(
            self.pathfix(
                '#! /usr/bin/env python -S',
                ['-i', '/usr/bin/python3', '-a', 's']),
            '#! /usr/bin/python3 -s')
        self.assertEqual(
            self.pathfix(
                '#! /usr/bin/env python -V',
                ['-i', '/usr/bin/python3', '-a', 'v', '-k']),
            '#! /usr/bin/python3 -vV')
        self.assertEqual(
            self.pathfix(
                '#! /usr/bin/env python',
                ['-i', '/usr/bin/python3', '-a', 'Rs']),
            '#! /usr/bin/python3 -Rs')
        self.assertEqual(
            self.pathfix(
                '#! /usr/bin/env python -W default',
                ['-i', '/usr/bin/python3', '-a', 's', '-k']),
            '#! /usr/bin/python3 -sW default')

    def test_pathfix_adding_errors(self):
        self.pathfix(
            '#! /usr/bin/env python -E',
            ['-i', '/usr/bin/python3', '-a', 'W default', '-k'],
            exitcode=2,
            stderr="-a option doesn't support whitespaces")


if __name__ == '__main__':
    unittest.main()
