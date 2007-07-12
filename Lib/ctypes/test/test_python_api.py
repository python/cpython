from ctypes import *
import unittest, sys
from ctypes.test import is_resource_enabled

################################################################
# This section should be moved into ctypes\__init__.py, when it's ready.

from _ctypes import PyObj_FromPtr

################################################################

from sys import getrefcount as grc
if sys.version_info > (2, 4):
    c_py_ssize_t = c_size_t
else:
    c_py_ssize_t = c_int

class PythonAPITestCase(unittest.TestCase):

    def test_PyString_FromStringAndSize(self):
        PyString_FromStringAndSize = pythonapi.PyString_FromStringAndSize

        PyString_FromStringAndSize.restype = py_object
        PyString_FromStringAndSize.argtypes = c_char_p, c_py_ssize_t

        self.failUnlessEqual(PyString_FromStringAndSize("abcdefghi", 3), "abc")

    def test_PyString_FromString(self):
        pythonapi.PyString_FromString.restype = py_object
        pythonapi.PyString_FromString.argtypes = (c_char_p,)

        s = "abc"
        refcnt = grc(s)
        pyob = pythonapi.PyString_FromString(s)
        self.failUnlessEqual(grc(s), refcnt)
        self.failUnlessEqual(s, pyob)
        del pyob
        self.failUnlessEqual(grc(s), refcnt)

    if is_resource_enabled("refcount"):
        # This test is unreliable, because it is possible that code in
        # unittest changes the refcount of the '42' integer.  So, it
        # is disabled by default.
        def test_PyLong_Long(self):
            ref42 = grc(42)
            pythonapi.PyLong_FromLong.restype = py_object
            self.failUnlessEqual(pythonapi.PyLong_FromLong(42), 42)

            self.failUnlessEqual(grc(42), ref42)

            pythonapi.PyLong_AsLong.argtypes = (py_object,)
            pythonapi.PyLong_AsLong.restype = c_long

            res = pythonapi.PyLong_AsLong(42)
            self.failUnlessEqual(grc(res), ref42 + 1)
            del res
            self.failUnlessEqual(grc(42), ref42)

    def test_PyObj_FromPtr(self):
        s = "abc def ghi jkl"
        ref = grc(s)
        # id(python-object) is the address
        pyobj = PyObj_FromPtr(id(s))
        self.failUnless(s is pyobj)

        self.failUnlessEqual(grc(s), ref + 1)
        del pyobj
        self.failUnlessEqual(grc(s), ref)

    def test_PyOS_snprintf(self):
        PyOS_snprintf = pythonapi.PyOS_snprintf
        PyOS_snprintf.argtypes = POINTER(c_char), c_size_t, c_char_p

        buf = c_buffer(256)
        PyOS_snprintf(buf, sizeof(buf), "Hello from %s", b"ctypes")
        self.failUnlessEqual(buf.value, "Hello from ctypes")

        PyOS_snprintf(buf, sizeof(buf), "Hello from %s (%d, %d, %d)", b"ctypes", 1, 2, 3)
        self.failUnlessEqual(buf.value, "Hello from ctypes (1, 2, 3)")

        # not enough arguments
        self.failUnlessRaises(TypeError, PyOS_snprintf, buf)

    def test_pyobject_repr(self):
        self.failUnlessEqual(repr(py_object()), "py_object(<NULL>)")
        self.failUnlessEqual(repr(py_object(42)), "py_object(42)")
        self.failUnlessEqual(repr(py_object(object)), "py_object(%r)" % object)

if __name__ == "__main__":
    unittest.main()
