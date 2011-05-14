import math
from json.tests import PyTest, CTest


class TestFloat(object):
    def test_floats(self):
        for num in [1617161771.7650001, math.pi, math.pi**100,
                    math.pi**-100, 3.1]:
            self.assertEqual(float(self.dumps(num)), num)
            self.assertEqual(self.loads(self.dumps(num)), num)
            self.assertEqual(self.loads(unicode(self.dumps(num))), num)

    def test_ints(self):
        for num in [1, 1L, 1<<32, 1<<64]:
            self.assertEqual(self.dumps(num), str(num))
            self.assertEqual(int(self.dumps(num)), num)
            self.assertEqual(self.loads(self.dumps(num)), num)
            self.assertEqual(self.loads(unicode(self.dumps(num))), num)


class TestPyFloat(TestFloat, PyTest): pass
class TestCFloat(TestFloat, CTest): pass
