import unittest
import contextlib
import sys
from test import support
from test.support import import_helper

try:
    import _testlimitedcapi
except ImportError:
    _testlimitedcapi = None

NULL = None

class CAPITest(unittest.TestCase):
    # TODO: Test the following functions:
    #
    #   PySys_Audit()
    #   PySys_AuditTuple()

    maxDiff = None

    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_sys_getattr(self):
        # Test PySys_GetAttr()
        sys_getattr = _testlimitedcapi.sys_getattr

        self.assertIs(sys_getattr('stdout'), sys.stdout)
        with support.swap_attr(sys, '\U0001f40d', 42):
            self.assertEqual(sys_getattr('\U0001f40d'), 42)

        with self.assertRaisesRegex(RuntimeError, r'lost sys\.nonexistent'):
            sys_getattr('nonexistent')
        with self.assertRaisesRegex(RuntimeError, r'lost sys\.\U0001f40d'):
            sys_getattr('\U0001f40d')
        self.assertRaises(TypeError, sys_getattr, 1)
        self.assertRaises(TypeError, sys_getattr, [])
        # CRASHES sys_getattr(NULL)

    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_sys_getattrstring(self):
        # Test PySys_GetAttrString()
        getattrstring = _testlimitedcapi.sys_getattrstring

        self.assertIs(getattrstring(b'stdout'), sys.stdout)
        with support.swap_attr(sys, '\U0001f40d', 42):
            self.assertEqual(getattrstring('\U0001f40d'.encode()), 42)

        with self.assertRaisesRegex(RuntimeError, r'lost sys\.nonexistent'):
            getattrstring(b'nonexistent')
        with self.assertRaisesRegex(RuntimeError, r'lost sys\.\U0001f40d'):
            getattrstring('\U0001f40d'.encode())
        self.assertRaises(UnicodeDecodeError, getattrstring, b'\xff')
        # CRASHES getattrstring(NULL)

    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_sys_getoptionalattr(self):
        # Test PySys_GetOptionalAttr()
        getoptionalattr = _testlimitedcapi.sys_getoptionalattr

        self.assertIs(getoptionalattr('stdout'), sys.stdout)
        with support.swap_attr(sys, '\U0001f40d', 42):
            self.assertEqual(getoptionalattr('\U0001f40d'), 42)

        self.assertIs(getoptionalattr('nonexistent'), AttributeError)
        self.assertIs(getoptionalattr('\U0001f40d'), AttributeError)
        self.assertRaises(TypeError, getoptionalattr, 1)
        self.assertRaises(TypeError, getoptionalattr, [])
        # CRASHES getoptionalattr(NULL)

    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_sys_getoptionalattrstring(self):
        # Test PySys_GetOptionalAttrString()
        getoptionalattrstring = _testlimitedcapi.sys_getoptionalattrstring

        self.assertIs(getoptionalattrstring(b'stdout'), sys.stdout)
        with support.swap_attr(sys, '\U0001f40d', 42):
            self.assertEqual(getoptionalattrstring('\U0001f40d'.encode()), 42)

        self.assertIs(getoptionalattrstring(b'nonexistent'), AttributeError)
        self.assertIs(getoptionalattrstring('\U0001f40d'.encode()), AttributeError)
        self.assertRaises(UnicodeDecodeError, getoptionalattrstring, b'\xff')
        # CRASHES getoptionalattrstring(NULL)

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_sys_getobject(self):
        # Test PySys_GetObject()
        getobject = _testlimitedcapi.sys_getobject

        self.assertIs(getobject(b'stdout'), sys.stdout)
        with support.swap_attr(sys, '\U0001f40d', 42):
            self.assertEqual(getobject('\U0001f40d'.encode()), 42)

        self.assertIs(getobject(b'nonexistent'), AttributeError)
        with support.catch_unraisable_exception() as cm:
            self.assertIs(getobject(b'\xff'), AttributeError)
            self.assertEqual(cm.unraisable.exc_type, UnicodeDecodeError)
            self.assertRegex(str(cm.unraisable.exc_value),
                             "'utf-8' codec can't decode")
        # CRASHES getobject(NULL)

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_sys_setobject(self):
        # Test PySys_SetObject()
        setobject = _testlimitedcapi.sys_setobject

        value = ['value']
        value2 = ['value2']
        try:
            self.assertEqual(setobject(b'newattr', value), 0)
            self.assertIs(sys.newattr, value)
            self.assertEqual(setobject(b'newattr', value2), 0)
            self.assertIs(sys.newattr, value2)
            self.assertEqual(setobject(b'newattr', NULL), 0)
            self.assertNotHasAttr(sys, 'newattr')
            self.assertEqual(setobject(b'newattr', NULL), 0)
        finally:
            with contextlib.suppress(AttributeError):
                del sys.newattr
        try:
            self.assertEqual(setobject('\U0001f40d'.encode(), value), 0)
            self.assertIs(getattr(sys, '\U0001f40d'), value)
            self.assertEqual(setobject('\U0001f40d'.encode(), NULL), 0)
            self.assertNotHasAttr(sys, '\U0001f40d')
        finally:
            with contextlib.suppress(AttributeError):
                delattr(sys, '\U0001f40d')

        with self.assertRaises(UnicodeDecodeError):
            setobject(b'\xff', value)
        # CRASHES setobject(NULL, value)

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_sys_getxoptions(self):
        # Test PySys_GetXOptions()
        getxoptions = _testlimitedcapi.sys_getxoptions

        self.assertIs(getxoptions(), sys._xoptions)

        xoptions = sys._xoptions
        try:
            sys._xoptions = 'non-dict'
            self.assertEqual(getxoptions(), {})
            self.assertIs(getxoptions(), sys._xoptions)

            del sys._xoptions
            self.assertEqual(getxoptions(), {})
            self.assertIs(getxoptions(), sys._xoptions)
        finally:
            sys._xoptions = xoptions
        self.assertIs(getxoptions(), sys._xoptions)

    def _test_sys_formatstream(self, funname, streamname):
        import_helper.import_module('ctypes')
        from ctypes import pythonapi, c_char_p, py_object
        func = getattr(pythonapi, funname)
        func.argtypes = (c_char_p,)

        # Supports plain C types.
        with support.captured_output(streamname) as stream:
            func(b'Hello, %s!', c_char_p(b'world'))
        self.assertEqual(stream.getvalue(), 'Hello, world!')

        # Supports Python objects.
        with support.captured_output(streamname) as stream:
            func(b'Hello, %R!', py_object('world'))
        self.assertEqual(stream.getvalue(), "Hello, 'world'!")

        # The total length is not limited.
        with support.captured_output(streamname) as stream:
            func(b'Hello, %s!', c_char_p(b'world'*200))
        self.assertEqual(stream.getvalue(), 'Hello, ' + 'world'*200 + '!')

    def test_sys_formatstdout(self):
        # Test PySys_FormatStdout()
        self._test_sys_formatstream('PySys_FormatStdout', 'stdout')

    def test_sys_formatstderr(self):
        # Test PySys_FormatStderr()
        self._test_sys_formatstream('PySys_FormatStderr', 'stderr')

    def _test_sys_writestream(self, funname, streamname):
        import_helper.import_module('ctypes')
        from ctypes import pythonapi, c_char_p
        func = getattr(pythonapi, funname)
        func.argtypes = (c_char_p,)

        # Supports plain C types.
        with support.captured_output(streamname) as stream:
            func(b'Hello, %s!', c_char_p(b'world'))
        self.assertEqual(stream.getvalue(), 'Hello, world!')

        # There is a limit on the total length.
        with support.captured_output(streamname) as stream:
            func(b'Hello, %s!', c_char_p(b'world'*100))
        self.assertEqual(stream.getvalue(), 'Hello, ' + 'world'*100 + '!')
        with support.captured_output(streamname) as stream:
            func(b'Hello, %s!', c_char_p(b'world'*200))
        out = stream.getvalue()
        self.assertEqual(out[:20], 'Hello, worldworldwor')
        self.assertEqual(out[-13:], '... truncated')
        self.assertGreater(len(out), 1000)

    def test_sys_writestdout(self):
        # Test PySys_WriteStdout()
        self._test_sys_writestream('PySys_WriteStdout', 'stdout')

    def test_sys_writestderr(self):
        # Test PySys_WriteStderr()
        self._test_sys_writestream('PySys_WriteStderr', 'stderr')


if __name__ == "__main__":
    unittest.main()
