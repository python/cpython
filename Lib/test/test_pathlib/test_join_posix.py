"""
Tests for Posix-flavoured pathlib.types._JoinablePath
"""

import os
import unittest

from .support import is_pypi
from .support.lexical_path import LexicalPosixPath


class JoinTestBase:
    def test_join(self):
        P = self.cls
        p = P('//a')
        pp = p.joinpath('b')
        self.assertEqual(pp, P('//a/b'))
        pp = P('/a').joinpath('//c')
        self.assertEqual(pp, P('//c'))
        pp = P('//a').joinpath('/c')
        self.assertEqual(pp, P('/c'))

    def test_div(self):
        # Basically the same as joinpath().
        P = self.cls
        p = P('//a')
        pp = p / 'b'
        self.assertEqual(pp, P('//a/b'))
        pp = P('/a') / '//c'
        self.assertEqual(pp, P('//c'))
        pp = P('//a') / '/c'
        self.assertEqual(pp, P('/c'))


class LexicalPosixPathJoinTest(JoinTestBase, unittest.TestCase):
    cls = LexicalPosixPath


if not is_pypi:
    from pathlib import PurePosixPath, PosixPath

    class PurePosixPathJoinTest(JoinTestBase, unittest.TestCase):
        cls = PurePosixPath

    if os.name != 'nt':
        class PosixPathJoinTest(JoinTestBase, unittest.TestCase):
            cls = PosixPath


if __name__ == "__main__":
    unittest.main()
