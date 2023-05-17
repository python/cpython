from test.support.hypothesis_helper import hypothesis

floats = hypothesis.strategies.floats
integers = hypothesis.strategies.integers

from math import nextafter, inf
from functools import reduce


def via_reduce(x, y, steps):
    return reduce(nextafter, [y] * steps, x)

class NextafterTests(unittest.TestCase):
    @requires_IEEE_754
    @hypothesis.given(
      x=floats(),
      y=floats(),
      a=integers(),
      b=integers())
    def test_addition_commutes(self, x, y, a, b):
      assert nextafter(nextafter(x, y, steps=a), steps=b) == nextafter(x, y, steps=a+b)
    
    @requires_IEEE_754
    @hypothesis.given(
      x=floats(),
      y=floats(),
      steps=integers())
    def test_count(self, x, y, steps):
      assert via_reduce(x, y, steps) == nextafter(x, y, steps=steps)
