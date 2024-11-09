import ctypes
import gc
import sys
import unittest
from ctypes import POINTER, byref, c_void_p
from ctypes.wintypes import BOOL, BYTE, DWORD, WORD

COINIT_APARTMENTTHREADED = 0x2
CLSCTX_SERVER = 5
S_OK = 0
OUT = 2
TRUE = 1
E_NOINTERFACE = -2147467262

PyInstanceMethod_New = ctypes.pythonapi.PyInstanceMethod_New
PyInstanceMethod_New.argtypes = [ctypes.py_object]
PyInstanceMethod_New.restype = ctypes.py_object
PyInstanceMethod_Type = type(PyInstanceMethod_New(id))


class GUID(ctypes.Structure):
    _fields_ = [
        ("Data1", DWORD),
        ("Data2", WORD),
        ("Data3", WORD),
        ("Data4", BYTE * 8),
    ]


class ProtoComMethod:
    """Utilities for tests that define COM methods."""

    def __init__(self, index, restype, *argtypes):
        self.index = index
        self.proto = ctypes.WINFUNCTYPE(restype, *argtypes)

    def __set_name__(self, owner, name):
        foreign_func = self.proto(self.index, name, *self.args)
        self.mth = PyInstanceMethod_Type(foreign_func)

    # NOTE: To aid understanding, adding type annotations with `typing`
    # would look like this.
    #
    # _ParamFlags = None | tuple[tuple[int, str] | tuple[int, str, Any], ...]
    #
    # @overload
    # def __call__(self, paramflags: _ParamFlags, iid: GUID, /) -> Self: ...
    # @overload
    # def __call__(self, paramflags: _ParamFlags, /) -> Self: ...
    # @overload
    # def __call__(self) -> Self: ...
    def __call__(self, *args):
        self.args = args
        return self

    def __get__(self, instance, owner=None):
        # if instance is None:
        #     return self
        # NOTE: In this test, there is no need to define behavior for the
        # custom descriptor as a class attribute, so the above implementation
        # is omitted.
        return self.mth.__get__(instance)


def CLSIDFromString(name):
    guid = GUID()
    ole32.CLSIDFromString(name, byref(guid))
    return guid


def IsEqualGUID(guid1, guid2):
    return ole32.IsEqualGUID(byref(guid1), byref(guid2))


if sys.platform == "win32":
    from _ctypes import COMError
    from ctypes import HRESULT

    ole32 = ctypes.oledll.ole32

    IID_IUnknown = CLSIDFromString("{00000000-0000-0000-C000-000000000046}")
    IID_IStream = CLSIDFromString("{0000000C-0000-0000-C000-000000000046}")
    IID_IPersist = CLSIDFromString("{0000010C-0000-0000-C000-000000000046}")
    CLSID_ShellLink = CLSIDFromString("{00021401-0000-0000-C000-000000000046}")

    proto_qi = ProtoComMethod(0, HRESULT, POINTER(GUID), POINTER(c_void_p))
    proto_addref = ProtoComMethod(1, ctypes.c_long)
    proto_release = ProtoComMethod(2, ctypes.c_long)
    proto_get_class_id = ProtoComMethod(3, HRESULT, POINTER(GUID))


@unittest.skipUnless(sys.platform == "win32", "Windows-specific test")
class ForeignFunctionsThatWillCallComMethodsTests(unittest.TestCase):
    def setUp(self):
        ole32.CoInitializeEx(None, COINIT_APARTMENTTHREADED)

    def tearDown(self):
        ole32.CoUninitialize()
        gc.collect()

    @staticmethod
    def create_shelllink_persist(typ):
        ppst = typ()
        ole32.CoCreateInstance(
            byref(CLSID_ShellLink),
            None,
            CLSCTX_SERVER,
            byref(IID_IPersist),
            byref(ppst),
        )
        return ppst

    def test_without_paramflags_and_iid(self):
        class IUnknown(c_void_p):
            QueryInterface = proto_qi()
            AddRef = proto_addref()
            Release = proto_release()

        class IPersist(IUnknown):
            GetClassID = proto_get_class_id()

        ppst = self.create_shelllink_persist(IPersist)

        clsid = GUID()
        hr_getclsid = ppst.GetClassID(byref(clsid))
        self.assertEqual(S_OK, hr_getclsid)
        self.assertEqual(TRUE, IsEqualGUID(CLSID_ShellLink, clsid))

        self.assertEqual(2, ppst.AddRef())
        self.assertEqual(3, ppst.AddRef())

        punk = IUnknown()
        hr_qi = ppst.QueryInterface(IID_IUnknown, punk)
        self.assertEqual(S_OK, hr_qi)
        self.assertEqual(3, punk.Release())

        with self.assertRaises(WindowsError) as e:
            punk.QueryInterface(IID_IStream, IUnknown())
        self.assertEqual(E_NOINTERFACE, e.exception.winerror)

        self.assertEqual(2, ppst.Release())
        self.assertEqual(1, ppst.Release())
        self.assertEqual(0, ppst.Release())

    def test_with_paramflags_and_without_iid(self):
        class IUnknown(c_void_p):
            QueryInterface = proto_qi(None)
            AddRef = proto_addref()
            Release = proto_release()

        class IPersist(IUnknown):
            GetClassID = proto_get_class_id(((OUT, "pClassID"),))

        ppst = self.create_shelllink_persist(IPersist)

        clsid = ppst.GetClassID()
        self.assertEqual(TRUE, IsEqualGUID(CLSID_ShellLink, clsid))

        punk = IUnknown()
        hr_qi = ppst.QueryInterface(IID_IUnknown, punk)
        self.assertEqual(S_OK, hr_qi)
        self.assertEqual(1, punk.Release())

        with self.assertRaises(WindowsError) as e:
            ppst.QueryInterface(IID_IStream, IUnknown())
        self.assertEqual(E_NOINTERFACE, e.exception.winerror)

        self.assertEqual(0, ppst.Release())

    def test_with_paramflags_and_iid(self):
        class IUnknown(c_void_p):
            QueryInterface = proto_qi(None, IID_IUnknown)
            AddRef = proto_addref()
            Release = proto_release()

        class IPersist(IUnknown):
            GetClassID = proto_get_class_id(((OUT, "pClassID"),), IID_IPersist)

        ppst = self.create_shelllink_persist(IPersist)

        clsid = ppst.GetClassID()
        self.assertEqual(TRUE, IsEqualGUID(CLSID_ShellLink, clsid))

        punk = IUnknown()
        hr_qi = ppst.QueryInterface(IID_IUnknown, punk)
        self.assertEqual(S_OK, hr_qi)
        self.assertEqual(1, punk.Release())

        with self.assertRaises(COMError) as e:
            ppst.QueryInterface(IID_IStream, IUnknown())
        self.assertEqual(E_NOINTERFACE, e.exception.hresult)

        self.assertEqual(0, ppst.Release())
