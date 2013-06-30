import unittest
from idlelib.Delegator import Delegator

class DelegatorTest(unittest.TestCase):

    def test_mydel(self):
        # test a simple use scenario

        # initialize
        mydel = Delegator(int)
        self.assertIs(mydel.delegate, int)
        self.assertEqual(mydel._Delegator__cache, set())

        # add an attribute:
        self.assertRaises(AttributeError, mydel.__getattr__, 'xyz')
        bl = mydel.bit_length
        self.assertIs(bl, int.bit_length)
        self.assertIs(mydel.__dict__['bit_length'], int.bit_length)
        self.assertEqual(mydel._Delegator__cache, {'bit_length'})

        # add a second attribute
        mydel.numerator
        self.assertEqual(mydel._Delegator__cache, {'bit_length', 'numerator'})

        # delete the second (which, however, leaves it in the name cache)
        del mydel.numerator
        self.assertNotIn('numerator', mydel.__dict__)
        self.assertIn('numerator', mydel._Delegator__cache)

        # reset by calling .setdelegate, which calls .resetcache
        mydel.setdelegate(float)
        self.assertIs(mydel.delegate, float)
        self.assertNotIn('bit_length', mydel.__dict__)
        self.assertEqual(mydel._Delegator__cache, set())

if __name__ == '__main__':
    unittest.main(verbosity=2, exit=2)
