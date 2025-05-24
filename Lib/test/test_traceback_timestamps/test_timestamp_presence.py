"""
Tests to verify timestamp presence on exception types.
"""
import json
import sys
import unittest
from test.support import script_helper


class TimestampPresenceTests(unittest.TestCase):
    """Test that timestamps show up when enabled on all exception types except StopIteration."""

    def setUp(self):
        # Script to test timestamp presence on exceptions
        self.timestamp_presence_script = '''
import sys
import json
import traceback

def test_exception_timestamp(exc_class_name):
    """Test if an exception gets a timestamp when timestamps are enabled."""
    try:
        # Get the exception class by name
        if hasattr(__builtins__, exc_class_name):
            exc_class = getattr(__builtins__, exc_class_name)
        else:
            exc_class = getattr(sys.modules['builtins'], exc_class_name)
        
        # Create and raise the exception to trigger timestamp collection
        try:
            # Create exception with appropriate arguments
            if exc_class_name in ('OSError', 'IOError', 'PermissionError', 'FileNotFoundError', 
                                  'FileExistsError', 'IsADirectoryError', 'NotADirectoryError',
                                  'InterruptedError', 'ChildProcessError', 'ConnectionError',
                                  'BrokenPipeError', 'ConnectionAbortedError', 'ConnectionRefusedError',
                                  'ConnectionResetError', 'ProcessLookupError', 'TimeoutError'):
                raise exc_class(2, "No such file or directory")
            elif exc_class_name == 'UnicodeDecodeError':
                raise exc_class('utf-8', b'\\xff', 0, 1, 'invalid start byte')
            elif exc_class_name == 'UnicodeEncodeError':
                raise exc_class('ascii', '\\u1234', 0, 1, 'ordinal not in range')
            elif exc_class_name == 'UnicodeTranslateError':
                raise exc_class('\\u1234', 0, 1, 'character maps to <undefined>')
            elif exc_class_name in ('SyntaxError', 'IndentationError', 'TabError'):
                raise exc_class("invalid syntax", ("test.py", 1, 1, "bad code"))
            elif exc_class_name == 'SystemExit':
                raise exc_class(0)
            elif exc_class_name == 'KeyboardInterrupt':
                raise exc_class()
            elif exc_class_name in ('StopIteration', 'StopAsyncIteration'):
                raise exc_class()
            elif exc_class_name == 'GeneratorExit':
                raise exc_class()
            else:
                try:
                    raise exc_class("Test message for " + exc_class_name)
                except TypeError:
                    # Some exceptions may require no arguments
                    raise exc_class()
                
        except BaseException as exc:
            # Check if the exception has a timestamp
            has_timestamp = hasattr(exc, '__timestamp_ns__')
            timestamp_value = getattr(exc, '__timestamp_ns__', None)
            
            # Get traceback with timestamps if enabled
            import io
            traceback_io = io.StringIO()
            traceback.print_exc(file=traceback_io)
            traceback_output = traceback_io.getvalue()
            
            result = {
                'exception_type': type(exc).__name__,
                'has_timestamp_attr': has_timestamp,
                'timestamp_value': timestamp_value,
                'timestamp_is_positive': timestamp_value > 0 if timestamp_value is not None else False,
                'traceback_has_timestamp': '<@' in traceback_output,
                'traceback_output': traceback_output
            }
            
            print(json.dumps(result))
        
    except Exception as e:
        error_result = {
            'error': str(e),
            'error_type': type(e).__name__
        }
        print(json.dumps(error_result))

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: script.py <exception_class_name>")
        sys.exit(1)
    
    exc_name = sys.argv[1]
    test_exception_timestamp(exc_name)
'''

    def _get_all_exception_types(self):
        """Get all built-in exception types from the exception hierarchy."""
        exceptions = []
        
        def collect_exceptions(exc_class):
            # Only include concrete exception classes that are actually in builtins
            if (hasattr(__builtins__, exc_class.__name__) and
                issubclass(exc_class, BaseException)):
                exceptions.append(exc_class.__name__)
            for subclass in exc_class.__subclasses__():
                collect_exceptions(subclass)
        
        collect_exceptions(BaseException)
        return sorted(exceptions)

    def test_timestamp_presence_when_enabled(self):
        """Test that timestamps are present on exceptions when feature is enabled."""
        exception_types = self._get_all_exception_types()
        
        # Remove abstract/special cases that can't be easily instantiated
        skip_types = {
            'BaseException',  # Abstract base
            'Exception',      # Abstract base  
            'Warning',        # Abstract base
            'GeneratorExit',  # Special case
        }
        
        exception_types = [exc for exc in exception_types if exc not in skip_types]
        
        for exc_name in exception_types:
            with self.subTest(exception_type=exc_name):
                result = script_helper.assert_python_ok(
                    "-X", "traceback_timestamps=us",
                    "-c", self.timestamp_presence_script,
                    exc_name
                )
                
                # Parse JSON output
                output = json.loads(result.out.decode())
                
                # Should not have error
                self.assertNotIn('error', output, 
                                f"Error testing {exc_name}: {output.get('error', 'Unknown')}")
                
                # Validate exception type
                self.assertEqual(output['exception_type'], exc_name)
                
                # Check timestamp behavior based on exception type
                if exc_name in ('StopIteration', 'StopAsyncIteration'):
                    # These should NOT have timestamps by design (performance optimization)
                    self.assertFalse(output['has_timestamp_attr'] and output['timestamp_is_positive'],
                                   f"{exc_name} should not have timestamp for performance reasons")
                    self.assertFalse(output['traceback_has_timestamp'],
                                   f"{exc_name} traceback should not show timestamp")
                else:
                    # All other exceptions should have timestamps when enabled
                    self.assertTrue(output['has_timestamp_attr'],
                                  f"{exc_name} should have __timestamp_ns__ attribute")
                    self.assertTrue(output['timestamp_is_positive'],
                                  f"{exc_name} should have positive timestamp value")
                    self.assertTrue(output['traceback_has_timestamp'],
                                  f"{exc_name} traceback should show timestamp")

    def test_no_timestamp_when_disabled(self):
        """Test that no timestamps are present when feature is disabled."""
        # Test a few representative exception types
        test_exceptions = ['ValueError', 'TypeError', 'RuntimeError', 'KeyError']
        
        for exc_name in test_exceptions:
            with self.subTest(exception_type=exc_name):
                result = script_helper.assert_python_ok(
                    "-c", self.timestamp_presence_script,
                    exc_name
                )
                
                # Parse JSON output
                output = json.loads(result.out.decode())
                
                # Should not have error
                self.assertNotIn('error', output)
                
                # Should not have timestamps when disabled
                self.assertFalse(output['has_timestamp_attr'] and output['timestamp_is_positive'],
                               f"{exc_name} should not have timestamp when disabled")
                self.assertFalse(output['traceback_has_timestamp'],
                               f"{exc_name} traceback should not show timestamp when disabled")

    def test_stopiteration_special_case(self):
        """Test that StopIteration and StopAsyncIteration never get timestamps."""
        for exc_name in ['StopIteration', 'StopAsyncIteration']:
            with self.subTest(exception_type=exc_name):
                # Test with timestamps explicitly enabled
                result = script_helper.assert_python_ok(
                    "-X", "traceback_timestamps=us",
                    "-c", self.timestamp_presence_script,
                    exc_name
                )
                
                # Parse JSON output
                output = json.loads(result.out.decode())
                
                # Should not have error
                self.assertNotIn('error', output)
                self.assertEqual(output['exception_type'], exc_name)
                
                # Should never have timestamps (performance optimization)
                self.assertFalse(output['has_timestamp_attr'] and output['timestamp_is_positive'],
                               f"{exc_name} should never have timestamp (performance optimization)")
                self.assertFalse(output['traceback_has_timestamp'],
                               f"{exc_name} traceback should never show timestamp")

    def test_timestamp_formats(self):
        """Test that different timestamp formats work correctly."""
        formats = ['us', 'ns', 'iso']
        
        for format_type in formats:
            with self.subTest(format=format_type):
                result = script_helper.assert_python_ok(
                    "-X", f"traceback_timestamps={format_type}",
                    "-c", self.timestamp_presence_script,
                    "ValueError"
                )
                
                # Parse JSON output
                output = json.loads(result.out.decode())
                
                # Should not have error
                self.assertNotIn('error', output)
                
                # Should have timestamp
                self.assertTrue(output['has_timestamp_attr'])
                self.assertTrue(output['timestamp_is_positive'])
                self.assertTrue(output['traceback_has_timestamp'])
                
                # Check format-specific patterns in traceback
                traceback_output = output['traceback_output']
                if format_type == 'us':
                    # Microsecond format: <@1234567890.123456>
                    self.assertRegex(traceback_output, r'<@\d+\.\d{6}>')
                elif format_type == 'ns':
                    # Nanosecond format: <@1234567890123456789ns>
                    self.assertRegex(traceback_output, r'<@\d+ns>')
                elif format_type == 'iso':
                    # ISO format: <@2023-04-13T12:34:56.789012Z>
                    self.assertRegex(traceback_output, r'<@\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{6}Z>')


if __name__ == "__main__":
    unittest.main()