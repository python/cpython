from collections import deque
from test.test_json import PyTest, CTest


class TestDeque:
    def test_deque_ints(self):
        d = deque(range(0, 10))
        self.assertEqual(self.dumps(d), '[0, 1, 2, 3, 4, 5, 6, 7, 8, 9]')

    def test_deque_strings(self):
        d = deque('hello')
        self.assertEqual(self.dumps(d), '["h", "e", "l", "l", "o"]')


class TestPyDeque(TestDeque, PyTest): pass
class TestCDeque(TestDeque, CTest): pass
