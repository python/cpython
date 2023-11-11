import unittest

class SafeAttrTest(unittest.TestCase):
    
    def test_safe_attr(self):
        # Test basic safe attribute access
        class A:
            def __init__(self) -> None:
                self.b = 0

        a = A()
        assert a.b == a?.b == 0


if __name__ == '__main__':
    unittest.main()
