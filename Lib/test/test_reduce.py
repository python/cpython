import unittest
import operator
import sys
import functools as c_functools

class Test_reduce(unittest.TestCase):
    def test_add_func(self):
        del sys.modules['functools']
        sys.modules['_functools'] = None  # block the C accelerator
        import functools as py_functools

        class Iterator:
            def __init__(self):
                self.items = [1, 2, 3, 4, 4]
            def __next__(self):
                if not self.items:
                    raise StopIteration
                return self.items.pop(0)
        class Iterable:
            def __iter__(self):
                return Iterator()
        self.assertEqual(
            c_functools.reduce(operator.add, Iterable()),
            py_functools.reduce(operator.add, Iterable())
        )

if __name__ == "__main__":
    unittest.main()
