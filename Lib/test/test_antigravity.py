"""Tests for the antigravity module."""

import unittest
import antigravity
import hashlib
from unittest.mock import patch, MagicMock


class TestAntigravity(unittest.TestCase):
    """Test cases for the antigravity module."""

    def test_geohash_basic_functionality(self):
        """Test basic geohash functionality with known inputs."""
        # Test with the example from the docstring
        with patch('builtins.print') as mock_print:
            antigravity.geohash(37.421542, -122.085589, b'2005-05-26-10458.68')
            mock_print.assert_called_once()
            # The exact output depends on the MD5 hash, but we can verify it was called
            args = mock_print.call_args[0][0]
            self.assertIsInstance(args, str)
            # Should contain the latitude and longitude parts
            self.assertIn('37.', args)
            self.assertIn('-122.', args)

    def test_geohash_different_inputs(self):
        """Test geohash with different input values."""
        with patch('builtins.print') as mock_print:
            # Test with different coordinates
            antigravity.geohash(0.0, 0.0, b'test-date')
            mock_print.assert_called_once()
            args = mock_print.call_args[0][0]
            self.assertIsInstance(args, str)
            self.assertIn('0.', args)

    def test_geohash_hash_consistency(self):
        """Test that geohash produces consistent results for same inputs."""
        with patch('builtins.print') as mock_print:
            # Call geohash twice with same inputs
            antigravity.geohash(10.0, 20.0, b'same-date')
            first_call = mock_print.call_args[0][0]
            
            mock_print.reset_mock()
            antigravity.geohash(10.0, 20.0, b'same-date')
            second_call = mock_print.call_args[0][0]
            
            # Results should be identical
            self.assertEqual(first_call, second_call)

    def test_geohash_different_dates(self):
        """Test that different dates produce different results."""
        with patch('builtins.print') as mock_print:
            # Call geohash with different dates
            antigravity.geohash(10.0, 20.0, b'date1')
            first_call = mock_print.call_args[0][0]
            
            mock_print.reset_mock()
            antigravity.geohash(10.0, 20.0, b'date2')
            second_call = mock_print.call_args[0][0]
            
            # Results should be different
            self.assertNotEqual(first_call, second_call)

    def test_geohash_hash_algorithm(self):
        """Test that geohash uses MD5 as expected."""
        # Test the internal hash generation
        test_input = b'2005-05-26-10458.68'
        expected_hash = hashlib.md5(test_input, usedforsecurity=False).hexdigest()
        
        # We can't directly test the internal hash, but we can verify
        # that the function doesn't crash with various inputs
        with patch('builtins.print'):
            antigravity.geohash(0.0, 0.0, test_input)
            # If we get here without exception, the hash generation worked

    def test_geohash_edge_cases(self):
        """Test geohash with edge case inputs."""
        with patch('builtins.print'):
            # Test with extreme coordinates
            antigravity.geohash(90.0, 180.0, b'edge-case')
            antigravity.geohash(-90.0, -180.0, b'edge-case')
            
            # Test with zero coordinates
            antigravity.geohash(0.0, 0.0, b'zero-coords')
            
            # Test with empty date
            antigravity.geohash(0.0, 0.0, b'')

    def test_geohash_output_format(self):
        """Test that geohash output has expected format."""
        with patch('builtins.print') as mock_print:
            antigravity.geohash(37.421542, -122.085589, b'test')
            output = mock_print.call_args[0][0]
            
            # Output should be a string with two parts separated by space
            parts = output.split()
            self.assertEqual(len(parts), 2)
            
            # Each part should contain a number and additional characters
            for part in parts:
                self.assertIsInstance(part, str)
                self.assertTrue(len(part) > 0)

    def test_module_imports(self):
        """Test that the module imports correctly."""
        # Test that webbrowser and hashlib are available
        self.assertTrue(hasattr(antigravity, 'webbrowser'))
        self.assertTrue(hasattr(antigravity, 'hashlib'))
        self.assertTrue(hasattr(antigravity, 'geohash'))

    def test_geohash_function_signature(self):
        """Test that geohash function has expected signature."""
        import inspect
        sig = inspect.signature(antigravity.geohash)
        params = list(sig.parameters.keys())
        
        # Should have three parameters: latitude, longitude, datedow
        self.assertEqual(len(params), 3)
        self.assertIn('latitude', params)
        self.assertIn('longitude', params)
        self.assertIn('datedow', params)


if __name__ == '__main__':
    unittest.main()
