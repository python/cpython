# Tests some corner cases with isinstance() and issubclass().  While these
# tests use new style classes and properties, they actually do whitebox
# testing of error conditions uncovered when using extension types.

import unittest
import test_support



class TestIsInstanceWhitebox(unittest.TestCase):
    # Test to make sure that an AttributeError when accessing the instance's
    # class's bases is masked.  This was actually a bug in Python 2.2 and
    # 2.2.1 where the exception wasn't caught but it also wasn't being cleared
    # (leading to an "undetected error" in the debug build).  Set up is,
    # isinstance(inst, cls) where:
    #
    # - inst isn't an InstanceType
    # - cls isn't a ClassType, a TypeType, or a TupleType
    # - cls has a __bases__ attribute
    # - inst has a __class__ attribute
    # - inst.__class__ as no __bases__ attribute
    #
    # Sounds complicated, I know, but this mimics a situation where an
    # extension type raises an AttributeError when its __bases__ attribute is
    # gotten.  In that case, isinstance() should return False.
    def test_class_has_no_bases(self):
        class I(object):
            def getclass(self):
                # This must return an object that has no __bases__ attribute
                return None
            __class__ = property(getclass)

        class C(object):
            def getbases(self):
                return ()
            __bases__ = property(getbases)

        self.assertEqual(False, isinstance(I(), C()))

    # Like above except that inst.__class__.__bases__ raises an exception
    # other than AttributeError
    def test_bases_raises_other_than_attribute_error(self):
        class E(object):
            def getbases(self):
                raise RuntimeError
            __bases__ = property(getbases)

        class I(object):
            def getclass(self):
                return E()
            __class__ = property(getclass)

        class C(object):
            def getbases(self):
                return ()
            __bases__ = property(getbases)

        self.assertRaises(RuntimeError, isinstance, I(), C())

    # Here's a situation where getattr(cls, '__bases__') raises an exception.
    # If that exception is not AttributeError, it should not get masked
    def test_dont_mask_non_attribute_error(self):
        class I: pass

        class C(object):
            def getbases(self):
                raise RuntimeError
            __bases__ = property(getbases)

        self.assertRaises(RuntimeError, isinstance, I(), C())

    # Like above, except that getattr(cls, '__bases__') raises an
    # AttributeError, which /should/ get masked as a TypeError
    def test_mask_attribute_error(self):
        class I: pass

        class C(object):
            def getbases(self):
                raise AttributeError
            __bases__ = property(getbases)

        self.assertRaises(TypeError, isinstance, I(), C())



# These tests are similar to above, but tickle certain code paths in
# issubclass() instead of isinstance() -- really PyObject_IsSubclass()
# vs. PyObject_IsInstance().
class TestIsSubclassWhitebox(unittest.TestCase):
    def test_dont_mask_non_attribute_error(self):
        class C(object):
            def getbases(self):
                raise RuntimeError
            __bases__ = property(getbases)

        class S(C): pass

        self.assertRaises(RuntimeError, issubclass, C(), S())

    def test_mask_attribute_error(self):
        class C(object):
            def getbases(self):
                raise AttributeError
            __bases__ = property(getbases)

        class S(C): pass
        
        self.assertRaises(TypeError, issubclass, C(), S())

    # Like above, but test the second branch, where the __bases__ of the
    # second arg (the cls arg) is tested.  This means the first arg must
    # return a valid __bases__, and it's okay for it to be a normal --
    # unrelated by inheritance -- class.
    def test_dont_mask_non_attribute_error_in_cls_arg(self):
        class B: pass

        class C(object):
            def getbases(self):
                raise RuntimeError
            __bases__ = property(getbases)

        self.assertRaises(RuntimeError, issubclass, B, C())

    def test_mask_attribute_error_in_cls_arg(self):
        class B: pass

        class C(object):
            def getbases(self):
                raise AttributeError
            __bases__ = property(getbases)

        self.assertRaises(TypeError, issubclass, B, C())



def test_main():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(TestIsInstanceWhitebox))
    suite.addTest(unittest.makeSuite(TestIsSubclassWhitebox))
    test_support.run_suite(suite)


if __name__ == '__main__':
    test_main()
