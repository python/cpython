import unittest
import string
import zipfile

from ._functools import compose
from ._itertools import consume

from ._support import import_or_skip


big_o = import_or_skip('big_o')


class TestComplexity(unittest.TestCase):
    def test_implied_dirs_performance(self):
        best, others = big_o.big_o(
            compose(consume, zipfile.CompleteDirs._implied_dirs),
            lambda size: [
                '/'.join(string.ascii_lowercase + str(n)) for n in range(size)
            ],
            max_n=1000,
            min_n=1,
        )
        assert best <= big_o.complexities.Linear
