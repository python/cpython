import collections
from test.test_json import PyTest, CTest


class TestDefault:
    def test_default(self):
        self.assertEqual(
            self.dumps(type, default=repr),
            self.dumps(repr(type)))

    def test_ordereddict(self):
        od = collections.OrderedDict(a=1, b=2)
        od.move_to_end('a')
        self.assertEqual(
            self.dumps(od),
            '{"b": 2, "a": 1}')
        self.assertEqual(
            self.dumps(od, sort_keys=True),
            '{"a": 1, "b": 2}')


class TestPyDefault(TestDefault, PyTest): pass
class TestCDefault(TestDefault, CTest): pass
