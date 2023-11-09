import unittest

class UntilTest(unittest.TestCase):
    
    def test_until(self):
        # Test basic safe attribute access
        runs = 0
        until runs == 2:
            runs += 1
        assert runs == 2

    # Add more tests for various use cases and edge cases.

if __name__ == '__main__':
    unittest.main()


