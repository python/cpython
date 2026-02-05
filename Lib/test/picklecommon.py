# Classes used for pickle testing.
# They are moved to separate file, so they can be loaded
# in other Python version for test_xpickle.

import sys

class C:
    def __eq__(self, other):
        return self.__dict__ == other.__dict__

# For test_load_classic_instance
class D(C):
    def __init__(self, arg):
        pass

class E(C):
    def __getinitargs__(self):
        return ()

import __main__
__main__.C = C
C.__module__ = "__main__"
__main__.D = D
D.__module__ = "__main__"
__main__.E = E
E.__module__ = "__main__"

# Simple mutable object.
class Object(object):
    pass

# Hashable immutable key object containing unheshable mutable data.
class K:
    def __init__(self, value):
        self.value = value

    def __reduce__(self):
        # Shouldn't support the recursion itself
        return K, (self.value,)

class WithSlots(object):
    __slots__ = ('a', 'b')

class WithSlotsSubclass(WithSlots):
    __slots__ = ('c',)

class WithSlotsAndDict(object):
    __slots__ = ('a', '__dict__')

class WithPrivateAttrs(object):
    def __init__(self, a):
        self.__private = a
    def get(self):
        return self.__private

class WithPrivateAttrsSubclass(WithPrivateAttrs):
    def __init__(self, a, b):
        super().__init__(a)
        self.__private = b
    def get2(self):
        return self.__private

class WithPrivateSlots(object):
    __slots__ = ('__private',)
    def __init__(self, a):
        self.__private = a
    def get(self):
        return self.__private

class WithPrivateSlotsSubclass(WithPrivateSlots):
    __slots__ = ('__private',)
    def __init__(self, a, b):
        super().__init__(a)
        self.__private = b
    def get2(self):
        return self.__private

# For test_misc
class myint(int):
    def __init__(self, x):
        self.str = str(x)

# For test_misc and test_getinitargs
class initarg(C):

    def __init__(self, a, b):
        self.a = a
        self.b = b

    def __getinitargs__(self):
        return self.a, self.b

# For test_metaclass
class metaclass(type):
    pass

if sys.version_info >= (3,):
    # Syntax not compatible with Python 2
    exec('''
class use_metaclass(object, metaclass=metaclass):
    pass
''')
else:
    class use_metaclass(object):
        __metaclass__ = metaclass


# Test classes for reduce_ex

class R:
    def __init__(self, reduce=None):
        self.reduce = reduce
    def __reduce__(self, proto):
        return self.reduce

class REX:
    def __init__(self, reduce_ex=None):
        self.reduce_ex = reduce_ex
    def __reduce_ex__(self, proto):
        return self.reduce_ex

class REX_one(object):
    """No __reduce_ex__ here, but inheriting it from object"""
    _reduce_called = 0
    def __reduce__(self):
        self._reduce_called = 1
        return REX_one, ()

class REX_two(object):
    """No __reduce__ here, but inheriting it from object"""
    _proto = None
    def __reduce_ex__(self, proto):
        self._proto = proto
        return REX_two, ()

class REX_three(object):
    _proto = None
    def __reduce_ex__(self, proto):
        self._proto = proto
        return REX_two, ()
    def __reduce__(self):
        raise AssertionError("This __reduce__ shouldn't be called")

class REX_four(object):
    """Calling base class method should succeed"""
    _proto = None
    def __reduce_ex__(self, proto):
        self._proto = proto
        return object.__reduce_ex__(self, proto)

class REX_five(object):
    """This one used to fail with infinite recursion"""
    _reduce_called = 0
    def __reduce__(self):
        self._reduce_called = 1
        return object.__reduce__(self)

class REX_six(object):
    """This class is used to check the 4th argument (list iterator) of
    the reduce protocol.
    """
    def __init__(self, items=None):
        self.items = items if items is not None else []
    def __eq__(self, other):
        return type(self) is type(other) and self.items == other.items
    def append(self, item):
        self.items.append(item)
    def __reduce__(self):
        return type(self), (), None, iter(self.items), None

class REX_seven(object):
    """This class is used to check the 5th argument (dict iterator) of
    the reduce protocol.
    """
    def __init__(self, table=None):
        self.table = table if table is not None else {}
    def __eq__(self, other):
        return type(self) is type(other) and self.table == other.table
    def __setitem__(self, key, value):
        self.table[key] = value
    def __reduce__(self):
        return type(self), (), None, None, iter(self.table.items())

class REX_state(object):
    """This class is used to check the 3th argument (state) of
    the reduce protocol.
    """
    def __init__(self, state=None):
        self.state = state
    def __eq__(self, other):
        return type(self) is type(other) and self.state == other.state
    def __setstate__(self, state):
        self.state = state
    def __reduce__(self):
        return type(self), (), self.state

# For test_reduce_ex_None
class REX_None:
    """ Setting __reduce_ex__ to None should fail """
    __reduce_ex__ = None

# For test_reduce_None
class R_None:
    """ Setting __reduce__ to None should fail """
    __reduce__ = None

# For test_pickle_setstate_None
class C_None_setstate:
    """  Setting __setstate__ to None should fail """
    def __getstate__(self):
        return 1

    __setstate__ = None


# Test classes for newobj

# For test_newobj_generic and test_newobj_proxies

class MyInt(int):
    sample = 1

if sys.version_info >= (3,):
    class MyLong(int):
        sample = 1
else:
    class MyLong(long):
        sample = long(1)

class MyFloat(float):
    sample = 1.0

class MyComplex(complex):
    sample = 1.0 + 0.0j

class MyStr(str):
    sample = "hello"

if sys.version_info >= (3,):
    class MyUnicode(str):
        sample = "hello \u1234"
else:
    class MyUnicode(unicode):
        sample = unicode(r"hello \u1234", "raw-unicode-escape")

class MyTuple(tuple):
    sample = (1, 2, 3)

class MyList(list):
    sample = [1, 2, 3]

class MyDict(dict):
    sample = {"a": 1, "b": 2}

class MySet(set):
    sample = {"a", "b"}

class MyFrozenSet(frozenset):
    sample = frozenset({"a", "b"})

myclasses = [MyInt, MyLong, MyFloat,
             MyComplex,
             MyStr, MyUnicode,
             MyTuple, MyList, MyDict, MySet, MyFrozenSet]

# For test_newobj_overridden_new
class MyIntWithNew(int):
    def __new__(cls, value):
        raise AssertionError

class MyIntWithNew2(MyIntWithNew):
    __new__ = int.__new__


# For test_newobj_list_slots
class SlotList(MyList):
    __slots__ = ["foo"]

# Ruff "redefined while unused" false positive here due to `global` variables
# being assigned (and then restored) from within test methods earlier in the file
class SimpleNewObj(int):  # noqa: F811
    def __init__(self, *args, **kwargs):
        # raise an error, to make sure this isn't called
        raise TypeError("SimpleNewObj.__init__() didn't expect to get called")
    def __eq__(self, other):
        return int(self) == int(other) and self.__dict__ == other.__dict__

class ComplexNewObj(SimpleNewObj):
    def __getnewargs__(self):
        return ('%X' % self, 16)

class ComplexNewObjEx(SimpleNewObj):
    def __getnewargs_ex__(self):
        return ('%X' % self,), {'base': 16}


class ZeroCopyBytes(bytes):
    readonly = True
    c_contiguous = True
    f_contiguous = True
    zero_copy_reconstruct = True

    def __reduce_ex__(self, protocol):
        if protocol >= 5:
            import pickle
            return type(self)._reconstruct, (pickle.PickleBuffer(self),), None
        else:
            return type(self)._reconstruct, (bytes(self),)

    def __repr__(self):
        return "{}({!r})".format(self.__class__.__name__, bytes(self))

    __str__ = __repr__

    @classmethod
    def _reconstruct(cls, obj):
        with memoryview(obj) as m:
            obj = m.obj
            if type(obj) is cls:
                # Zero-copy
                return obj
            else:
                return cls(obj)


class ZeroCopyBytearray(bytearray):
    readonly = False
    c_contiguous = True
    f_contiguous = True
    zero_copy_reconstruct = True

    def __reduce_ex__(self, protocol):
        if protocol >= 5:
            import pickle
            return type(self)._reconstruct, (pickle.PickleBuffer(self),), None
        else:
            return type(self)._reconstruct, (bytes(self),)

    def __repr__(self):
        return "{}({!r})".format(self.__class__.__name__, bytes(self))

    __str__ = __repr__

    @classmethod
    def _reconstruct(cls, obj):
        with memoryview(obj) as m:
            obj = m.obj
            if type(obj) is cls:
                # Zero-copy
                return obj
            else:
                return cls(obj)


# For test_nested_names
class Nested:
    class A:
        class B:
            class C:
                pass

# For test_py_methods
class PyMethodsTest:
    @staticmethod
    def cheese():
        return "cheese"
    @classmethod
    def wine(cls):
        assert cls is PyMethodsTest
        return "wine"
    def biscuits(self):
        assert isinstance(self, PyMethodsTest)
        return "biscuits"
    class Nested:
        "Nested class"
        @staticmethod
        def ketchup():
            return "ketchup"
        @classmethod
        def maple(cls):
            assert cls is PyMethodsTest.Nested
            return "maple"
        def pie(self):
            assert isinstance(self, PyMethodsTest.Nested)
            return "pie"

# For test_c_methods
class Subclass(tuple):
    class Nested(str):
        pass

# For test_private_methods
class PrivateMethods:
    def __init__(self, value):
        self.value = value

    def __private_method(self):
        return self.value

    def get_method(self):
        return self.__private_method

    @classmethod
    def get_unbound_method(cls):
        return cls.__private_method

    @classmethod
    def __private_classmethod(cls):
        return 43

    @classmethod
    def get_classmethod(cls):
        return cls.__private_classmethod

    @staticmethod
    def __private_staticmethod():
        return 44

    @classmethod
    def get_staticmethod(cls):
        return cls.__private_staticmethod

# For test_private_nested_classes
class PrivateNestedClasses:
    @classmethod
    def get_nested(cls):
        return cls.__Nested

    class __Nested:
        @classmethod
        def get_nested2(cls):
            return cls.__Nested2

        class __Nested2:
            pass
