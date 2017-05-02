import unittest
import sys

if __name__ == '__main__':
    tc = unittest.TestCase('__init__')
    sys.setrecursionlimit(50)

    step = 100
    list1 = list(range(1, step))
    list2 = list(range(step, step+step))

    tc.assertEquals(list1, list2)

