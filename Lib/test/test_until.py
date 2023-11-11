import unittest

class UntilTest(unittest.TestCase):
    def test_until(self):
        runs = 0
        until runs == 2:
            runs += 1
        assert runs == 2

if __name__ == '__main__':
    unittest.main()


