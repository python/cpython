from test.test_json import PyTest, CTest


class TestTransform:
    def test_transform(self):
        self.assertEqual(
            self.dumps(type, transform=repr),
            self.dumps(repr(type)))

    def test_transform_intrinsics(self):
        def _transform(o):
            if isinstance(o, tuple):
                return "Tuple%s" % str(o)

            if isinstance(o, dict):
                return "Dict%s" % str(o)

            if isinstance(o, list):
                return "List%s" % str(o)

            return o

        for t in (1, 2, 3), [1, 2, 3], {"foo": "bar"}:
            self.assertEqual(
                self.dumps(t, transform=_transform),
                self.dumps(_transform(t)))


class TestPyTransform(TestTransform, PyTest): pass
class TestCTransform(TestTransform, CTest): pass
