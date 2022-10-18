import unittest


class ListMaxTest(unittest.TestCase):
    def test_empty_list(self):
        ls = []
        msg = "max() arg is an empty sequence"
        with self.assertRaises(ValueError, msg=msg):
            ls.max()
    
    def test_one_element_list(self):
        ls = [0]
        self.assertEqual(ls.max(), 0)
    
    def test_small_int_list(self):
        ls = [1, 4, 5]
        self.assertEqual(ls.max(), 5)
    
    def test_small_float_list(self):
        ls = [1.2, 3.4, 5.6]
        self.assertEqual(ls.max(), 5.6)
    
    def test_large_int_list(self):
        ls = [
            231212340289713793871,
            -123234203423412312320,
            212342423309546512039
        ]
        self.assertEqual(ls.max(), 231212340289713793871)

if __name__ == '__main__':
    unittest.main()