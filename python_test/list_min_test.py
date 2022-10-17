import unittest


class ListMinTest(unittest.TestCase):
    def test_empty_list(self):
        ls = []
        msg = "min()) arg is an empty sequence"
        with self.assertRaises(ValueError, msg=msg):
            ls.min()
    
    def test_one_element_list(self):
        ls = [0]
        self.assertEqual(ls.min(), 0)
    
    def test_small_int_list(self):
        ls = [3, 1, 5]
        self.assertEqual(ls.min(), 1)
    
    def test_small_float_list(self):
        ls = [1.2, -3.4, 5.6]
        self.assertEqual(ls.min(), -3.4)
    
    def test_large_int_list(self):
        ls = [
            231212340289713793871,
            212342423309546512039,
            -123234203423412312320,
        ]
        self.assertEqual(ls.min(), -123234203423412312320)

if __name__ == '__main__':
    unittest.main()