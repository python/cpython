class C:
    def __eq__(self, other):
        return self.__dict__ == other.__dict__

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
class Object:
    pass

# Hashable immutable key object containing unheshable mutable data.
class K:
    def __init__(self, value):
        self.value = value

    def __reduce__(self):
        # Shouldn't support the recursion itself
        return K, (self.value,)

class myint(int):
    def __init__(self, x):
        self.str = str(x)

class initarg(C):

    def __init__(self, a, b):
        self.a = a
        self.b = b

    def __getinitargs__(self):
        return self.a, self.b

class metaclass(type):
    pass

class use_metaclass(object, metaclass=metaclass):
    pass


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
        raise TestFailed("This __reduce__ shouldn't be called")

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

class REX_None:
    """ Setting __reduce_ex__ to None should fail """
    __reduce_ex__ = None

class R_None:
    """ Setting __reduce__ to None should fail """
    __reduce__ = None

class C_None_setstate:
    """  Setting __setstate__ to None should fail """
    def __getstate__(self):
        return 1

    __setstate__ = None


# Test classes for newobj

class MyInt(int):
    sample = 1

class MyFloat(float):
    sample = 1.0

class MyComplex(complex):
    sample = 1.0 + 0.0j

class MyStr(str):
    sample = "hello"

class MyUnicode(str):
    sample = "hello \u1234"

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

myclasses = [MyInt, MyFloat,
             MyComplex,
             MyStr, MyUnicode,
             MyTuple, MyList, MyDict, MySet, MyFrozenSet]

class MyIntWithNew(int):
    def __new__(cls, value):
        raise AssertionError

class MyIntWithNew2(MyIntWithNew):
    __new__ = int.__new__


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

class BadGetattr:
    def __getattr__(self, key):
        self.foo

class NoNew:
    def __getattribute__(self, name):
        if name == '__new__':
            raise AttributeError
        return super().__getattribute__(name)


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


try:
    import _testbuffer
except ImportError:
    _testbuffer = None

if _testbuffer is not None:

    class PicklableNDArray:
        # A not-really-zero-copy picklable ndarray, as the ndarray()
        # constructor doesn't allow for it

        zero_copy_reconstruct = False

        def __init__(self, *args, **kwargs):
            self.array = _testbuffer.ndarray(*args, **kwargs)

        def __getitem__(self, idx):
            cls = type(self)
            new = cls.__new__(cls)
            new.array = self.array[idx]
            return new

        @property
        def readonly(self):
            return self.array.readonly

        @property
        def c_contiguous(self):
            return self.array.c_contiguous

        @property
        def f_contiguous(self):
            return self.array.f_contiguous

        def __eq__(self, other):
            if not isinstance(other, PicklableNDArray):
                return NotImplemented
            return (other.array.format == self.array.format and
                    other.array.shape == self.array.shape and
                    other.array.strides == self.array.strides and
                    other.array.readonly == self.array.readonly and
                    other.array.tobytes() == self.array.tobytes())

        def __ne__(self, other):
            if not isinstance(other, PicklableNDArray):
                return NotImplemented
            return not (self == other)

        def __repr__(self):
            return ("{name}(shape={array.shape},"
                    "strides={array.strides}, "
                    "bytes={array.tobytes()})").format(
                    name=type(self).__name__, array=self.array.shape)

        def __reduce_ex__(self, protocol):
            if not self.array.contiguous:
                raise NotImplementedError("Reconstructing a non-contiguous "
                                          "ndarray does not seem possible")
            ndarray_kwargs = {"shape": self.array.shape,
                              "strides": self.array.strides,
                              "format": self.array.format,
                              "flags": (0 if self.readonly
                                        else _testbuffer.ND_WRITABLE)}
            import pickle
            pb = pickle.PickleBuffer(self.array)
            if protocol >= 5:
                return (type(self)._reconstruct,
                        (pb, ndarray_kwargs))
            else:
                # Need to serialize the bytes in physical order
                with pb.raw() as m:
                    return (type(self)._reconstruct,
                            (m.tobytes(), ndarray_kwargs))

        @classmethod
        def _reconstruct(cls, obj, kwargs):
            with memoryview(obj) as m:
                # For some reason, ndarray() wants a list of integers...
                # XXX This only works if format == 'B'
                items = list(m.tobytes())
            return cls(items, **kwargs)
