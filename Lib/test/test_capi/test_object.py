import enum
import os
import sys
import textwrap
import unittest
from test import support
from test.support import import_helper
from test.support import os_helper
from test.support import threading_helper
from test.support.script_helper import assert_python_failure


_testlimitedcapi = import_helper.import_module('_testlimitedcapi')
_testcapi = import_helper.import_module('_testcapi')
_testinternalcapi = import_helper.import_module('_testinternalcapi')

NULL = None
STDERR_FD = 2


class Constant(enum.IntEnum):
    Py_CONSTANT_NONE = 0
    Py_CONSTANT_FALSE = 1
    Py_CONSTANT_TRUE = 2
    Py_CONSTANT_ELLIPSIS = 3
    Py_CONSTANT_NOT_IMPLEMENTED = 4
    Py_CONSTANT_ZERO = 5
    Py_CONSTANT_ONE = 6
    Py_CONSTANT_EMPTY_STR = 7
    Py_CONSTANT_EMPTY_BYTES = 8
    Py_CONSTANT_EMPTY_TUPLE = 9

    INVALID_CONSTANT = Py_CONSTANT_EMPTY_TUPLE + 1


class GetConstantTest(unittest.TestCase):
    def check_get_constant(self, get_constant):
        self.assertIs(get_constant(Constant.Py_CONSTANT_NONE), None)
        self.assertIs(get_constant(Constant.Py_CONSTANT_FALSE), False)
        self.assertIs(get_constant(Constant.Py_CONSTANT_TRUE), True)
        self.assertIs(get_constant(Constant.Py_CONSTANT_ELLIPSIS), Ellipsis)
        self.assertIs(get_constant(Constant.Py_CONSTANT_NOT_IMPLEMENTED), NotImplemented)

        for constant_id, constant_type, value in (
            (Constant.Py_CONSTANT_ZERO, int, 0),
            (Constant.Py_CONSTANT_ONE, int, 1),
            (Constant.Py_CONSTANT_EMPTY_STR, str, ""),
            (Constant.Py_CONSTANT_EMPTY_BYTES, bytes, b""),
            (Constant.Py_CONSTANT_EMPTY_TUPLE, tuple, ()),
        ):
            with self.subTest(constant_id=constant_id):
                obj = get_constant(constant_id)
                self.assertEqual(type(obj), constant_type, obj)
                self.assertEqual(obj, value)

        with self.assertRaises(SystemError):
            get_constant(Constant.INVALID_CONSTANT)

    def test_get_constant(self):
        self.check_get_constant(_testlimitedcapi.get_constant)

    def test_get_constant_borrowed(self):
        self.check_get_constant(_testlimitedcapi.get_constant_borrowed)


class PrintTest(unittest.TestCase):
    def testPyObjectPrintObject(self):

        class PrintableObject:

            def __repr__(self):
                return "spam spam spam"

            def __str__(self):
                return "egg egg egg"

        obj = PrintableObject()
        output_filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, output_filename)

        # Test repr printing
        _testcapi.call_pyobject_print(obj, output_filename, False)
        with open(output_filename, 'r') as output_file:
            self.assertEqual(output_file.read(), repr(obj))

        # Test str printing
        _testcapi.call_pyobject_print(obj, output_filename, True)
        with open(output_filename, 'r') as output_file:
            self.assertEqual(output_file.read(), str(obj))

    def testPyObjectPrintNULL(self):
        output_filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, output_filename)

        # Test repr printing
        _testcapi.pyobject_print_null(output_filename)
        with open(output_filename, 'r') as output_file:
            self.assertEqual(output_file.read(), '<nil>')

    def testPyObjectPrintNoRefObject(self):
        output_filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, output_filename)

        # Test repr printing
        correct_output = _testcapi.pyobject_print_noref_object(output_filename)
        with open(output_filename, 'r') as output_file:
            self.assertEqual(output_file.read(), correct_output)

    def testPyObjectPrintOSError(self):
        output_filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, output_filename)

        open(output_filename, "w+").close()
        with self.assertRaises(OSError):
            _testcapi.pyobject_print_os_error(output_filename)


class ClearWeakRefsNoCallbacksTest(unittest.TestCase):
    """Test PyUnstable_Object_ClearWeakRefsNoCallbacks"""
    def test_ClearWeakRefsNoCallbacks(self):
        """Ensure PyUnstable_Object_ClearWeakRefsNoCallbacks works"""
        import weakref
        import gc
        class C:
            pass
        obj = C()
        messages = []
        ref = weakref.ref(obj, lambda: messages.append("don't add this"))
        self.assertIs(ref(), obj)
        self.assertFalse(messages)
        _testcapi.pyobject_clear_weakrefs_no_callbacks(obj)
        self.assertIsNone(ref())
        gc.collect()
        self.assertFalse(messages)

    def test_ClearWeakRefsNoCallbacks_no_weakref_support(self):
        """Don't fail on objects that don't support weakrefs"""
        import weakref
        obj = object()
        with self.assertRaises(TypeError):
            ref = weakref.ref(obj)
        _testcapi.pyobject_clear_weakrefs_no_callbacks(obj)


@threading_helper.requires_working_threading()
class EnableDeferredRefcountingTest(unittest.TestCase):
    """Test PyUnstable_Object_EnableDeferredRefcount"""
    @support.requires_resource("cpu")
    def test_enable_deferred_refcount(self):
        from threading import Thread

        self.assertEqual(_testcapi.pyobject_enable_deferred_refcount("not tracked"), 0)
        foo = []
        self.assertEqual(_testcapi.pyobject_enable_deferred_refcount(foo), int(support.Py_GIL_DISABLED))

        # Make sure reference counting works on foo now
        self.assertEqual(foo, [])
        if support.Py_GIL_DISABLED:
            self.assertTrue(_testinternalcapi.has_deferred_refcount(foo))

        # Make sure that PyUnstable_Object_EnableDeferredRefcount is thread safe
        def silly_func(obj):
            self.assertIn(
                _testcapi.pyobject_enable_deferred_refcount(obj),
                (0, 1)
            )

        silly_list = [1, 2, 3]
        threads = [
            Thread(target=silly_func, args=(silly_list,)) for _ in range(4)
        ]

        with threading_helper.start_threads(threads):
            for i in range(10):
                silly_list.append(i)

        if support.Py_GIL_DISABLED:
            self.assertTrue(_testinternalcapi.has_deferred_refcount(silly_list))


class IsUniquelyReferencedTest(unittest.TestCase):
    """Test PyUnstable_Object_IsUniquelyReferenced"""
    def test_is_uniquely_referenced(self):
        self.assertTrue(_testcapi.is_uniquely_referenced(object()))
        self.assertTrue(_testcapi.is_uniquely_referenced([]))
        # Immortals
        self.assertFalse(_testcapi.is_uniquely_referenced(()))
        self.assertFalse(_testcapi.is_uniquely_referenced(42))
        # CRASHES is_uniquely_referenced(NULL)

class CAPITest(unittest.TestCase):
    def check_negative_refcount(self, code):
        # bpo-35059: Check that Py_DECREF() reports the correct filename
        # when calling _Py_NegativeRefcount() to abort Python.
        code = textwrap.dedent(code)
        rc, out, err = assert_python_failure('-c', code)
        self.assertRegex(err,
                         br'object\.c:[0-9]+: '
                         br'_Py_NegativeRefcount: Assertion failed: '
                         br'object has negative ref count')

    @unittest.skipUnless(hasattr(_testcapi, 'negative_refcount'),
                         'need _testcapi.negative_refcount()')
    def test_negative_refcount(self):
        code = """
            import _testcapi
            from test import support

            with support.SuppressCrashReport():
                _testcapi.negative_refcount()
        """
        self.check_negative_refcount(code)

    @unittest.skipUnless(hasattr(_testcapi, 'decref_freed_object'),
                         'need _testcapi.decref_freed_object()')
    @support.skip_if_sanitizer("use after free on purpose",
                               address=True, memory=True, ub=True)
    def test_decref_freed_object(self):
        code = """
            import _testcapi
            from test import support

            with support.SuppressCrashReport():
                _testcapi.decref_freed_object()
        """
        self.check_negative_refcount(code)

    @support.requires_resource('cpu')
    def test_decref_delayed(self):
        # gh-130519: Test that _PyObject_XDecRefDelayed() and QSBR code path
        # handles destructors that are possibly re-entrant or trigger a GC.
        import gc

        class MyObj:
            def __del__(self):
                gc.collect()

        for _ in range(1000):
            obj = MyObj()
            _testinternalcapi.incref_decref_delayed(obj)

    def test_is_unique_temporary(self):
        self.assertTrue(_testcapi.pyobject_is_unique_temporary(object()))
        obj = object()
        self.assertFalse(_testcapi.pyobject_is_unique_temporary(obj))

        def func(x):
            # This relies on the LOAD_FAST_BORROW optimization (gh-130704)
            self.assertEqual(sys.getrefcount(x), 1)
            self.assertFalse(_testcapi.pyobject_is_unique_temporary(x))

        func(object())

    def pyobject_dump(self, obj, release_gil=False):
        pyobject_dump = _testcapi.pyobject_dump

        try:
            old_stderr = os.dup(STDERR_FD)
        except OSError as exc:
            # os.dup(STDERR_FD) is not supported on WASI
            self.skipTest(f"os.dup() failed with {exc!r}")

        filename = os_helper.TESTFN
        try:
            try:
                with open(filename, "wb") as fp:
                    fd = fp.fileno()
                    os.dup2(fd, STDERR_FD)
                    pyobject_dump(obj, release_gil)
            finally:
                os.dup2(old_stderr, STDERR_FD)
                os.close(old_stderr)

            with open(filename) as fp:
                return fp.read().rstrip()
        finally:
            os_helper.unlink(filename)

    def test_pyobject_dump(self):
        # test string object
        str_obj = 'test string'
        output = self.pyobject_dump(str_obj)
        hex_regex = r'(0x)?[0-9a-fA-F]+'
        regex = (
            fr"object address  : {hex_regex}\n"
             r"object refcount : [0-9]+\n"
            fr"object type     : {hex_regex}\n"
             r"object type name: str\n"
             r"object repr     : 'test string'"
        )
        self.assertRegex(output, regex)

        # release the GIL
        output = self.pyobject_dump(str_obj, release_gil=True)
        self.assertRegex(output, regex)

        # test NULL object
        output = self.pyobject_dump(NULL)
        self.assertRegex(output, r'<object at .* is freed>')


if __name__ == "__main__":
    unittest.main()
