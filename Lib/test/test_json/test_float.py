import math
from test.test_json import PyTest, CTest


class NotUsableAsABoolean:
    def __bool__(self):
        raise TypeError("I refuse to be interpreted as a boolean")


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

    def test_allow_nan_null(self):
        # when allow_nan is 'as_null', infinities and NaNs convert to 'null'
        for val in [float('inf'), float('-inf'), float('nan')]:
            with self.subTest(val=val):
                out = self.dumps([val], allow_nan='as_null')
                res = self.loads(out)
                self.assertEqual(res, [None])

        # and finite values are treated as normal
        for val in [1.25, -23, -0.0, 0.0]:
            with self.subTest(val=val):
                out = self.dumps([val], allow_nan='as_null')
                res = self.loads(out)
                self.assertEqual(res, [val])

        # testing a mixture
        vals = [-1.3, 1e100, -math.inf, 1234, -0.0, math.nan]
        out = self.dumps(vals, allow_nan='as_null')
        res = self.loads(out)
        self.assertEqual(res, [-1.3, 1e100, None, 1234, -0.0, None])

    def test_allow_nan_string_deprecation(self):
        with self.assertWarns(DeprecationWarning):
            self.dumps(2.3, allow_nan='true')

    def test_allow_nan_non_boolean(self):
        # check that exception gets propagated as expected
        with self.assertRaises(TypeError):
            self.dumps(math.inf, allow_nan=NotUsableAsABoolean())


class TestPyFloat(TestFloat, PyTest): pass
class TestCFloat(TestFloat, CTest): pass
