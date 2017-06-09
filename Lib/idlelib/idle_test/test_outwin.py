""" Test idlelib.outwin.
"""

import unittest
import unittest.mock as mock
import re
from idlelib import outwin


class DummyOutputWindow:
    _file_line_helper = outwin.OutputWindow._file_line_helper
    goto_file_line = outwin.OutputWindow.goto_file_line
    file_line_pats = outwin.OutputWindow.file_line_pats


class OutputWindowFunctionTest(unittest.TestCase):

    @mock.patch('builtins.open')
    def test_file_line_helper(self, mock_open):
        dummy = DummyOutputWindow()

        l = []
        for pat in dummy.file_line_pats:
            l.append(re.compile(pat, re.IGNORECASE))
        dummy.file_line_progs = l

        text = ((r'file "filepasstest1", line 42, text', ('filepasstest1', 42)),
                (r'filepasstest2(42)', ('filepasstest2', 42)),
                (r'filepasstest3: 42: text text\n', ('filepasstest3', 42)),
                (r'filefailtest 10 text', None))
        for line, expected_output in text:
            self.assertEqual(dummy._file_line_helper(line), expected_output)
            if expected_output:
                mock_open.assert_called_with(expected_output[0], 'r')


if __name__ == '__main__':
    unittest.main(verbosity=2)
