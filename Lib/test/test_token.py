import unittest
import token

class TokenTest(unittest.TestCase):
    def test_terminal_validations(self):
        # Regression test for gh-142968
        self.assertRaises(TypeError, token.ISTERMINAL, 0.5)
        self.assertRaises(TypeError, token.ISNONTERMINAL, 0.5)
        self.assertRaises(TypeError, token.ISTERMINAL, "string")
        self.assertRaises(TypeError, token.ISNONTERMINAL, "string")

        # (sanity check)
        self.assertIs(token.ISTERMINAL(1), True)
        self.assertIs(token.ISNONTERMINAL(1), False)

if __name__ == "__main__":
    unittest.main()
