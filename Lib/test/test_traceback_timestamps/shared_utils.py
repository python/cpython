"""
Shared utilities for traceback timestamps tests.
"""

import json
import sys


def get_builtin_exception_types():
    """Get all built-in exception types from the exception hierarchy."""
    exceptions = []

    def collect_exceptions(exc_class):
        if hasattr(__builtins__, exc_class.__name__) and issubclass(
            exc_class, BaseException
        ):
            exceptions.append(exc_class.__name__)
        for subclass in exc_class.__subclasses__():
            collect_exceptions(subclass)

    collect_exceptions(BaseException)
    return sorted(exceptions)


def create_exception_instance(exc_class_name):
    """Create an exception instance by name with appropriate arguments."""
    # Get the exception class by name
    if hasattr(__builtins__, exc_class_name):
        exc_class = getattr(__builtins__, exc_class_name)
    else:
        exc_class = getattr(sys.modules["builtins"], exc_class_name)

    # Create exception with appropriate arguments
    if exc_class_name in (
        "OSError",
        "IOError",
        "PermissionError",
        "FileNotFoundError",
        "FileExistsError",
        "IsADirectoryError",
        "NotADirectoryError",
        "InterruptedError",
        "ChildProcessError",
        "ConnectionError",
        "BrokenPipeError",
        "ConnectionAbortedError",
        "ConnectionRefusedError",
        "ConnectionResetError",
        "ProcessLookupError",
        "TimeoutError",
    ):
        return exc_class(2, "No such file or directory")
    elif exc_class_name == "UnicodeDecodeError":
        return exc_class("utf-8", b"\xff", 0, 1, "invalid start byte")
    elif exc_class_name == "UnicodeEncodeError":
        return exc_class("ascii", "\u1234", 0, 1, "ordinal not in range")
    elif exc_class_name == "UnicodeTranslateError":
        return exc_class("\u1234", 0, 1, "character maps to <undefined>")
    elif exc_class_name in ("SyntaxError", "IndentationError", "TabError"):
        return exc_class("invalid syntax", ("test.py", 1, 1, "bad code"))
    elif exc_class_name == "SystemExit":
        return exc_class(0)
    elif exc_class_name in (
        "KeyboardInterrupt",
        "StopIteration",
        "StopAsyncIteration",
        "GeneratorExit",
    ):
        return exc_class()
    else:
        try:
            return exc_class("Test message")
        except TypeError:
            return exc_class()


def run_subprocess_test(script_code, args, xopts=None, env_vars=None):
    """Run a test script in subprocess and return parsed JSON result."""
    from test.support import script_helper

    cmd_args = []
    if xopts:
        for opt in xopts:
            cmd_args.extend(["-X", opt])
    cmd_args.extend(["-c", script_code])
    cmd_args.extend(args)

    kwargs = {}
    if env_vars:
        kwargs.update(env_vars)

    result = script_helper.assert_python_ok(*cmd_args, **kwargs)
    return json.loads(result.out.decode())


def get_create_exception_code():
    """Return the create_exception_instance function code as a string."""
    return '''
def create_exception_instance(exc_class_name):
    """Create an exception instance by name with appropriate arguments."""
    import sys
    # Get the exception class by name
    if hasattr(__builtins__, exc_class_name):
        exc_class = getattr(__builtins__, exc_class_name)
    else:
        exc_class = getattr(sys.modules['builtins'], exc_class_name)

    # Create exception with appropriate arguments
    if exc_class_name in ('OSError', 'IOError', 'PermissionError', 'FileNotFoundError',
                          'FileExistsError', 'IsADirectoryError', 'NotADirectoryError',
                          'InterruptedError', 'ChildProcessError', 'ConnectionError',
                          'BrokenPipeError', 'ConnectionAbortedError', 'ConnectionRefusedError',
                          'ConnectionResetError', 'ProcessLookupError', 'TimeoutError'):
        return exc_class(2, "No such file or directory")
    elif exc_class_name == 'UnicodeDecodeError':
        return exc_class('utf-8', b'\\xff', 0, 1, 'invalid start byte')
    elif exc_class_name == 'UnicodeEncodeError':
        return exc_class('ascii', '\\u1234', 0, 1, 'ordinal not in range')
    elif exc_class_name == 'UnicodeTranslateError':
        return exc_class('\\u1234', 0, 1, 'character maps to <undefined>')
    elif exc_class_name in ('SyntaxError', 'IndentationError', 'TabError'):
        return exc_class("invalid syntax", ("test.py", 1, 1, "bad code"))
    elif exc_class_name == 'SystemExit':
        return exc_class(0)
    elif exc_class_name in ('KeyboardInterrupt', 'StopIteration', 'StopAsyncIteration', 'GeneratorExit'):
        return exc_class()
    else:
        try:
            return exc_class("Test message")
        except TypeError:
            return exc_class()
'''


PICKLE_TEST_SCRIPT = f"""
import pickle
import sys
import json

{get_create_exception_code()}

def test_exception_pickle(exc_class_name, with_timestamps=False):
    try:
        exc = create_exception_instance(exc_class_name)
        exc.custom_attr = "custom_value"

        if with_timestamps:
            exc.__timestamp_ns__ = 1234567890123456789

        pickled_data = pickle.dumps(exc, protocol=0)
        unpickled_exc = pickle.loads(pickled_data)

        result = {{
            'exception_type': type(unpickled_exc).__name__,
            'message': str(unpickled_exc),
            'has_custom_attr': hasattr(unpickled_exc, 'custom_attr'),
            'custom_attr_value': getattr(unpickled_exc, 'custom_attr', None),
            'has_timestamp': hasattr(unpickled_exc, '__timestamp_ns__'),
            'timestamp_value': getattr(unpickled_exc, '__timestamp_ns__', None),
        }}
        print(json.dumps(result))

    except Exception as e:
        error_result = {{'error': str(e), 'error_type': type(e).__name__}}
        print(json.dumps(error_result))

if __name__ == "__main__":
    exc_name = sys.argv[1]
    with_timestamps = len(sys.argv) > 2 and sys.argv[2] == 'with_timestamps'
    test_exception_pickle(exc_name, with_timestamps)
"""


TIMESTAMP_TEST_SCRIPT = f"""
import sys
import json
import traceback
import io

{get_create_exception_code()}

def test_exception_timestamp(exc_class_name):
    try:
        try:
            raise create_exception_instance(exc_class_name)
        except BaseException as exc:
            has_timestamp = hasattr(exc, '__timestamp_ns__')
            timestamp_value = getattr(exc, '__timestamp_ns__', None)

            traceback_io = io.StringIO()
            traceback.print_exc(file=traceback_io)
            traceback_output = traceback_io.getvalue()

            result = {{
                'exception_type': type(exc).__name__,
                'has_timestamp_attr': has_timestamp,
                'timestamp_value': timestamp_value,
                'timestamp_is_positive': timestamp_value > 0 if timestamp_value is not None else False,
                'traceback_has_timestamp': '<@' in traceback_output,
                'traceback_output': traceback_output
            }}
            print(json.dumps(result))

    except Exception as e:
        error_result = {{'error': str(e), 'error_type': type(e).__name__}}
        print(json.dumps(error_result))

if __name__ == "__main__":
    exc_name = sys.argv[1]
    test_exception_timestamp(exc_name)
"""
