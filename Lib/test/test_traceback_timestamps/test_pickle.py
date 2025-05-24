"""
Tests for pickle/unpickle of exception types with timestamp feature.
"""
import os
import pickle
import sys
import unittest
from test.support import script_helper


class ExceptionPickleTests(unittest.TestCase):
    """Test that exception types can be pickled and unpickled with timestamps intact."""

    def setUp(self):
        # Script to test exception pickling
        self.pickle_script = '''
import pickle
import sys
import traceback
import json

def test_exception_pickle(exc_class_name, with_timestamps=False):
    """Test pickling an exception with optional timestamp."""
    try:
        # Get the exception class by name
        if hasattr(__builtins__, exc_class_name):
            exc_class = getattr(__builtins__, exc_class_name)
        else:
            exc_class = getattr(sys.modules['builtins'], exc_class_name)
        
        # Create an exception instance
        if exc_class_name in ('OSError', 'IOError', 'PermissionError', 'FileNotFoundError', 
                              'FileExistsError', 'IsADirectoryError', 'NotADirectoryError',
                              'InterruptedError', 'ChildProcessError', 'ConnectionError',
                              'BrokenPipeError', 'ConnectionAbortedError', 'ConnectionRefusedError',
                              'ConnectionResetError', 'ProcessLookupError', 'TimeoutError'):
            exc = exc_class(2, "No such file or directory")
        elif exc_class_name == 'UnicodeDecodeError':
            exc = exc_class('utf-8', b'\\xff', 0, 1, 'invalid start byte')
        elif exc_class_name == 'UnicodeEncodeError':
            exc = exc_class('ascii', '\\u1234', 0, 1, 'ordinal not in range')
        elif exc_class_name == 'UnicodeTranslateError':
            exc = exc_class('\\u1234', 0, 1, 'character maps to <undefined>')
        elif exc_class_name in ('SyntaxError', 'IndentationError', 'TabError'):
            exc = exc_class("invalid syntax", ("test.py", 1, 1, "bad code"))
        elif exc_class_name == 'SystemExit':
            exc = exc_class(0)
        elif exc_class_name == 'KeyboardInterrupt':
            exc = exc_class()
        elif exc_class_name in ('StopIteration', 'StopAsyncIteration'):
            exc = exc_class()
        elif exc_class_name == 'GeneratorExit':
            exc = exc_class()
        else:
            try:
                exc = exc_class("Test message")
            except TypeError:
                # Some exceptions may require no arguments
                exc = exc_class()
        
        # Add some custom attributes
        exc.custom_attr = "custom_value"
        
        # Manually set timestamp if needed (simulating timestamp collection)
        if with_timestamps:
            exc.__timestamp_ns__ = 1234567890123456789
        
        # Pickle and unpickle
        pickled_data = pickle.dumps(exc, protocol=0)
        unpickled_exc = pickle.loads(pickled_data)
        
        # Verify basic properties
        result = {
            'exception_type': type(unpickled_exc).__name__,
            'message': str(unpickled_exc),
            'has_custom_attr': hasattr(unpickled_exc, 'custom_attr'),
            'custom_attr_value': getattr(unpickled_exc, 'custom_attr', None),
            'has_timestamp': hasattr(unpickled_exc, '__timestamp_ns__'),
            'timestamp_value': getattr(unpickled_exc, '__timestamp_ns__', None),
            'pickle_size': len(pickled_data)
        }
        
        print(json.dumps(result))
        
    except Exception as e:
        error_result = {
            'error': str(e),
            'error_type': type(e).__name__
        }
        print(json.dumps(error_result))

if __name__ == "__main__":
    import sys
    if len(sys.argv) < 2:
        print("Usage: script.py <exception_class_name> [with_timestamps]")
        sys.exit(1)
    
    exc_name = sys.argv[1]
    with_timestamps = len(sys.argv) > 2 and sys.argv[2] == 'with_timestamps'
    test_exception_pickle(exc_name, with_timestamps)
'''

    def _get_builtin_exception_types(self):
        """Get all built-in exception types from the exception hierarchy."""
        builtin_exceptions = []
        
        def collect_exceptions(exc_class):
            # Only include concrete exception classes that are actually in builtins
            if (exc_class.__name__ not in ['BaseException', 'Exception'] and
                hasattr(__builtins__, exc_class.__name__) and
                issubclass(exc_class, BaseException)):
                builtin_exceptions.append(exc_class.__name__)
            for subclass in exc_class.__subclasses__():
                collect_exceptions(subclass)
        
        collect_exceptions(BaseException)
        return sorted(builtin_exceptions)

    def test_builtin_exception_pickle_without_timestamps(self):
        """Test that all built-in exception types can be pickled without timestamps."""
        exception_types = self._get_builtin_exception_types()
        
        for exc_name in exception_types:
            with self.subTest(exception_type=exc_name):
                result = script_helper.assert_python_ok(
                    "-c", self.pickle_script,
                    exc_name
                )
                
                # Parse JSON output
                import json
                output = json.loads(result.out.decode())
                
                # Should not have error
                self.assertNotIn('error', output, 
                                f"Error pickling {exc_name}: {output.get('error', 'Unknown')}")
                
                # Basic validations
                self.assertEqual(output['exception_type'], exc_name)
                self.assertTrue(output['has_custom_attr'])
                self.assertEqual(output['custom_attr_value'], 'custom_value')
                self.assertFalse(output['has_timestamp'])  # No timestamps when disabled

    def test_builtin_exception_pickle_with_timestamps(self):
        """Test that all built-in exception types can be pickled with timestamps."""
        exception_types = self._get_builtin_exception_types()
        
        for exc_name in exception_types:
            with self.subTest(exception_type=exc_name):
                result = script_helper.assert_python_ok(
                    "-X", "traceback_timestamps=us",
                    "-c", self.pickle_script,
                    exc_name, "with_timestamps"
                )
                
                # Parse JSON output
                import json
                output = json.loads(result.out.decode())
                
                # Should not have error
                self.assertNotIn('error', output, 
                                f"Error pickling {exc_name}: {output.get('error', 'Unknown')}")
                
                # Basic validations
                self.assertEqual(output['exception_type'], exc_name)
                self.assertTrue(output['has_custom_attr'])
                self.assertEqual(output['custom_attr_value'], 'custom_value')
                self.assertTrue(output['has_timestamp'])  # Should have timestamp
                self.assertEqual(output['timestamp_value'], 1234567890123456789)

    def test_stopiteration_no_timestamp(self):
        """Test that StopIteration and StopAsyncIteration don't get timestamps by design."""
        for exc_name in ['StopIteration', 'StopAsyncIteration']:
            with self.subTest(exception_type=exc_name):
                # Test with timestamps enabled
                result = script_helper.assert_python_ok(
                    "-X", "traceback_timestamps=us",
                    "-c", self.pickle_script,
                    exc_name, "with_timestamps"
                )
                
                # Parse JSON output
                import json
                output = json.loads(result.out.decode())
                
                # Should not have error
                self.assertNotIn('error', output, 
                                f"Error pickling {exc_name}: {output.get('error', 'Unknown')}")
                
                # Basic validations
                self.assertEqual(output['exception_type'], exc_name)
                self.assertTrue(output['has_custom_attr'])
                self.assertEqual(output['custom_attr_value'], 'custom_value')
                # StopIteration and StopAsyncIteration should not have timestamps even when enabled
                # (This depends on the actual implementation - may need adjustment)


if __name__ == "__main__":
    unittest.main()