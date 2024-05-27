import io
import itertools
import math
import re
import string
import unittest
import zipfile

from ._functools import compose
from ._itertools import consume

from ._support import import_or_skip


big_o = import_or_skip('big_o')
pytest = import_or_skip('pytest')


class TestComplexity(unittest.TestCase):
    @pytest.mark.flaky
    def test_implied_dirs_performance(self):
        best, others = big_o.big_o(
            compose(consume, zipfile._path.CompleteDirs._implied_dirs),
            lambda size: [
                '/'.join(string.ascii_lowercase + str(n)) for n in range(size)
            ],
            max_n=1000,
            min_n=1,
        )
        assert best <= big_o.complexities.Linear

    def make_zip_path(self, depth=1, width=1) -> zipfile.Path:
        """
        Construct a Path with width files at every level of depth.
        """
        zf = zipfile.ZipFile(io.BytesIO(), mode='w')
        pairs = itertools.product(self.make_deep_paths(depth), self.make_names(width))
        for path, name in pairs:
            zf.writestr(f"{path}{name}.txt", b'')
        zf.filename = "big un.zip"
        return zipfile.Path(zf)

    @classmethod
    def make_names(cls, width, letters=string.ascii_lowercase):
        """
        >>> list(TestComplexity.make_names(2))
        ['a', 'b']
        >>> list(TestComplexity.make_names(30))
        ['aa', 'ab', ..., 'bd']
        """
        # determine how many products are needed to produce width
        n_products = math.ceil(math.log(width, len(letters)))
        inputs = (letters,) * n_products
        combinations = itertools.product(*inputs)
        names = map(''.join, combinations)
        return itertools.islice(names, width)

    @classmethod
    def make_deep_paths(cls, depth):
        return map(cls.make_deep_path, range(depth))

    @classmethod
    def make_deep_path(cls, depth):
        return ''.join(('d/',) * depth)

    def test_baseline_regex_complexity(self):
        best, others = big_o.big_o(
            lambda path: re.fullmatch(r'[^/]*\\.txt', path),
            self.make_deep_path,
            max_n=100,
            min_n=1,
        )
        assert best <= big_o.complexities.Constant

    @pytest.mark.flaky
    def test_glob_depth(self):
        best, others = big_o.big_o(
            lambda path: consume(path.glob('*.txt')),
            self.make_zip_path,
            max_n=100,
            min_n=1,
        )
        assert best <= big_o.complexities.Quadratic

    @pytest.mark.flaky
    def test_glob_width(self):
        best, others = big_o.big_o(
            lambda path: consume(path.glob('*.txt')),
            lambda size: self.make_zip_path(width=size),
            max_n=100,
            min_n=1,
        )
        assert best <= big_o.complexities.Linear

    @pytest.mark.flaky
    def test_glob_width_and_depth(self):
        best, others = big_o.big_o(
            lambda path: consume(path.glob('*.txt')),
            lambda size: self.make_zip_path(depth=size, width=size),
            max_n=10,
            min_n=1,
        )
        assert best <= big_o.complexities.Linear
