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
           'total_ordering', 'cmp_to_key', 'lfu_cache', 'lru_cache']

from _functools import partial, reduce
from collections import OrderedDict, Counter
from heapq import nsmallest
from operator import itemgetter
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
    for attr in assigned:
        if hasattr(wrapped, attr):
            setattr(wrapper, attr, getattr(wrapped, attr))
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
    roots = set(dir(cls)) & set(convert)
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

def lfu_cache(maxsize=100):
    """Least-frequently-used cache decorator.

    Arguments to the cached function must be hashable.
    Cache performance statistics stored in f.hits and f.misses.
    Clear the cache using f.clear().
    http://en.wikipedia.org/wiki/Cache_algorithms#Least-Frequently_Used

    """
    def decorating_function(user_function, tuple=tuple, sorted=sorted, len=len):
        cache = {}                      # mapping of args to results
        use_count = Counter()           # times each key has been accessed
        kwd_mark = object()             # separate positional and keyword args
        lock = Lock()

        @wraps(user_function)
        def wrapper(*args, **kwds):
            key = args
            if kwds:
                key += (kwd_mark,) + tuple(sorted(kwds.items()))
            try:
                with lock:
                    use_count[key] += 1         # count a use of this key
                    result = cache[key]
                    wrapper.hits += 1
            except KeyError:
                result = user_function(*args, **kwds)
                with lock:
                    use_count[key] += 1         # count a use of this key
                    cache[key] = result
                    wrapper.misses += 1
                    if len(cache) > maxsize:
                        # purge the 10% least frequently used entries
                        for key, _ in nsmallest(maxsize // 10,
                                                use_count.items(),
                                                key=itemgetter(1)):
                            del cache[key], use_count[key]
            return result

        def clear():
            """Clear the cache and cache statistics"""
            with lock:
                cache.clear()
                use_count.clear()
                wrapper.hits = wrapper.misses = 0

        wrapper.hits = wrapper.misses = 0
        wrapper.clear = clear
        return wrapper
    return decorating_function

def lru_cache(maxsize=100):
    """Least-recently-used cache decorator.

    Arguments to the cached function must be hashable.
    Cache performance statistics stored in f.hits and f.misses.
    Clear the cache using f.clear().
    http://en.wikipedia.org/wiki/Cache_algorithms#Least_Recently_Used

    """
    def decorating_function(user_function, tuple=tuple, sorted=sorted, len=len):
        cache = OrderedDict()           # ordered least recent to most recent
        kwd_mark = object()             # separate positional and keyword args
        lock = Lock()

        @wraps(user_function)
        def wrapper(*args, **kwds):
            key = args
            if kwds:
                key += (kwd_mark,) + tuple(sorted(kwds.items()))
            try:
                with lock:
                    result = cache[key]
                    del cache[key]
                    cache[key] = result         # record recent use of this key
                    wrapper.hits += 1
            except KeyError:
                result = user_function(*args, **kwds)
                with lock:
                    cache[key] = result         # record recent use of this key
                    wrapper.misses += 1
                    if len(cache) > maxsize:
                        cache.popitem(0)        # purge least recently used cache entry
            return result

        def clear():
            """Clear the cache and cache statistics"""
            with lock:
                cache.clear()
                wrapper.hits = wrapper.misses = 0

        wrapper.hits = wrapper.misses = 0
        wrapper.clear = clear
        return wrapper
    return decorating_function
