"""Generic (shallow and deep) copying operations.

Interface summary:

        import copy

        x = copy.copy(y)        # make a shallow copy of y
        x = copy.deepcopy(y)    # make a deep copy of y

For module specific errors, copy.error is raised.

The difference between shallow and deep copying is only relevant for
compound objects (objects that contain other objects, like lists or
class instances).

- A shallow copy constructs a new compound object and then (to the
  extent possible) inserts *the same objects* into in that the
  original contains.

- A deep copy constructs a new compound object and then, recursively,
  inserts *copies* into it of the objects found in the original.

Two problems often exist with deep copy operations that don't exist
with shallow copy operations:

 a) recursive objects (compound objects that, directly or indirectly,
    contain a reference to themselves) may cause a recursive loop

 b) because deep copy copies *everything* it may copy too much, e.g.
    administrative data structures that should be shared even between
    copies

Python's deep copy operation avoids these problems by:

 a) keeping a table of objects already copied during the current
    copying pass

 b) letting user-defined classes override the copying operation or the
    set of components copied

This version does not copy types like module, class, function, method,
nor stack trace, stack frame, nor file, socket, window, nor array, nor
any similar types.

Classes can use the same interfaces to control copying that they use
to control pickling: they can define methods called __getinitargs__(),
__getstate__() and __setstate__().  See the documentation for module
"pickle" for information on these methods.
"""

# XXX need to support copy_reg here too...

import types

class Error(Exception):
    pass
error = Error   # backward compatibility

try:
    from org.python.core import PyStringMap
except ImportError:
    PyStringMap = None

__all__ = ["Error", "error", "copy", "deepcopy"]

def copy(x):
    """Shallow copy operation on arbitrary Python objects.

    See the module's __doc__ string for more info.
    """

    try:
        copierfunction = _copy_dispatch[type(x)]
    except KeyError:
        try:
            copier = x.__copy__
        except AttributeError:
            try:
                reductor = x.__reduce__
            except AttributeError:
                raise error, \
                      "un(shallow)copyable object of type %s" % type(x)
            else:
                y = _reconstruct(x, reductor(), 0)
        else:
            y = copier()
    else:
        y = copierfunction(x)
    return y

_copy_dispatch = d = {}

def _copy_atomic(x):
    return x
d[types.NoneType] = _copy_atomic
d[types.IntType] = _copy_atomic
d[types.LongType] = _copy_atomic
d[types.FloatType] = _copy_atomic
try:
    d[types.ComplexType] = _copy_atomic
except AttributeError:
    pass
d[types.StringType] = _copy_atomic
try:
    d[types.UnicodeType] = _copy_atomic
except AttributeError:
    pass
try:
    d[types.CodeType] = _copy_atomic
except AttributeError:
    pass
d[types.TypeType] = _copy_atomic
d[types.XRangeType] = _copy_atomic
d[types.ClassType] = _copy_atomic

def _copy_list(x):
    return x[:]
d[types.ListType] = _copy_list

def _copy_tuple(x):
    return x[:]
d[types.TupleType] = _copy_tuple

def _copy_dict(x):
    return x.copy()
d[types.DictionaryType] = _copy_dict
if PyStringMap is not None:
    d[PyStringMap] = _copy_dict

def _copy_inst(x):
    if hasattr(x, '__copy__'):
        return x.__copy__()
    if hasattr(x, '__getinitargs__'):
        args = x.__getinitargs__()
        y = apply(x.__class__, args)
    else:
        y = _EmptyClass()
        y.__class__ = x.__class__
    if hasattr(x, '__getstate__'):
        state = x.__getstate__()
    else:
        state = x.__dict__
    if hasattr(y, '__setstate__'):
        y.__setstate__(state)
    else:
        y.__dict__.update(state)
    return y
d[types.InstanceType] = _copy_inst

del d

def deepcopy(x, memo = None):
    """Deep copy operation on arbitrary Python objects.

    See the module's __doc__ string for more info.
    """

    if memo is None:
        memo = {}
    d = id(x)
    if memo.has_key(d):
        return memo[d]
    try:
        copierfunction = _deepcopy_dispatch[type(x)]
    except KeyError:
        try:
            issc = issubclass(type(x), type)
        except TypeError:
            issc = 0
        if issc:
            y = _deepcopy_dispatch[type](x, memo)
        else:
            try:
                copier = x.__deepcopy__
            except AttributeError:
                try:
                    reductor = x.__reduce__
                except AttributeError:
                    raise error, \
                       "un-deep-copyable object of type %s" % type(x)
                else:
                    y = _reconstruct(x, reductor(), 1, memo)
            else:
                y = copier(memo)
    else:
        y = copierfunction(x, memo)
    memo[d] = y
    _keep_alive(x, memo) # Make sure x lives at least as long as d
    return y

_deepcopy_dispatch = d = {}

def _deepcopy_atomic(x, memo):
    return x
d[types.NoneType] = _deepcopy_atomic
d[types.IntType] = _deepcopy_atomic
d[types.LongType] = _deepcopy_atomic
d[types.FloatType] = _deepcopy_atomic
try:
    d[types.ComplexType] = _deepcopy_atomic
except AttributeError:
    pass
d[types.StringType] = _deepcopy_atomic
try:
    d[types.UnicodeType] = _deepcopy_atomic
except AttributeError:
    pass
try:
    d[types.CodeType] = _deepcopy_atomic
except AttributeError:
    pass
d[types.TypeType] = _deepcopy_atomic
d[types.XRangeType] = _deepcopy_atomic

def _deepcopy_list(x, memo):
    y = []
    memo[id(x)] = y
    for a in x:
        y.append(deepcopy(a, memo))
    return y
d[types.ListType] = _deepcopy_list

def _deepcopy_tuple(x, memo):
    y = []
    for a in x:
        y.append(deepcopy(a, memo))
    d = id(x)
    try:
        return memo[d]
    except KeyError:
        pass
    for i in range(len(x)):
        if x[i] is not y[i]:
            y = tuple(y)
            break
    else:
        y = x
    memo[d] = y
    return y
d[types.TupleType] = _deepcopy_tuple

def _deepcopy_dict(x, memo):
    y = {}
    memo[id(x)] = y
    for key in x.keys():
        y[deepcopy(key, memo)] = deepcopy(x[key], memo)
    return y
d[types.DictionaryType] = _deepcopy_dict
if PyStringMap is not None:
    d[PyStringMap] = _deepcopy_dict

def _keep_alive(x, memo):
    """Keeps a reference to the object x in the memo.

    Because we remember objects by their id, we have
    to assure that possibly temporary objects are kept
    alive by referencing them.
    We store a reference at the id of the memo, which should
    normally not be used unless someone tries to deepcopy
    the memo itself...
    """
    try:
        memo[id(memo)].append(x)
    except KeyError:
        # aha, this is the first one :-)
        memo[id(memo)]=[x]

def _deepcopy_inst(x, memo):
    if hasattr(x, '__deepcopy__'):
        return x.__deepcopy__(memo)
    if hasattr(x, '__getinitargs__'):
        args = x.__getinitargs__()
        args = deepcopy(args, memo)
        y = apply(x.__class__, args)
    else:
        y = _EmptyClass()
        y.__class__ = x.__class__
    memo[id(x)] = y
    if hasattr(x, '__getstate__'):
        state = x.__getstate__()
    else:
        state = x.__dict__
    state = deepcopy(state, memo)
    if hasattr(y, '__setstate__'):
        y.__setstate__(state)
    else:
        y.__dict__.update(state)
    return y
d[types.InstanceType] = _deepcopy_inst

def _reconstruct(x, info, deep, memo=None):
    if isinstance(info, str):
        return x
    assert isinstance(info, tuple)
    if memo is None:
        memo = {}
    n = len(info)
    assert n in (2, 3)
    callable, args = info[:2]
    if n > 2:
        state = info[2]
    else:
        state = {}
    if deep:
        args = deepcopy(args, memo)
    y = callable(*args)
    if state:
        if deep:
            state = deepcopy(state, memo)
        if hasattr(y, '__setstate__'):
            y.__setstate__(state)
        else:
            y.__dict__.update(state)
    return y

del d

del types

# Helper for instance creation without calling __init__
class _EmptyClass:
    pass

def _test():
    l = [None, 1, 2L, 3.14, 'xyzzy', (1, 2L), [3.14, 'abc'],
         {'abc': 'ABC'}, (), [], {}]
    l1 = copy(l)
    print l1==l
    l1 = map(copy, l)
    print l1==l
    l1 = deepcopy(l)
    print l1==l
    class C:
        def __init__(self, arg=None):
            self.a = 1
            self.arg = arg
            if __name__ == '__main__':
                import sys
                file = sys.argv[0]
            else:
                file = __file__
            self.fp = open(file)
            self.fp.close()
        def __getstate__(self):
            return {'a': self.a, 'arg': self.arg}
        def __setstate__(self, state):
            for key in state.keys():
                setattr(self, key, state[key])
        def __deepcopy__(self, memo = None):
            new = self.__class__(deepcopy(self.arg, memo))
            new.a = self.a
            return new
    c = C('argument sketch')
    l.append(c)
    l2 = copy(l)
    print l == l2
    print l
    print l2
    l2 = deepcopy(l)
    print l == l2
    print l
    print l2
    l.append({l[1]: l, 'xyz': l[2]})
    l3 = copy(l)
    import repr
    print map(repr.repr, l)
    print map(repr.repr, l1)
    print map(repr.repr, l2)
    print map(repr.repr, l3)
    l3 = deepcopy(l)
    import repr
    print map(repr.repr, l)
    print map(repr.repr, l1)
    print map(repr.repr, l2)
    print map(repr.repr, l3)

if __name__ == '__main__':
    _test()
