import functools
import unittest
from math import isnan, nextafter
from test.support import requires_IEEE_754
from test.support.hypothesis_helper import hypothesis

floats = hypothesis.strategies.floats
integers = hypothesis.strategies.integers


def assert_equal_float(x, y):
    assert isnan(x) and isnan(y) or x == y


def via_reduce(x, y, steps):
    return functools.reduce(nextafter, [y] * steps, x)


class NextafterTests(unittest.TestCase):
    @requires_IEEE_754
    @hypothesis.given(
        x=floats(),
        y=floats(),
        steps=integers(min_value=0, max_value=2**16))
    def test_count(self, x, y, steps):
        assert_equal_float(via_reduce(x, y, steps),
                           nextafter(x, y, steps=steps))

    @requires_IEEE_754
    @hypothesis.given(
        x=floats(),
        y=floats(),
        a=integers(min_value=0),
        b=integers(min_value=0))
    def test_addition_commutes(self, x, y, a, b):
        first = nextafter(x, y, steps=a)
        second = nextafter(first, y, steps=b)
        combined = nextafter(x, y, steps=a+b)
        hypothesis.note(f"{first} -> {second} == {combined}")

        assert_equal_float(second, combined)
