"""Tests for scripts in the Tools directory.

This file contains regression tests for some of the scripts found in the
Tools directory of a Python checkout or tarball.
"""

import os
import unittest
from test.support.script_helper import assert_python_ok

from test.test_tools import toolsdir, skip_if_missing

skip_if_missing()

class ReindentTests(unittest.TestCase):
    script = os.path.join(toolsdir, 'cases_generator', 'lexer.py')

    def test_multiline_comment_dedent_dedent4(self):
        input_code = """
        int main() {
        /*
            This is a
            multi-line comment.
            Let's see if it de-indents correctly.
        */
        return 0;
    }

        """

        expected_output = """
    int main() {
    /*
        This is a
        multi-line comment.
        Let's see if it de-indents correctly.
    */
    return 0;
}
"""

        dedent_amount = '4'
        rc, out, err = assert_python_ok(self.script, '-c', input_code, dedent_amount)
        self.assertEqual(out, bytes(expected_output, 'utf-8')[1:], "Multi-line comment de-indentation failed")

    def test_multiline_comment_dedent_dedent40(self):
        input_code = """
        int main() {
        /*
            This is a
            multi-line comment.
            Let's see if it de-indents correctly.
        */
        return 0;
    }

        """

        expected_output = """
int main() {
/*
This is a
multi-line comment.
Let's see if it de-indents correctly.
*/
return 0;
}
"""

        dedent_amount = '40'
        rc, out, err = assert_python_ok(self.script, '-c', input_code, dedent_amount)
        self.assertEqual(out, bytes(expected_output, 'utf-8')[1:], "Multi-line comment de-indentation failed")

if __name__ == '__main__':
    unittest.main()
