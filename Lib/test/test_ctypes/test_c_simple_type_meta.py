import unittest
import ctypes
from ctypes import POINTER, c_void_p

from ._support import PyCSimpleType


class PyCSimpleTypeAsMetaclassTest(unittest.TestCase):
    def tearDown(self):
        # to not leak references, we must clean _pointer_type_cache
        ctypes._reset_cache()

    def test_early_return_in_dunder_new_1(self):
        # Such an implementation is used in `IUnknown` of `comtypes`.

        class _ct_meta(type):
            def __new__(cls, name, bases, namespace):
                self = super().__new__(cls, name, bases, namespace)
                if bases == (c_void_p,):
                    return self
                if issubclass(self, _PtrBase):
                    return self
                if bases == (object,):
                    _ptr_bases = (self, _PtrBase)
                else:
                    _ptr_bases = (self, POINTER(bases[0]))
                p = _p_meta(f"POINTER({self.__name__})", _ptr_bases, {})
                ctypes._pointer_type_cache[self] = p
                return self

        class _p_meta(PyCSimpleType, _ct_meta):
            pass

        class _PtrBase(c_void_p, metaclass=_p_meta):
            pass

        class _CtBase(object, metaclass=_ct_meta):
            pass

        class _Sub(_CtBase):
            pass

        class _Sub2(_Sub):
            pass

        self.assertIsInstance(POINTER(_Sub2), _p_meta)
        self.assertTrue(issubclass(POINTER(_Sub2), _Sub2))
        self.assertTrue(issubclass(POINTER(_Sub2), POINTER(_Sub)))
        self.assertTrue(issubclass(POINTER(_Sub), POINTER(_CtBase)))
