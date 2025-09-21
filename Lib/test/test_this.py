"""Tests for the this module."""

import unittest
import this
from unittest.mock import patch


class TestThis(unittest.TestCase):
    """Test cases for the this module."""

    def test_module_has_expected_variables(self):
        """Test that the module has the expected variables."""
        # The module should have 's' (the ROT13 encoded string) and 'd' (the translation dict)
        self.assertTrue(hasattr(this, 's'))
        self.assertTrue(hasattr(this, 'd'))
        
        # 's' should be a string
        self.assertIsInstance(this.s, str)
        self.assertTrue(len(this.s) > 0)
        
        # 'd' should be a dictionary
        self.assertIsInstance(this.d, dict)
        self.assertTrue(len(this.d) > 0)

    def test_rot13_dictionary(self):
        """Test that the ROT13 dictionary is correctly constructed."""
        # Test that it contains mappings for both uppercase and lowercase letters
        self.assertIn('A', this.d)
        self.assertIn('a', this.d)
        self.assertIn('Z', this.d)
        self.assertIn('z', this.d)
        
        # Test ROT13 properties
        self.assertEqual(this.d['A'], 'N')  # A -> N
        self.assertEqual(this.d['N'], 'A')  # N -> A (ROT13 is its own inverse)
        self.assertEqual(this.d['a'], 'n')  # a -> n
        self.assertEqual(this.d['n'], 'a')  # n -> a
        
        # Test that it covers the full alphabet
        for i in range(26):
            upper_char = chr(ord('A') + i)
            lower_char = chr(ord('a') + i)
            self.assertIn(upper_char, this.d)
            self.assertIn(lower_char, this.d)

    def test_rot13_inverse_property(self):
        """Test that ROT13 is its own inverse."""
        # For any letter, applying ROT13 twice should give the original
        for char in 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz':
            if char in this.d:
                rotated = this.d[char]
                double_rotated = this.d.get(rotated, rotated)
                self.assertEqual(char, double_rotated)

    def test_string_translation(self):
        """Test that the string translation works correctly."""
        # Test a simple ROT13 transformation
        test_string = "Hello"
        expected = "Uryyb"
        
        result = ''.join([this.d.get(c, c) for c in test_string])
        self.assertEqual(result, expected)

    def test_zen_of_python_content(self):
        """Test that the encoded string contains the Zen of Python."""
        # Decode the string using the dictionary
        decoded = ''.join([this.d.get(c, c) for c in this.s])
        
        # Check for key phrases from the Zen of Python
        zen_phrases = [
            "Beautiful is better than ugly",
            "Explicit is better than implicit",
            "Simple is better than complex",
            "Complex is better than complicated",
            "Readability counts",
            "There should be one",
            "Although never is often better than *right* now"
        ]
        
        for phrase in zen_phrases:
            self.assertIn(phrase, decoded)

    def test_print_statement_execution(self):
        """Test that the print statement would execute correctly."""
        with patch('builtins.print') as mock_print:
            # Execute the print statement from the module
            exec('print("".join([d.get(c, c) for c in s]))', {'d': this.d, 's': this.s})
            
            # Verify print was called
            mock_print.assert_called_once()
            
            # Get the printed content
            printed_content = mock_print.call_args[0][0]
            
            # Should contain the Zen of Python
            self.assertIn("Beautiful is better than ugly", printed_content)
            self.assertIn("The Zen of Python", printed_content)

    def test_module_structure(self):
        """Test the overall structure of the module."""
        # The module should be importable
        import this
        
        # Should have the expected attributes
        expected_attrs = ['s', 'd']
        for attr in expected_attrs:
            self.assertTrue(hasattr(this, attr))

    def test_rot13_edge_cases(self):
        """Test ROT13 with edge cases."""
        # Test with non-alphabetic characters
        test_chars = "123!@#$%^&*()_+-=[]{}|;':\",./<>?"
        for char in test_chars:
            # Non-alphabetic characters should not be in the dictionary
            # and should remain unchanged
            if char not in this.d:
                result = this.d.get(char, char)
                self.assertEqual(result, char)

    def test_dictionary_completeness(self):
        """Test that the ROT13 dictionary is complete."""
        # Should have exactly 52 entries (26 uppercase + 26 lowercase)
        self.assertEqual(len(this.d), 52)
        
        # All entries should be single character strings
        for key, value in this.d.items():
            self.assertIsInstance(key, str)
            self.assertIsInstance(value, str)
            self.assertEqual(len(key), 1)
            self.assertEqual(len(value), 1)

    def test_string_length(self):
        """Test that the encoded string has reasonable length."""
        # The Zen of Python is substantial, so the encoded string should be long
        self.assertGreater(len(this.s), 100)
        
        # Should contain newlines and spaces
        self.assertIn('\n', this.s)
        self.assertIn(' ', this.s)


if __name__ == '__main__':
    unittest.main()
