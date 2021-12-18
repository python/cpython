import unittest

GLOBAL_LIST = []

class RefLeakTest(unittest.TestCase):
    def test_leak(self):
        GLOBAL_LIST.append(object())

if __name__ == '__main__':
    unittest.main()
