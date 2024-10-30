import unittest
import ctypes
from ctypes import POINTER, c_void_p

from ._support import PyCSimpleType


class PyCSimpleTypeAsMetaclassTest(unittest.TestCase):
    def tearDown(self):
        # to not leak references, we must clean _pointer_type_cache
        ctypes._reset_cache()

    def test_creating_pointer_in_dunder_new_1(self):
        # Test metaclass whose instances are C types; when the type is
        # created it automatically creates a pointer type for itself.
        # The pointer type is also an instance of the metaclass.
        # Such an implementation is used in `IUnknown` of the `comtypes`
        # project. See gh-124520.

        class ct_meta(type):
            def __new__(cls, name, bases, namespace):
                self = super().__new__(cls, name, bases, namespace)

                # Avoid recursion: don't set up a pointer to
                # a pointer (to a pointer...)
                if bases == (c_void_p,):
                    # When creating PtrBase itself, the name
                    # is not yet available
                    return self
                if issubclass(self, PtrBase):
                    return self

                if bases == (object,):
                    ptr_bases = (self, PtrBase)
                else:
                    ptr_bases = (self, POINTER(bases[0]))
                p = p_meta(f"POINTER({self.__name__})", ptr_bases, {})
                ctypes._pointer_type_cache[self] = p
                return self

        class p_meta(PyCSimpleType, ct_meta):
            pass

        class PtrBase(c_void_p, metaclass=p_meta):
            pass

        class CtBase(object, metaclass=ct_meta):
            pass

        class Sub(CtBase):
            pass

        class Sub2(Sub):
            pass

        self.assertIsInstance(POINTER(Sub2), p_meta)
        self.assertTrue(issubclass(POINTER(Sub2), Sub2))
        self.assertTrue(issubclass(POINTER(Sub2), POINTER(Sub)))
        self.assertTrue(issubclass(POINTER(Sub), POINTER(CtBase)))

    def test_creating_pointer_in_dunder_new_2(self):
        # A simpler variant of the above, used in `CoClass` of the `comtypes`
        # project.

        class ct_meta(type):
            def __new__(cls, name, bases, namespace):
                self = super().__new__(cls, name, bases, namespace)
                if isinstance(self, p_meta):
                    return self
                p = p_meta(f"POINTER({self.__name__})", (self, c_void_p), {})
                ctypes._pointer_type_cache[self] = p
                return self

        class p_meta(PyCSimpleType, ct_meta):
            pass

        class Core(object):
            pass

        class CtBase(Core, metaclass=ct_meta):
            pass

        class Sub(CtBase):
            pass

        self.assertIsInstance(POINTER(Sub), p_meta)
        self.assertTrue(issubclass(POINTER(Sub), Sub))

    def test_creating_pointer_in_dunder_init_1(self):
        class ct_meta(type):
            def __init__(self, name, bases, namespace):
                super().__init__(name, bases, namespace)

                # Avoid recursion.
                # (See test_creating_pointer_in_dunder_new_1)
                if bases == (c_void_p,):
                    return
                if issubclass(self, PtrBase):
                    return
                if bases == (object,):
                    ptr_bases = (self, PtrBase)
                else:
                    ptr_bases = (self, POINTER(bases[0]))
                p = p_meta(f"POINTER({self.__name__})", ptr_bases, {})
                ctypes._pointer_type_cache[self] = p

        class p_meta(PyCSimpleType, ct_meta):
            pass

        class PtrBase(c_void_p, metaclass=p_meta):
            pass

        class CtBase(object, metaclass=ct_meta):
            pass

        class Sub(CtBase):
            pass

        class Sub2(Sub):
            pass

        self.assertIsInstance(POINTER(Sub2), p_meta)
        self.assertTrue(issubclass(POINTER(Sub2), Sub2))
        self.assertTrue(issubclass(POINTER(Sub2), POINTER(Sub)))
        self.assertTrue(issubclass(POINTER(Sub), POINTER(CtBase)))

    def test_creating_pointer_in_dunder_init_2(self):
        class ct_meta(type):
            def __init__(self, name, bases, namespace):
                super().__init__(name, bases, namespace)

                # Avoid recursion.
                # (See test_creating_pointer_in_dunder_new_2)
                if isinstance(self, p_meta):
                    return
                p = p_meta(f"POINTER({self.__name__})", (self, c_void_p), {})
                ctypes._pointer_type_cache[self] = p

        class p_meta(PyCSimpleType, ct_meta):
            pass

        class Core(object):
            pass

        class CtBase(Core, metaclass=ct_meta):
            pass

        class Sub(CtBase):
            pass

        self.assertIsInstance(POINTER(Sub), p_meta)
        self.assertTrue(issubclass(POINTER(Sub), Sub))
