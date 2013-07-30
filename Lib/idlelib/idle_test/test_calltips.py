import unittest
import idlelib.CallTips as ct
CTi = ct.CallTips()

class Get_entityTest(unittest.TestCase):
    # In 3.x, get_entity changed from 'instance method' to module function
    # since 'self' not used. Use dummy instance until change 2.7 also.
    def test_bad_entity(self):
        self.assertIsNone(CTi.get_entity('1/0'))
    def test_good_entity(self):
        self.assertIs(CTi.get_entity('int'), int)

class Py2Test(unittest.TestCase):
    def test_paramtuple_float(self):
        # 18539: (a,b) becomes '.0' in code object; change that but not float
        def f((a,b), c=0.0): pass
        self.assertEqual(ct.get_arg_text(f), '(<tuple>, c=0.0)')

if __name__ == '__main__':
    unittest.main(verbosity=2, exit=False)
