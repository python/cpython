import ctypes
import gc
import sys
import unittest
from ctypes import POINTER, byref, c_void_p
from ctypes.wintypes import BYTE, DWORD, WORD

if sys.platform != "win32":
    raise unittest.SkipTest("Windows-specific test")


from ctypes import COMError, CopyComPointer, HRESULT


COINIT_APARTMENTTHREADED = 0x2
CLSCTX_SERVER = 5
S_OK = 0
OUT = 2
TRUE = 1
E_NOINTERFACE = -2147467262


class GUID(ctypes.Structure):
    # https://learn.microsoft.com/en-us/windows/win32/api/guiddef/ns-guiddef-guid
    _fields_ = [
        ("Data1", DWORD),
        ("Data2", WORD),
        ("Data3", WORD),
        ("Data4", BYTE * 8),
    ]


def create_proto_com_method(name, index, restype, *argtypes):
    proto = ctypes.WINFUNCTYPE(restype, *argtypes)

    def make_method(*args):
        foreign_func = proto(index, name, *args)

        def call(self, *args, **kwargs):
            return foreign_func(self, *args, **kwargs)

        return call

    return make_method


def create_guid(name):
    guid = GUID()
    # https://learn.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-clsidfromstring
    ole32.CLSIDFromString(name, byref(guid))
    return guid


def is_equal_guid(guid1, guid2):
    # https://learn.microsoft.com/en-us/windows/win32/api/objbase/nf-objbase-isequalguid
    return ole32.IsEqualGUID(byref(guid1), byref(guid2))


ole32 = ctypes.oledll.ole32

IID_IUnknown = create_guid("{00000000-0000-0000-C000-000000000046}")
IID_IStream = create_guid("{0000000C-0000-0000-C000-000000000046}")
IID_IPersist = create_guid("{0000010C-0000-0000-C000-000000000046}")
CLSID_ShellLink = create_guid("{00021401-0000-0000-C000-000000000046}")

# https://learn.microsoft.com/en-us/windows/win32/api/unknwn/nf-unknwn-iunknown-queryinterface(refiid_void)
proto_query_interface = create_proto_com_method(
    "QueryInterface", 0, HRESULT, POINTER(GUID), POINTER(c_void_p)
)
# https://learn.microsoft.com/en-us/windows/win32/api/unknwn/nf-unknwn-iunknown-addref
proto_add_ref = create_proto_com_method("AddRef", 1, ctypes.c_long)
# https://learn.microsoft.com/en-us/windows/win32/api/unknwn/nf-unknwn-iunknown-release
proto_release = create_proto_com_method("Release", 2, ctypes.c_long)
# https://learn.microsoft.com/en-us/windows/win32/api/objidl/nf-objidl-ipersist-getclassid
proto_get_class_id = create_proto_com_method(
    "GetClassID", 3, HRESULT, POINTER(GUID)
)


def create_shelllink_persist(typ):
    ppst = typ()
    # https://learn.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-cocreateinstance
    ole32.CoCreateInstance(
        byref(CLSID_ShellLink),
        None,
        CLSCTX_SERVER,
        byref(IID_IPersist),
        byref(ppst),
    )
    return ppst


class ForeignFunctionsThatWillCallComMethodsTests(unittest.TestCase):
    def setUp(self):
        # https://learn.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-coinitializeex
        ole32.CoInitializeEx(None, COINIT_APARTMENTTHREADED)

    def tearDown(self):
        # https://learn.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-couninitialize
        ole32.CoUninitialize()
        gc.collect()

    def test_without_paramflags_and_iid(self):
        class IUnknown(c_void_p):
            QueryInterface = proto_query_interface()
            AddRef = proto_add_ref()
            Release = proto_release()

        class IPersist(IUnknown):
            GetClassID = proto_get_class_id()

        ppst = create_shelllink_persist(IPersist)

        clsid = GUID()
        hr_getclsid = ppst.GetClassID(byref(clsid))
        self.assertEqual(S_OK, hr_getclsid)
        self.assertEqual(TRUE, is_equal_guid(CLSID_ShellLink, clsid))

        self.assertEqual(2, ppst.AddRef())
        self.assertEqual(3, ppst.AddRef())

        punk = IUnknown()
        hr_qi = ppst.QueryInterface(IID_IUnknown, punk)
        self.assertEqual(S_OK, hr_qi)
        self.assertEqual(3, punk.Release())

        with self.assertRaises(OSError) as e:
            punk.QueryInterface(IID_IStream, IUnknown())
        self.assertEqual(E_NOINTERFACE, e.exception.winerror)

        self.assertEqual(2, ppst.Release())
        self.assertEqual(1, ppst.Release())
        self.assertEqual(0, ppst.Release())

    def test_with_paramflags_and_without_iid(self):
        class IUnknown(c_void_p):
            QueryInterface = proto_query_interface(None)
            AddRef = proto_add_ref()
            Release = proto_release()

        class IPersist(IUnknown):
            GetClassID = proto_get_class_id(((OUT, "pClassID"),))

        ppst = create_shelllink_persist(IPersist)

        clsid = ppst.GetClassID()
        self.assertEqual(TRUE, is_equal_guid(CLSID_ShellLink, clsid))

        punk = IUnknown()
        hr_qi = ppst.QueryInterface(IID_IUnknown, punk)
        self.assertEqual(S_OK, hr_qi)
        self.assertEqual(1, punk.Release())

        with self.assertRaises(OSError) as e:
            ppst.QueryInterface(IID_IStream, IUnknown())
        self.assertEqual(E_NOINTERFACE, e.exception.winerror)

        self.assertEqual(0, ppst.Release())

    def test_with_paramflags_and_iid(self):
        class IUnknown(c_void_p):
            QueryInterface = proto_query_interface(None, IID_IUnknown)
            AddRef = proto_add_ref()
            Release = proto_release()

        class IPersist(IUnknown):
            GetClassID = proto_get_class_id(((OUT, "pClassID"),), IID_IPersist)

        ppst = create_shelllink_persist(IPersist)

        clsid = ppst.GetClassID()
        self.assertEqual(TRUE, is_equal_guid(CLSID_ShellLink, clsid))

        punk = IUnknown()
        hr_qi = ppst.QueryInterface(IID_IUnknown, punk)
        self.assertEqual(S_OK, hr_qi)
        self.assertEqual(1, punk.Release())

        with self.assertRaises(COMError) as e:
            ppst.QueryInterface(IID_IStream, IUnknown())
        self.assertEqual(E_NOINTERFACE, e.exception.hresult)

        self.assertEqual(0, ppst.Release())


class CopyComPointerTests(unittest.TestCase):
    def setUp(self):
        ole32.CoInitializeEx(None, COINIT_APARTMENTTHREADED)

        class IUnknown(c_void_p):
            QueryInterface = proto_query_interface(None, IID_IUnknown)
            AddRef = proto_add_ref()
            Release = proto_release()

        class IPersist(IUnknown):
            GetClassID = proto_get_class_id(((OUT, "pClassID"),), IID_IPersist)

        self.IUnknown = IUnknown
        self.IPersist = IPersist

    def tearDown(self):
        ole32.CoUninitialize()
        gc.collect()

    def test_both_are_null(self):
        src = self.IPersist()
        dst = self.IPersist()

        hr = CopyComPointer(src, byref(dst))

        self.assertEqual(S_OK, hr)

        self.assertIsNone(src.value)
        self.assertIsNone(dst.value)

    def test_src_is_nonnull_and_dest_is_null(self):
        # The reference count of the COM pointer created by `CoCreateInstance`
        # is initially 1.
        src = create_shelllink_persist(self.IPersist)
        dst = self.IPersist()

        # `CopyComPointer` calls `AddRef` explicitly in the C implementation.
        # The refcount of `src` is incremented from 1 to 2 here.
        hr = CopyComPointer(src, byref(dst))

        self.assertEqual(S_OK, hr)
        self.assertEqual(src.value, dst.value)

        # This indicates that the refcount was 2 before the `Release` call.
        self.assertEqual(1, src.Release())

        clsid = dst.GetClassID()
        self.assertEqual(TRUE, is_equal_guid(CLSID_ShellLink, clsid))

        self.assertEqual(0, dst.Release())

    def test_src_is_null_and_dest_is_nonnull(self):
        src = self.IPersist()
        dst_orig = create_shelllink_persist(self.IPersist)
        dst = self.IPersist()
        CopyComPointer(dst_orig, byref(dst))
        self.assertEqual(1, dst_orig.Release())

        clsid = dst.GetClassID()
        self.assertEqual(TRUE, is_equal_guid(CLSID_ShellLink, clsid))

        # This does NOT affects the refcount of `dst_orig`.
        hr = CopyComPointer(src, byref(dst))

        self.assertEqual(S_OK, hr)
        self.assertIsNone(dst.value)

        with self.assertRaises(ValueError):
            dst.GetClassID()  # NULL COM pointer access

        # This indicates that the refcount was 1 before the `Release` call.
        self.assertEqual(0, dst_orig.Release())

    def test_both_are_nonnull(self):
        src = create_shelllink_persist(self.IPersist)
        dst_orig = create_shelllink_persist(self.IPersist)
        dst = self.IPersist()
        CopyComPointer(dst_orig, byref(dst))
        self.assertEqual(1, dst_orig.Release())

        self.assertEqual(dst.value, dst_orig.value)
        self.assertNotEqual(src.value, dst.value)

        hr = CopyComPointer(src, byref(dst))

        self.assertEqual(S_OK, hr)
        self.assertEqual(src.value, dst.value)
        self.assertNotEqual(dst.value, dst_orig.value)

        self.assertEqual(1, src.Release())

        clsid = dst.GetClassID()
        self.assertEqual(TRUE, is_equal_guid(CLSID_ShellLink, clsid))

        self.assertEqual(0, dst.Release())
        self.assertEqual(0, dst_orig.Release())


if __name__ == '__main__':
    unittest.main()
