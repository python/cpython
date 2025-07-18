import math
from test.test_json import PyTest, CTest


class TestFloat:
    def test_floats(self):
        for num in [1617161771.7650001, math.pi, math.pi**100, math.pi**-100, 3.1]:
            self.assertEqual(float(self.dumps(num)), num)
            self.assertEqual(self.loads(self.dumps(num)), num)

    def test_ints(self):
        for num in [1, 1<<32, 1<<64]:
            self.assertEqual(self.dumps(num), str(num))
            self.assertEqual(int(self.dumps(num)), num)

    def test_out_of_range(self):
        self.assertEqual(self.loads('[23456789012E666]'), [float('inf')])
        self.assertEqual(self.loads('[-23456789012E666]'), [float('-inf')])

    def test_allow_nan(self):
        for val in (float('inf'), float('-inf'), float('nan')):
            out = self.dumps([val])
            if val == val:  # inf
                self.assertEqual(self.loads(out), [val])
            else:  # nan
                res = self.loads(out)
                self.assertEqual(len(res), 1)
                self.assertNotEqual(res[0], res[0])
            msg = f'Out of range float values are not JSON compliant: {val}'
            self.assertRaisesRegex(ValueError, msg, self.dumps, [val], allow_nan=False)


class TestPyFloat(TestFloat, PyTest): pass
class TestCFloat(TestFloat, CTest): pass
