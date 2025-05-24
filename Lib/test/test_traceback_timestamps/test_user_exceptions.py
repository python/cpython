"""
Tests for user-derived exception classes with timestamp feature.
"""
import json
import sys
import unittest
from test.support import script_helper


class UserExceptionTests(unittest.TestCase):
    """Test user-derived exception classes from various base classes."""

    def setUp(self):
        # Script to test user-derived exceptions
        self.user_exception_script = '''
import pickle
import sys
import json

# Define user-derived exception classes
class MyBaseException(BaseException):
    def __init__(self, message="MyBaseException"):
        super().__init__(message)
        self.custom_data = "base_exception_data"

class MyException(Exception):
    def __init__(self, message="MyException"):
        super().__init__(message)
        self.custom_data = "exception_data"

class MyOSError(OSError):
    def __init__(self, errno=None, strerror=None):
        if errno is not None and strerror is not None:
            super().__init__(errno, strerror)
        else:
            super().__init__("MyOSError")
        self.custom_data = "os_error_data"

class MyImportError(ImportError):
    def __init__(self, message="MyImportError"):
        super().__init__(message)
        self.custom_data = "import_error_data"

class MyAttributeError(AttributeError):
    def __init__(self, message="MyAttributeError"):
        super().__init__(message)
        self.custom_data = "attribute_error_data"

def test_user_exception(exc_class_name, with_timestamps=False):
    """Test a user-defined exception class."""
    try:
        # Get the exception class by name
        exc_class = globals()[exc_class_name]
        
        # Create an exception instance
        if exc_class_name == 'MyOSError':
            exc = exc_class(2, "No such file or directory")
        else:
            exc = exc_class()
        
        # Add additional custom attributes
        exc.extra_attr = "extra_value"
        
        # Note: The actual timestamp implementation may automatically set timestamps
        # when creating exceptions, depending on the traceback_timestamps setting.
        # We don't need to manually set timestamps here as the implementation handles it.
        
        # Pickle and unpickle
        pickled_data = pickle.dumps(exc, protocol=0)
        unpickled_exc = pickle.loads(pickled_data)
        
        # Verify all properties
        result = {
            'exception_type': type(unpickled_exc).__name__,
            'base_class': type(unpickled_exc).__bases__[0].__name__,
            'message': str(unpickled_exc),
            'has_custom_data': hasattr(unpickled_exc, 'custom_data'),
            'custom_data_value': getattr(unpickled_exc, 'custom_data', None),
            'has_extra_attr': hasattr(unpickled_exc, 'extra_attr'),
            'extra_attr_value': getattr(unpickled_exc, 'extra_attr', None),
            'has_timestamp': hasattr(unpickled_exc, '__timestamp_ns__'),
            'timestamp_value': getattr(unpickled_exc, '__timestamp_ns__', None),
            'pickle_size': len(pickled_data),
            'is_instance_of_base': isinstance(unpickled_exc, exc_class.__bases__[0])
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
        print("Usage: script.py <exception_class_name> [with_timestamps]")
        sys.exit(1)
    
    exc_name = sys.argv[1]
    with_timestamps = len(sys.argv) > 2 and sys.argv[2] == 'with_timestamps'
    test_user_exception(exc_name, with_timestamps)
'''

    def test_user_derived_exceptions_without_timestamps(self):
        """Test user-derived exception classes without timestamps."""
        user_exceptions = [
            'MyBaseException',
            'MyException', 
            'MyOSError',
            'MyImportError',
            'MyAttributeError'
        ]
        
        expected_bases = {
            'MyBaseException': 'BaseException',
            'MyException': 'Exception',
            'MyOSError': 'OSError',
            'MyImportError': 'ImportError',
            'MyAttributeError': 'AttributeError'
        }
        
        for exc_name in user_exceptions:
            with self.subTest(exception_type=exc_name):
                result = script_helper.assert_python_ok(
                    "-c", self.user_exception_script,
                    exc_name
                )
                
                # Parse JSON output
                output = json.loads(result.out.decode())
                
                # Should not have error
                self.assertNotIn('error', output, 
                                f"Error with {exc_name}: {output.get('error', 'Unknown')}")
                
                # Validate properties
                self.assertEqual(output['exception_type'], exc_name)
                self.assertEqual(output['base_class'], expected_bases[exc_name])
                self.assertTrue(output['has_custom_data'])
                self.assertTrue(output['has_extra_attr'])
                self.assertEqual(output['extra_attr_value'], 'extra_value')
                # The implementation may set timestamps to 0 when disabled
                # Check for the absence of meaningful timestamps  
                if output['has_timestamp']:
                    # If timestamp is 0, then it's effectively disabled
                    self.assertEqual(output['timestamp_value'], 0, 
                                   f"Expected 0 timestamp when disabled, got {output['timestamp_value']}")
                else:
                    self.assertFalse(output['has_timestamp'])  # No timestamps when disabled
                self.assertTrue(output['is_instance_of_base'])

    def test_user_derived_exceptions_with_timestamps(self):
        """Test user-derived exception classes with timestamps."""
        user_exceptions = [
            'MyBaseException',
            'MyException', 
            'MyOSError',
            'MyImportError',
            'MyAttributeError'
        ]
        
        expected_bases = {
            'MyBaseException': 'BaseException',
            'MyException': 'Exception',
            'MyOSError': 'OSError',
            'MyImportError': 'ImportError',
            'MyAttributeError': 'AttributeError'
        }
        
        for exc_name in user_exceptions:
            with self.subTest(exception_type=exc_name):
                result = script_helper.assert_python_ok(
                    "-X", "traceback_timestamps=us",
                    "-c", self.user_exception_script,
                    exc_name, "with_timestamps"
                )
                
                # Parse JSON output
                output = json.loads(result.out.decode())
                
                # Should not have error
                self.assertNotIn('error', output, 
                                f"Error with {exc_name}: {output.get('error', 'Unknown')}")
                
                # Validate properties
                self.assertEqual(output['exception_type'], exc_name)
                self.assertEqual(output['base_class'], expected_bases[exc_name])
                self.assertTrue(output['has_custom_data'])
                self.assertTrue(output['has_extra_attr'])
                self.assertEqual(output['extra_attr_value'], 'extra_value')
                self.assertTrue(output['has_timestamp'])  # Should have timestamp
                self.assertIsNotNone(output['timestamp_value'])
                self.assertGreater(output['timestamp_value'], 0)  # Should be a real timestamp
                self.assertTrue(output['is_instance_of_base'])

    def test_inheritance_chain_preservation(self):
        """Test that inheritance chain is preserved through pickle/unpickle."""
        inheritance_test_script = '''
import pickle
import json

class MyBaseException(BaseException):
    pass

class MySpecificException(MyBaseException):
    def __init__(self, message="MySpecificException"):
        super().__init__(message)
        self.level = "specific"

try:
    exc = MySpecificException()
    pickled = pickle.dumps(exc)
    unpickled = pickle.loads(pickled)
    
    result = {
        'is_instance_of_MySpecificException': isinstance(unpickled, MySpecificException),
        'is_instance_of_MyBaseException': isinstance(unpickled, MyBaseException),
        'is_instance_of_BaseException': isinstance(unpickled, BaseException),
        'mro': [cls.__name__ for cls in type(unpickled).__mro__],
        'has_level_attr': hasattr(unpickled, 'level'),
        'level_value': getattr(unpickled, 'level', None)
    }
    print(json.dumps(result))
    
except Exception as e:
    error_result = {'error': str(e), 'error_type': type(e).__name__}
    print(json.dumps(error_result))
'''
        
        result = script_helper.assert_python_ok("-c", inheritance_test_script)
        output = json.loads(result.out.decode())
        
        self.assertNotIn('error', output)
        self.assertTrue(output['is_instance_of_MySpecificException'])
        self.assertTrue(output['is_instance_of_MyBaseException'])
        self.assertTrue(output['is_instance_of_BaseException'])
        self.assertTrue(output['has_level_attr'])
        self.assertEqual(output['level_value'], 'specific')
        self.assertIn('MySpecificException', output['mro'])
        self.assertIn('MyBaseException', output['mro'])
        self.assertIn('BaseException', output['mro'])


if __name__ == "__main__":
    unittest.main()