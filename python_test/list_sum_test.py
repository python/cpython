import unittest

class ListSumTest(unittest.TestCase):
    def test_empty_list(self):
        ls = []
        assert ls.sum() == 0
    
    def test_small_int_list(self):
        ls = [1, 4, 5]
        assert ls.sum() == 10
    
    def test_small_float_list(self):
        ls = [1.2, 3.4, 5.6]
        assert ls.sum() == 10.2
    
    def test_large_int_list(self):
        ls = [
            231212340289713793871,
            123234203423412312320,
            212342423309546512039
        ]
        assert ls.sum() == 566788967022672618230
    
    def test_large_float_list(self):
        ls = [
            231212340289713793871.12312312312,
            123234203423412312320.349853490,
            212342423309546512039.1230981230918
        ]
        assert ls.sum() == 5.667889670226726e+20

if __name__ == '__main__':
    unittest.main()