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

    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_sys_audit(self):
        sys_audit = _testlimitedcapi.sys_audit

        audit_events = []
        def audit_hook(event, args):
            audit_events.append((event, args))
            return None

        import sys
        sys.addaudithook(audit_hook)

        try:
            result = sys_audit("cpython.run_command", "")
            self.assertEqual(result, 0)
            self.assertEqual(len(audit_events), 1)
            self.assertEqual(audit_events[-1][0], "cpython.run_command")
            self.assertEqual(audit_events[-1][1], ())

            result = sys_audit("open", "OOO", "test.txt", "r", 0)
            self.assertEqual(result, 0)
            self.assertEqual(len(audit_events), 2)
            self.assertEqual(audit_events[-1][0], "open")
            self.assertEqual(len(audit_events[-1][1]), 3)
            self.assertEqual(audit_events[-1][1][0], "test.txt")
            self.assertEqual(audit_events[-1][1][1], "r")
            self.assertEqual(audit_events[-1][1][2], 0)

            test_dict = {"key": "value"}
            test_list = [1, 2, 3]
            result = sys_audit("test.objects", "OO", test_dict, test_list)
            self.assertEqual(result, 0)
            self.assertEqual(len(audit_events), 3)
            self.assertEqual(audit_events[-1][0], "test.objects")
            self.assertEqual(audit_events[-1][1][0], test_dict)
            self.assertEqual(audit_events[-1][1][1], test_list)

            result = sys_audit("test.mixed_types", "OOO", "string", 42, 123456789)
            self.assertEqual(result, 0)
            self.assertEqual(len(audit_events), 4)
            self.assertEqual(audit_events[-1][0], "test.mixed_types")
            self.assertEqual(audit_events[-1][1][0], "string")
            self.assertEqual(audit_events[-1][1][1], 42)
            self.assertEqual(audit_events[-1][1][2], 123456789)

        finally:
            sys.audit_hooks = []

        result = sys_audit("cpython.run_file", "")
        self.assertEqual(result, 0)

        result = sys_audit("os.chdir", "(O)", "/tmp")
        self.assertEqual(result, 0)

        result = sys_audit("ctypes.dlopen", "O", "libc.so.6")
        self.assertEqual(result, 0)

        self.assertRaises(TypeError, sys_audit, 123, "O", "arg")
        self.assertRaises(TypeError, sys_audit, None, "O", "arg")
        self.assertRaises(TypeError, sys_audit, ["not", "a", "string"], "O", "arg")

        self.assertRaises(TypeError, sys_audit, "test.event", 456, "arg")
        self.assertRaises(TypeError, sys_audit, "test.event", None, "arg")
        self.assertRaises(TypeError, sys_audit, "test.event", {"format": "string"}, "arg")

    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_sys_audittuple(self):
        sys_audittuple = _testlimitedcapi.sys_audittuple

        # Test with audit hook to verify internal behavior
        audit_events = []
        def audit_hook(event, args):
            audit_events.append((event, args))
            return None

        import sys
        sys.addaudithook(audit_hook)

        try:
            result = sys_audittuple("cpython.run_command", ())
            self.assertEqual(result, 0)
            self.assertEqual(len(audit_events), 1)
            self.assertEqual(audit_events[-1][0], "cpython.run_command")
            self.assertEqual(audit_events[-1][1], ())

            result = sys_audittuple("os.chdir", ("/tmp",))
            self.assertEqual(result, 0)
            self.assertEqual(len(audit_events), 2)
            self.assertEqual(audit_events[-1][0], "os.chdir")
            self.assertEqual(audit_events[-1][1], ("/tmp",))

            result = sys_audittuple("open", ("test.txt", "r", 0))
            self.assertEqual(result, 0)
            self.assertEqual(len(audit_events), 3)
            self.assertEqual(audit_events[-1][0], "open")
            self.assertEqual(audit_events[-1][1], ("test.txt", "r", 0))

            test_dict = {"key": "value"}
            test_list = [1, 2, 3]
            result = sys_audittuple("test.objects", (test_dict, test_list))
            self.assertEqual(result, 0)
            self.assertEqual(len(audit_events), 4)
            self.assertEqual(audit_events[-1][0], "test.objects")
            self.assertEqual(audit_events[-1][1][0], test_dict)
            self.assertEqual(audit_events[-1][1][1], test_list)

            result = sys_audittuple("test.complex", ("text", 3.14, True, None))
            self.assertEqual(result, 0)
            self.assertEqual(len(audit_events), 5)
            self.assertEqual(audit_events[-1][0], "test.complex")
            self.assertEqual(audit_events[-1][1][0], "text")
            self.assertEqual(audit_events[-1][1][1], 3.14)
            self.assertEqual(audit_events[-1][1][2], True)
            self.assertEqual(audit_events[-1][1][3], None)

        finally:
            sys.audit_hooks = []

        result = sys_audittuple("cpython.run_file", ())
        self.assertEqual(result, 0)

        result = sys_audittuple("ctypes.dlopen", ("libc.so.6",))
        self.assertEqual(result, 0)

        result = sys_audittuple("sqlite3.connect", ("test.db",))
        self.assertEqual(result, 0)

        self.assertRaises(TypeError, sys_audittuple, 123, ("arg",))
        self.assertRaises(TypeError, sys_audittuple, None, ("arg",))
        self.assertRaises(TypeError, sys_audittuple, ["not", "a", "string"], ("arg",))

        self.assertRaises(TypeError, sys_audittuple, "test.event", "not_a_tuple")
        self.assertRaises(TypeError, sys_audittuple, "test.event", ["list", "not", "tuple"])
        self.assertRaises(TypeError, sys_audittuple, "test.event", {"dict": "not_tuple"})
        self.assertRaises(TypeError, sys_audittuple, "test.event", None)


if __name__ == "__main__":
    unittest.main()
