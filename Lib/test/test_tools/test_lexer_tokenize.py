"""Tests for scripts in the Tools directory.

This file contains regression tests for some of the scripts found in the
Tools directory of a Python checkout or tarball.
"""

import os
import unittest
from test.support.script_helper import assert_python_ok
from test.support import findfile

from test.test_tools import toolsdir, skip_if_missing

skip_if_missing()

class TokenizeTests(unittest.TestCase):
    script = os.path.join(toolsdir, 'cases_generator', 'lexer.py')

    def test_identifiers(self):
        code = "int myVariable = 123;"
        expected_out = bytes("INT('int', 1:1:4)\nIDENTIFIER('myVariable', 1:5:15)\nEQUALS('=', 1:16:17)\nNUMBER('123', 1:18:21)\nSEMI(';', 1:21:22)\n", 'utf-8')
        rc, out, err = assert_python_ok(self.script, '-c', code)
        self.assertEqual(out, expected_out)

    def test_operators(self):
        code = "x = y + z;"
        expected_out = bytes("IDENTIFIER('x', 1:1:2)\nEQUALS('=', 1:3:4)\nIDENTIFIER('y', 1:5:6)\nPLUS('+', 1:7:8)\nIDENTIFIER('z', 1:9:10)\nSEMI(';', 1:10:11)\n", 'utf-8')
        rc, out, err = assert_python_ok(self.script, '-c', code)
        self.assertEqual(out, expected_out)

    def test_numbers(self):
        code = "int num = 42;"
        expected_out = bytes("INT('int', 1:1:4)\nIDENTIFIER('num', 1:5:8)\nEQUALS('=', 1:9:10)\nNUMBER('42', 1:11:13)\nSEMI(';', 1:13:14)\n", 'utf-8')
        rc, out, err = assert_python_ok(self.script, '-c', code)
        self.assertEqual(out, expected_out)

    def test_strings(self):
        code = 'printf("Hello, World!");'
        expected_out = bytes("""IDENTIFIER(\'printf\', 1:1:7)\nLPAREN(\'(\', 1:7:8)\nSTRING(\'"Hello, World!"\', 1:8:23)\nRPAREN(\')\', 1:23:24)\nSEMI(\';\', 1:24:25)\n""", 'utf-8')
        rc, out, err = assert_python_ok(self.script, '-c', code)
        self.assertEqual(out, expected_out)

    def test_characters_with_escape_sequences(self):
        code = "char a = '\n'; char b = '\x41'; char c = '\\';"
        expected_out = bytes("""CHAR(\'char\', 1:1:5)\nIDENTIFIER(\'a\', 1:6:7)\nEQUALS(\'=\', 1:8:9)\nCHARACTER("\'\\n\'", 1:10:13)\nSEMI(\';\', 1:13:14)\nCHAR(\'char\', 1:15:19)\nIDENTIFIER(\'b\', 1:20:21)\nEQUALS(\'=\', 1:22:23)\nCHARACTER("\'A\'", 1:24:27)\nSEMI(\';\', 1:27:28)\nCHAR(\'char\', 1:29:33)\nIDENTIFIER(\'c\', 1:34:35)\nEQUALS(\'=\', 1:36:37)\nCHARACTER("\'", 1:38:39)\nBACKSLASH(\'\\\\\', 1:39:40)\nCHARACTER("\'", 1:40:41)\nSEMI(\';\', 1:41:42)\n""", 'utf-8')
        rc, out, err = assert_python_ok(self.script, '-c', code)
        self.assertEqual(out, expected_out)

if __name__ == '__main__':
    unittest.main()