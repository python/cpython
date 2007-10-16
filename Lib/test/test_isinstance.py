# Tests some corner cases with isinstance() and issubclass().  While these
# tests use new style classes and properties, they actually do whitebox
# testing of error conditions uncovered when using extension types.

import unittest
from test import test_support
import sys



class TestIsInstanceExceptions(unittest.TestCase):
    # Test to make sure that an AttributeError when accessing the instance's
    # class's bases is masked.  This was actually a bug in Python 2.2 and
    # 2.2.1 where the exception wasn't caught but it also wasn't being cleared
    # (leading to an "undetected error" in the debug build).  Set up is,
    # isinstance(inst, cls) where:
    #
    # - cls isn't a a type, or a tuple
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
class TestIsSubclassExceptions(unittest.TestCase):
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



# meta classes for creating abstract classes and instances
class AbstractClass(object):
    def __init__(self, bases):
        self.bases = bases

    def getbases(self):
        return self.bases
    __bases__ = property(getbases)

    def __call__(self):
        return AbstractInstance(self)

class AbstractInstance(object):
    def __init__(self, klass):
        self.klass = klass

    def getclass(self):
        return self.klass
    __class__ = property(getclass)

# abstract classes
AbstractSuper = AbstractClass(bases=())

AbstractChild = AbstractClass(bases=(AbstractSuper,))

# normal classes
class Super:
    pass

class Child(Super):
    pass

# new-style classes
class NewSuper(object):
    pass

class NewChild(NewSuper):
    pass



class TestIsInstanceIsSubclass(unittest.TestCase):
    # Tests to ensure that isinstance and issubclass work on abstract
    # classes and instances.  Before the 2.2 release, TypeErrors were
    # raised when boolean values should have been returned.  The bug was
    # triggered by mixing 'normal' classes and instances were with
    # 'abstract' classes and instances.  This case tries to test all
    # combinations.

    def test_isinstance_normal(self):
        # normal instances
        self.assertEqual(True, isinstance(Super(), Super))
        self.assertEqual(False, isinstance(Super(), Child))
        self.assertEqual(False, isinstance(Super(), AbstractSuper))
        self.assertEqual(False, isinstance(Super(), AbstractChild))

        self.assertEqual(True, isinstance(Child(), Super))
        self.assertEqual(False, isinstance(Child(), AbstractSuper))

    def test_isinstance_abstract(self):
        # abstract instances
        self.assertEqual(True, isinstance(AbstractSuper(), AbstractSuper))
        self.assertEqual(False, isinstance(AbstractSuper(), AbstractChild))
        self.assertEqual(False, isinstance(AbstractSuper(), Super))
        self.assertEqual(False, isinstance(AbstractSuper(), Child))

        self.assertEqual(True, isinstance(AbstractChild(), AbstractChild))
        self.assertEqual(True, isinstance(AbstractChild(), AbstractSuper))
        self.assertEqual(False, isinstance(AbstractChild(), Super))
        self.assertEqual(False, isinstance(AbstractChild(), Child))

    def test_subclass_normal(self):
        # normal classes
        self.assertEqual(True, issubclass(Super, Super))
        self.assertEqual(False, issubclass(Super, AbstractSuper))
        self.assertEqual(False, issubclass(Super, Child))

        self.assertEqual(True, issubclass(Child, Child))
        self.assertEqual(True, issubclass(Child, Super))
        self.assertEqual(False, issubclass(Child, AbstractSuper))

    def test_subclass_abstract(self):
        # abstract classes
        self.assertEqual(True, issubclass(AbstractSuper, AbstractSuper))
        self.assertEqual(False, issubclass(AbstractSuper, AbstractChild))
        self.assertEqual(False, issubclass(AbstractSuper, Child))

        self.assertEqual(True, issubclass(AbstractChild, AbstractChild))
        self.assertEqual(True, issubclass(AbstractChild, AbstractSuper))
        self.assertEqual(False, issubclass(AbstractChild, Super))
        self.assertEqual(False, issubclass(AbstractChild, Child))

    def test_subclass_tuple(self):
        # test with a tuple as the second argument classes
        self.assertEqual(True, issubclass(Child, (Child,)))
        self.assertEqual(True, issubclass(Child, (Super,)))
        self.assertEqual(False, issubclass(Super, (Child,)))
        self.assertEqual(True, issubclass(Super, (Child, Super)))
        self.assertEqual(False, issubclass(Child, ()))
        self.assertEqual(True, issubclass(Super, (Child, (Super,))))

        self.assertEqual(True, issubclass(NewChild, (NewChild,)))
        self.assertEqual(True, issubclass(NewChild, (NewSuper,)))
        self.assertEqual(False, issubclass(NewSuper, (NewChild,)))
        self.assertEqual(True, issubclass(NewSuper, (NewChild, NewSuper)))
        self.assertEqual(False, issubclass(NewChild, ()))
        self.assertEqual(True, issubclass(NewSuper, (NewChild, (NewSuper,))))

        self.assertEqual(True, issubclass(int, (int, (float, int))))
        self.assertEqual(True, issubclass(str, (str, (Child, NewChild, str))))

    def test_subclass_recursion_limit(self):
        # make sure that issubclass raises RuntimeError before the C stack is
        # blown
        self.assertRaises(RuntimeError, blowstack, issubclass, str, str)

    def test_isinstance_recursion_limit(self):
        # make sure that issubclass raises RuntimeError before the C stack is
        # blown
        self.assertRaises(RuntimeError, blowstack, isinstance, '', str)

def blowstack(fxn, arg, compare_to):
    # Make sure that calling isinstance with a deeply nested tuple for its
    # argument will raise RuntimeError eventually.
    tuple_arg = (compare_to,)
    for cnt in range(sys.getrecursionlimit()+5):
        tuple_arg = (tuple_arg,)
        fxn(arg, tuple_arg)


def test_main():
    test_support.run_unittest(
        TestIsInstanceExceptions,
        TestIsSubclassExceptions,
        TestIsInstanceIsSubclass
    )


if __name__ == '__main__':
    test_main()
