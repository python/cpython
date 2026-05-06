"""Tests for pickle round-trips of exceptions with and without timestamps."""

import json
import unittest
from test.support import script_helper


# Representative builtin types covering different __reduce__ paths,
# plus both control-flow exclusions.
EXCEPTION_TYPES = [
    "ValueError",
    "OSError",
    "UnicodeDecodeError",
    "SyntaxError",
    "StopIteration",
    "StopAsyncIteration",
]

# Subprocess script that creates, pickles, unpickles, and reports on an
# exception.  Accepts exc_class_name as argv[1].  Optionally sets a
# nonzero timestamp if argv[2] == "ts".
PICKLE_SCRIPT = r'''
import json, pickle, sys

name = sys.argv[1]
set_ts = len(sys.argv) > 2 and sys.argv[2] == "ts"

cls = getattr(__builtins__, name, None) or getattr(
    __import__("builtins"), name)

match name:
    case "OSError":
        exc = cls("something went wrong")
    case "UnicodeDecodeError":
        exc = cls("utf-8", b"\xff", 0, 1, "invalid start byte")
    case "SyntaxError":
        exc = cls("bad", ("test.py", 1, 1, "x"))
    case _:
        exc = cls("test")

exc.custom = "val"
if set_ts:
    exc.__timestamp_ns__ = 1234567890123456789

data = pickle.dumps(exc, protocol=0)
restored = pickle.loads(data)

print(json.dumps({
    "type": type(restored).__name__,
    "custom": getattr(restored, "custom", None),
    "ts": restored.__timestamp_ns__,
}))
'''

# Subprocess script for user-defined exception classes.
USER_PICKLE_SCRIPT = r'''
import json, pickle, sys

set_ts = len(sys.argv) > 1 and sys.argv[1] == "ts"

class MyException(Exception):
    def __init__(self, msg="MyException"):
        super().__init__(msg)
        self.data = "custom"

class MyOSError(OSError):
    def __init__(self, *args):
        super().__init__(*args)
        self.data = "custom"

class Child(MyException):
    def __init__(self, msg="child"):
        super().__init__(msg)
        self.level = "child"

results = []
for cls in (MyException, MyOSError, Child):
    exc = cls()
    if set_ts:
        exc.__timestamp_ns__ = 99999

    restored = pickle.loads(pickle.dumps(exc, protocol=0))
    results.append({
        "type": type(restored).__name__,
        "data": getattr(restored, "data", None),
        "level": getattr(restored, "level", None),
        "ts": restored.__timestamp_ns__,
        "isinstance_base": isinstance(restored, BaseException),
    })

print(json.dumps(results))
'''


class BuiltinExceptionPickleTests(unittest.TestCase):

    def test_pickle_without_timestamps(self):
        for exc_name in EXCEPTION_TYPES:
            with self.subTest(exc_name):
                result = script_helper.assert_python_ok(
                    "-X", "traceback_timestamps=0",
                    "-c", PICKLE_SCRIPT, exc_name,
                )
                output = json.loads(result.out)
                self.assertEqual(output["type"], exc_name)
                self.assertEqual(output["custom"], "val")
                self.assertEqual(output["ts"], 0)

    def test_pickle_with_timestamps(self):
        for exc_name in EXCEPTION_TYPES:
            with self.subTest(exc_name):
                result = script_helper.assert_python_ok(
                    "-X", "traceback_timestamps=ns",
                    "-c", PICKLE_SCRIPT, exc_name, "ts",
                )
                output = json.loads(result.out)
                self.assertEqual(output["type"], exc_name)
                self.assertEqual(output["custom"], "val")
                self.assertEqual(output["ts"], 1234567890123456789)


class UserExceptionPickleTests(unittest.TestCase):

    def test_user_exceptions_without_timestamps(self):
        result = script_helper.assert_python_ok(
            "-X", "traceback_timestamps=0",
            "-c", USER_PICKLE_SCRIPT,
        )
        results = json.loads(result.out)
        for entry in results:
            with self.subTest(entry["type"]):
                self.assertEqual(entry["data"], "custom")
                self.assertTrue(entry["isinstance_base"])
                self.assertEqual(entry["ts"], 0)
        # Check the inheritance chain child
        self.assertEqual(results[2]["level"], "child")

    def test_user_exceptions_with_timestamps(self):
        result = script_helper.assert_python_ok(
            "-X", "traceback_timestamps=ns",
            "-c", USER_PICKLE_SCRIPT, "ts",
        )
        results = json.loads(result.out)
        for entry in results:
            with self.subTest(entry["type"]):
                self.assertEqual(entry["data"], "custom")
                self.assertTrue(entry["isinstance_base"])
                self.assertEqual(entry["ts"], 99999)


if __name__ == "__main__":
    unittest.main()
