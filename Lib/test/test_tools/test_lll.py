"""Tests for the lll script in the Tools/script directory."""

import os
import tempfile
from test import support
from test.test_tools import skip_if_missing, import_tool
import unittest

skip_if_missing()


class lllTests(unittest.TestCase):

    def setUp(self):
        self.lll = import_tool('lll')

    @support.skip_unless_symlink
    def test_lll_multiple_dirs(self):
        with tempfile.TemporaryDirectory() as dir1, \
             tempfile.TemporaryDirectory() as dir2:
            fn1 = os.path.join(dir1, 'foo1')
            fn2 = os.path.join(dir2, 'foo2')
            for fn, dir in (fn1, dir1), (fn2, dir2):
                open(fn, 'w').close()
                os.symlink(fn, os.path.join(dir, 'symlink'))

            with support.captured_stdout() as output:
                self.lll.main([dir1, dir2])
            self.assertEqual(output.getvalue(),
                f'{dir1}:\n'
                f'symlink -> {fn1}\n'
                f'\n'
                f'{dir2}:\n'
                f'symlink -> {fn2}\n'
            )


if __name__ == '__main__':
    unittest.main()
