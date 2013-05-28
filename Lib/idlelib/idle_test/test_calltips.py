import unittest
import idlelib.CallTips as ct

class Test_get_entity(unittest.TestCase):
    def test_bad_entity(self):
        self.assertIsNone(ct.get_entity('1/0'))
    def test_good_entity(self):
        self.assertIs(ct.get_entity('int'), int)

if __name__ == '__main__':
    unittest.main(verbosity=2, exit=False)
