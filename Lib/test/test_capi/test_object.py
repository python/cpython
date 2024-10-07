import enum
import unittest
from test import support
from test.support import import_helper
from test.support import os_helper
from threading import Thread

_testlimitedcapi = import_helper.import_module('_testlimitedcapi')
_testcapi = import_helper.import_module('_testcapi')


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

class EnableDeferredRefcountingTest(unittest.TestCase):
    """Test PyUnstable_Object_EnableDeferredRefcount"""
    def test_enable_deferred_refcount(self):
        if support.Py_GIL_DISABLED:
            with self.assertRaises(TypeError):
                _testcapi.pyobject_enable_deferred_refcount("not tracked")

        if support.Py_GIL_DISABLED:
            def foo(obj):
                obj.append(1)  # Do something with it from another thread

                self.assertEqual(_testcapi.pyobject_enable_deferred_refcount(obj), 0)

            x = []
            self.assertEqual(_testcapi.pyobject_enable_deferred_refcount(x), int(support.Py_GIL_DISABLED))
            threads = [Thread(target=foo, args=(x,)) for _ in range(5)]

            for thread in threads:
                thread.start()

            for thread in threads:
                thread.join()

        self.assertEqual(_testcapi.pyobject_enable_deferred_refcount([]), int(support.Py_GIL_DISABLED))

if __name__ == "__main__":
    unittest.main()
