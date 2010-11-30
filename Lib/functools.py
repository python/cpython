"""functools.py - Tools for working with functions and callable objects
"""
# Python module wrapper for _functools C module
# to allow utilities written in Python to be added
# to the functools module.
# Written by Nick Coghlan <ncoghlan at gmail.com>
# and Raymond Hettinger <python at rcn.com>
#   Copyright (C) 2006-2010 Python Software Foundation.
# See C source code for _functools credits/copyright

__all__ = ['update_wrapper', 'wraps', 'WRAPPER_ASSIGNMENTS', 'WRAPPER_UPDATES',
           'total_ordering', 'cmp_to_key', 'lru_cache', 'reduce', 'partial']

from _functools import partial, reduce
from collections import OrderedDict, namedtuple
try:
    from _thread import allocate_lock as Lock
except:
    from _dummy_thread import allocate_lock as Lock

# update_wrapper() and wraps() are tools to help write
# wrapper functions that can handle naive introspection

WRAPPER_ASSIGNMENTS = ('__module__', '__name__', '__doc__', '__annotations__')
WRAPPER_UPDATES = ('__dict__',)
def update_wrapper(wrapper,
                   wrapped,
                   assigned = WRAPPER_ASSIGNMENTS,
                   updated = WRAPPER_UPDATES):
    """Update a wrapper function to look like the wrapped function

       wrapper is the function to be updated
       wrapped is the original function
       assigned is a tuple naming the attributes assigned directly
       from the wrapped function to the wrapper function (defaults to
       functools.WRAPPER_ASSIGNMENTS)
       updated is a tuple naming the attributes of the wrapper that
       are updated with the corresponding attribute from the wrapped
       function (defaults to functools.WRAPPER_UPDATES)
    """
    wrapper.__wrapped__ = wrapped
    for attr in assigned:
        try:
            value = getattr(wrapped, attr)
        except AttributeError:
            pass
        else:
            setattr(wrapper, attr, value)
    for attr in updated:
        getattr(wrapper, attr).update(getattr(wrapped, attr, {}))
    # Return the wrapper so this can be used as a decorator via partial()
    return wrapper

def wraps(wrapped,
          assigned = WRAPPER_ASSIGNMENTS,
          updated = WRAPPER_UPDATES):
    """Decorator factory to apply update_wrapper() to a wrapper function

       Returns a decorator that invokes update_wrapper() with the decorated
       function as the wrapper argument and the arguments to wraps() as the
       remaining arguments. Default arguments are as for update_wrapper().
       This is a convenience function to simplify applying partial() to
       update_wrapper().
    """
    return partial(update_wrapper, wrapped=wrapped,
                   assigned=assigned, updated=updated)

def total_ordering(cls):
    """Class decorator that fills in missing ordering methods"""
    convert = {
        '__lt__': [('__gt__', lambda self, other: other < self),
                   ('__le__', lambda self, other: not other < self),
                   ('__ge__', lambda self, other: not self < other)],
        '__le__': [('__ge__', lambda self, other: other <= self),
                   ('__lt__', lambda self, other: not other <= self),
                   ('__gt__', lambda self, other: not self <= other)],
        '__gt__': [('__lt__', lambda self, other: other > self),
                   ('__ge__', lambda self, other: not other > self),
                   ('__le__', lambda self, other: not self > other)],
        '__ge__': [('__le__', lambda self, other: other >= self),
                   ('__gt__', lambda self, other: not other >= self),
                   ('__lt__', lambda self, other: not self >= other)]
    }
    # Find user-defined comparisons (not those inherited from object).
    roots = [op for op in convert if getattr(cls, op, None) is not getattr(object, op, None)]
    if not roots:
        raise ValueError('must define at least one ordering operation: < > <= >=')
    root = max(roots)       # prefer __lt__ to __le__ to __gt__ to __ge__
    for opname, opfunc in convert[root]:
        if opname not in roots:
            opfunc.__name__ = opname
            opfunc.__doc__ = getattr(int, opname).__doc__
            setattr(cls, opname, opfunc)
    return cls

def cmp_to_key(mycmp):
    """Convert a cmp= function into a key= function"""
    class K(object):
        def __init__(self, obj, *args):
            self.obj = obj
        def __lt__(self, other):
            return mycmp(self.obj, other.obj) < 0
        def __gt__(self, other):
            return mycmp(self.obj, other.obj) > 0
        def __eq__(self, other):
            return mycmp(self.obj, other.obj) == 0
        def __le__(self, other):
            return mycmp(self.obj, other.obj) <= 0
        def __ge__(self, other):
            return mycmp(self.obj, other.obj) >= 0
        def __ne__(self, other):
            return mycmp(self.obj, other.obj) != 0
        def __hash__(self):
            raise TypeError('hash not implemented')
    return K

_CacheInfo = namedtuple("CacheInfo", "maxsize, size, hits, misses")

def lru_cache(maxsize=100):
    """Least-recently-used cache decorator.

    Arguments to the cached function must be hashable.

    View the cache statistics named tuple (maxsize, size, hits, misses) with
    f.cache_info().  Clear the cache and statistics with f.cache_clear().
    And access the underlying function with f.__wrapped__.

    See:  http://en.wikipedia.org/wiki/Cache_algorithms#Least_Recently_Used

    """
    # Users should only access the lru_cache through its public API:
    #       cache_info, cache_clear, and f.__wrapped__
    # The internals of the lru_cache are encapsulated for thread safety and
    # to allow the implementation to change (including a possible C version).

    def decorating_function(user_function,
                tuple=tuple, sorted=sorted, len=len, KeyError=KeyError):

        cache = OrderedDict()           # ordered least recent to most recent
        cache_popitem = cache.popitem
        cache_renew = cache.move_to_end
        hits = misses = 0
        kwd_mark = object()             # separates positional and keyword args
        lock = Lock()

        @wraps(user_function)
        def wrapper(*args, **kwds):
            nonlocal hits, misses
            key = args
            if kwds:
                key += (kwd_mark,) + tuple(sorted(kwds.items()))
            try:
                with lock:
                    result = cache[key]
                    cache_renew(key)            # record recent use of this key
                    hits += 1
            except KeyError:
                result = user_function(*args, **kwds)
                with lock:
                    cache[key] = result         # record recent use of this key
                    misses += 1
                    if len(cache) > maxsize:
                        cache_popitem(0)        # purge least recently used cache entry
            return result

        def cache_info():
            """Report cache statistics"""
            with lock:
                return _CacheInfo(maxsize, len(cache), hits, misses)

        def cache_clear():
            """Clear the cache and cache statistics"""
            nonlocal hits, misses
            with lock:
                cache.clear()
                hits = misses = 0

        wrapper.cache_info = cache_info
        wrapper.cache_clear = cache_clear
        return wrapper

    return decorating_function
