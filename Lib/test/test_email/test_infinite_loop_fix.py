"""Test for infinite loop fix in email MIME parameter folding.

This test verifies that the fix for GitHub issue #138223 prevents infinite loops
when processing MIME parameters with very long keys during RFC 2231 encoding.
"""

import unittest
import email.message
import email.policy
from email import policy


class TestInfiniteLoopFix(unittest.TestCase):
    """Test that infinite loops are prevented in MIME parameter folding."""

    def test_long_parameter_key_no_infinite_loop(self):
        """Test that very long parameter keys don't cause infinite loops."""
        msg = email.message.EmailMessage()
        
        # Create a parameter key that's 64 characters long (the problematic length)
        long_key = "a" * 64
        
        # Add attachment first
        msg.add_attachment(
            b"test content",
            maintype="text",
            subtype="plain",
            filename="test.txt"
        )
        
        # Set the long parameter using set_param
        msg.set_param(long_key, "test_value")
        
        # This should not hang - it should complete in reasonable time
        try:
            result = msg.as_string()
            # If we get here, no infinite loop occurred
            self.assertIsInstance(result, str)
            self.assertIn(long_key, result)
        except Exception as e:
            self.fail(f"as_string() failed with exception: {e}")

    def test_extremely_long_parameter_key(self):
        """Test with extremely long parameter keys that could cause maxchars < 0."""
        msg = email.message.EmailMessage()
        
        # Create an extremely long parameter key (100+ characters)
        extremely_long_key = "b" * 100
        
        # Add attachment first
        msg.add_attachment(
            b"test content",
            maintype="text",
            subtype="plain",
            filename="test.txt"
        )
        
        # Set the extremely long parameter
        msg.set_param(extremely_long_key, "test_value")
        
        # This should not hang even with extremely long keys
        try:
            result = msg.as_string()
            self.assertIsInstance(result, str)
            self.assertIn(extremely_long_key, result)
        except Exception as e:
            self.fail(f"as_string() failed with exception: {e}")

    def test_multiple_long_parameters(self):
        """Test with multiple long parameters to ensure robust handling."""
        msg = email.message.EmailMessage()
        
        # Add attachment first
        msg.add_attachment(
            b"test content",
            maintype="text",
            subtype="plain",
            filename="test.txt"
        )
        
        # Add multiple long parameters
        long_params = {
            "a" * 64: "value1",
            "b" * 80: "value2",
            "c" * 90: "value3"
        }
        
        for key, value in long_params.items():
            msg.set_param(key, value)
        
        try:
            result = msg.as_string()
            self.assertIsInstance(result, str)
            # Check that all parameters are present
            for key in long_params:
                self.assertIn(key, result)
        except Exception as e:
            self.fail(f"as_string() failed with exception: {e}")

    def test_edge_case_parameter_lengths(self):
        """Test edge cases around the problematic parameter lengths."""
        # Test parameter keys of various lengths around the problematic 64-char mark
        test_lengths = [60, 61, 62, 63, 64, 65, 66, 67, 68]
        
        for length in test_lengths:
            key = "x" * length
            msg = email.message.EmailMessage()
            
            # Add attachment first
            msg.add_attachment(
                b"test content",
                maintype="text",
                subtype="plain",
                filename="test.txt"
            )
            
            # Set the parameter
            msg.set_param(key, "test_value")
            
            try:
                result = msg.as_string()
                self.assertIsInstance(result, str)
                self.assertIn(key, result)
            except Exception as e:
                self.fail(f"as_string() failed with length {length}: {e}")

    def test_rfc_2231_compliance(self):
        """Test that the fix maintains RFC 2231 compliance."""
        msg = email.message.EmailMessage()
        
        # Add attachment first
        msg.add_attachment(
            b"test content",
            maintype="text",
            subtype="plain",
            filename="test.txt"
        )
        
        # Set a parameter with special characters that should trigger RFC 2231 encoding
        msg.set_param("long_param", "value_with_special_chars_ñáéíóú")
        
        try:
            result = msg.as_string()
            self.assertIsInstance(result, str)
            # The parameter should be properly encoded
            self.assertIn("long_param", result)
        except Exception as e:
            self.fail(f"as_string() failed with exception: {e}")


if __name__ == '__main__':
    unittest.main()
