import collections
from test.test_json import PyTest, CTest


class TestDefault:
    def test_default(self):
        self.assertEqual(
            self.dumps(type, default=repr),
            self.dumps(repr(type)))

    def test_ordereddict(self):
        od = collections.OrderedDict(a=1, b=2, c=3, d=4)
        od.move_to_end('b')
        self.assertEqual(
            self.dumps(od),
            '{"a": 1, "c": 3, "d": 4, "b": 2}')
        self.assertEqual(
            self.dumps(od, sort_keys=True),
            '{"a": 1, "b": 2, "c": 3, "d": 4}')


class TestPyDefault(TestDefault, PyTest): pass
class TestCDefault(TestDefault, CTest): pass
