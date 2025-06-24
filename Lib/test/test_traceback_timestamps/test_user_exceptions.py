"""
Tests for user-derived exception classes with timestamp feature.
"""

import json
import unittest
from test.support import script_helper


USER_EXCEPTION_SCRIPT = """
import pickle
import sys
import json

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

def test_user_exception(exc_class_name):
    try:
        exc_class = globals()[exc_class_name]
        exc = exc_class(2, "No such file or directory") if exc_class_name == 'MyOSError' else exc_class()
        exc.extra_attr = "extra_value"

        pickled_data = pickle.dumps(exc, protocol=0)
        unpickled_exc = pickle.loads(pickled_data)

        result = {
            'exception_type': type(unpickled_exc).__name__,
            'base_class': type(unpickled_exc).__bases__[0].__name__,
            'has_custom_data': hasattr(unpickled_exc, 'custom_data'),
            'has_extra_attr': hasattr(unpickled_exc, 'extra_attr'),
            'extra_attr_value': getattr(unpickled_exc, 'extra_attr', None),
            'has_timestamp': hasattr(unpickled_exc, '__timestamp_ns__'),
            'timestamp_value': getattr(unpickled_exc, '__timestamp_ns__', None),
            'is_instance_of_base': isinstance(unpickled_exc, exc_class.__bases__[0])
        }
        print(json.dumps(result))

    except Exception as e:
        print(json.dumps({'error': str(e), 'error_type': type(e).__name__}))

if __name__ == "__main__":
    test_user_exception(sys.argv[1])
"""


class UserExceptionTests(unittest.TestCase):
    """Test user-derived exception classes from various base classes."""

    def test_user_derived_exceptions_without_timestamps(self):
        """Test user-derived exception classes without timestamps."""
        user_exceptions = ["MyBaseException", "MyException", "MyOSError", "MyImportError", "MyAttributeError"]
        expected_bases = {
            "MyBaseException": "BaseException",
            "MyException": "Exception",
            "MyOSError": "OSError",
            "MyImportError": "ImportError",
            "MyAttributeError": "AttributeError",
        }

        for exc_name in user_exceptions:
            with self.subTest(exception_type=exc_name):
                result = script_helper.assert_python_ok(
                    "-c", USER_EXCEPTION_SCRIPT, exc_name
                )
                output = json.loads(result.out.decode())

                self.assertNotIn(
                    "error",
                    output,
                    f"Error with {exc_name}: {output.get('error', 'Unknown')}",
                )
                self.assertEqual(output["exception_type"], exc_name)
                self.assertEqual(
                    output["base_class"], expected_bases[exc_name]
                )
                self.assertTrue(output["has_custom_data"])
                self.assertTrue(output["has_extra_attr"])
                self.assertEqual(output["extra_attr_value"], "extra_value")
                if output["has_timestamp"]:
                    self.assertEqual(output["timestamp_value"], 0)
                self.assertTrue(output["is_instance_of_base"])

    def test_user_derived_exceptions_with_timestamps(self):
        """Test user-derived exception classes with timestamps."""
        user_exceptions = ["MyBaseException", "MyException", "MyOSError", "MyImportError", "MyAttributeError"]
        expected_bases = {
            "MyBaseException": "BaseException",
            "MyException": "Exception",
            "MyOSError": "OSError",
            "MyImportError": "ImportError",
            "MyAttributeError": "AttributeError",
        }

        for exc_name in user_exceptions:
            with self.subTest(exception_type=exc_name):
                result = script_helper.assert_python_ok(
                    "-X",
                    "traceback_timestamps=us",
                    "-c",
                    USER_EXCEPTION_SCRIPT,
                    exc_name,
                )
                output = json.loads(result.out.decode())

                self.assertNotIn(
                    "error",
                    output,
                    f"Error with {exc_name}: {output.get('error', 'Unknown')}",
                )
                self.assertEqual(output["exception_type"], exc_name)
                self.assertEqual(
                    output["base_class"], expected_bases[exc_name]
                )
                self.assertTrue(output["has_custom_data"])
                self.assertTrue(output["has_extra_attr"])
                self.assertEqual(output["extra_attr_value"], "extra_value")
                self.assertTrue(output["has_timestamp"])
                self.assertIsNotNone(output["timestamp_value"])
                self.assertGreater(output["timestamp_value"], 0)
                self.assertTrue(output["is_instance_of_base"])

    def test_inheritance_chain_preservation(self):
        """Test that inheritance chain is preserved through pickle/unpickle."""
        inheritance_script = """
import pickle, json
class MyBaseException(BaseException): pass
class MySpecificException(MyBaseException):
    def __init__(self): super().__init__(); self.level = "specific"

exc = MySpecificException()
unpickled = pickle.loads(pickle.dumps(exc))
result = {
    'is_instance_of_MySpecificException': isinstance(unpickled, MySpecificException),
    'is_instance_of_MyBaseException': isinstance(unpickled, MyBaseException),
    'is_instance_of_BaseException': isinstance(unpickled, BaseException),
    'has_level_attr': hasattr(unpickled, 'level'),
    'level_value': getattr(unpickled, 'level', None)
}
print(json.dumps(result))
"""

        result = script_helper.assert_python_ok("-c", inheritance_script)
        output = json.loads(result.out.decode())

        self.assertTrue(output["is_instance_of_MySpecificException"])
        self.assertTrue(output["is_instance_of_MyBaseException"])
        self.assertTrue(output["is_instance_of_BaseException"])
        self.assertTrue(output["has_level_attr"])
        self.assertEqual(output["level_value"], "specific")


if __name__ == "__main__":
    unittest.main()
