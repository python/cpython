"""unittest.mock matchers helpers."""

import collections
import re
from unittest import mock


class _NOTNONE(object):
    """NOTNONE matches anything that is not None."""

    def __eq__(self, o):
        return o is not None

    def __ne__(self, o):
        return o is None

    def __repr__(self):
        return "<NOTNONE>"


NOTNONE = _NOTNONE()


class REGEXP(object):
    """REGEXP matches pattern (str or compiled)."""

    def __init__(self, pattern, compile_flags=0):
        self._f = getattr(pattern, "search", None)
        if not callable(self._f):
            self._f = None
        self._p = getattr(pattern, "pattern", None)
        if (not self._f) ^ (not self._p):
            raise TypeError("%r is not a RE_Pattern" % pattern)
        if not self._f and not self._p:
            if not isinstance(pattern, str):
                raise TypeError("%r is not a str" % pattern)
            try:
                c = re.compile(pattern, compile_flags)
            except re.error as e:
                e = str(e)
                raise ValueError(e)
            self._f, self._p = c.search, c.pattern

    def __eq__(self, o):
        # Required because unittest assertCountEqual compares items within lhs.
        if isinstance(o, REGEXP):
            return self._f == o._f and self._p == o._p  # pylint: disable=protected-access
        try:
            return bool(self._f(o))
        except TypeError:
            # Required because unittest assertCountEqual compares vs a sentinel value.
            return False

    def __ne__(self, o):
        return not self._f(o)

    def __repr__(self):
        return "<REGEXP(%s)>" % self._p


class ONEOF(object):
    """ONEOF(options_list) check value in options_list."""

    def __init__(self, container):
        if not hasattr(container, "__contains__"):
            raise TypeError("%r is not a container" % container)
        if not container:
            raise ValueError("%r is empty" % container)
        self._c = container

    def __eq__(self, o):
        return o in self._c

    def __ne__(self, o):
        return o not in self._c

    def __repr__(self):
        return "<ONEOF(%s)>" % ",".join(repr(i) for i in self._c)


class HAS(object):
    """HAS(value, func) check value in object (optionally converted by func)."""

    def __init__(self, value, func=None):
        if func is None:
            self._r, self._f = "", lambda i: i
        elif _FuncArgCount(func) != 1:
            raise TypeError("%s is not callable with 1 arg" % func)
        else:
            self._r, self._f = ", " + getattr(func, "func_name", repr(func)), func
        self._v = value

    def __eq__(self, o):
        return self._v in self._f(o)

    def __ne__(self, o):
        return self._v not in self._f(o)

    def __repr__(self):
        return "<HAS(%r%s)>" % (self._v, self._r)


class IS(object):
    """IS(test_function) like IS(callable)."""

    def __init__(self, func):
        if _FuncArgCount(func) != 1:
            raise TypeError("%s is not callable with 1 arg" % func)
        self._f = func

    def __eq__(self, o):
        return bool(self._f(o))

    def __ne__(self, o):
        return not self._f(o)

    def __repr__(self):
        return "<IS(%s)>" % getattr(self._f, "func_name", repr(self._f))


class INSTANCEOF(object):
    """INSTANCEOF(class) is a "short" for IS(lambda x: isinstance(x, class))."""

    def __init__(self, klass):
        """Provide a klass to compare to.

        Args:
          klass: A class or tuple of classes to check, ie, str or (int, long).
        """
        if (not all(issubclass(k, object) for k in klass) if type(klass) == type(
            ()) else not issubclass(klass, object)):
            raise TypeError("%r is not a new-style class" % klass)
        self._c = klass

    def __eq__(self, o):
        return isinstance(o, self._c)

    def __ne__(self, o):
        return not self.__eq__(o)

    def __repr__(self):
        return "<INSTANCEOF(%r)>" % self._c


class EQUIV(object):
    """EQUIV(func, x) is a shortcut for IS(lambda y: func(x) == func(y))."""

    def __init__(self, func, x):
        self._x = x
        self._r = getattr(func, "func_name", repr(func))
        self._f = func
        # Will raise TypeError if func is not a function or not unary
        _ = func(x)

    def __eq__(self, o):
        try:
            return self._f(self._x) == self._f(o)
        except TypeError:
            return False

    def __ne__(self, o):
        try:
            return self._f(self._x) != self._f(o)
        except TypeError:
            return True

    def __repr__(self):
        return "<EQUIV(%s(%r))>" % (self._r, self._x)


class HASKEYVALUE(object):
    """HASKEYVALUE(key, value) check object contains key and value."""

    def __init__(self, key, value):
        self._k = key
        self._v = value

    def __eq__(self, o):
        try:
            return o[self._k] == self._v
        except (KeyError, TypeError):
            return False

    def __ne__(self, o):
        try:
            return o[self._k] != self._v
        except (KeyError, TypeError):
            return True

    def __repr__(self):
        return "<HASKEYVALUE(%r, %r)>" % (self._k, self._v)


class HASATTRVALUE(object):
    """HASATTRVALUE(attr, value) check object has attribute with value."""

    def __init__(self, attr, value):
        self._a = attr
        self._v = value

    def __eq__(self, o):
        try:
            return getattr(o, self._a) == self._v
        except (AttributeError, TypeError):
            return False

    def __ne__(self, o):
        try:
            return getattr(o, self._a) != self._v
        except (AttributeError, TypeError):
            return True

    def __repr__(self):
        return "<HASATTRVALUE(%r, %r)>" % (self._a, self._v)


class HASALLOF(object):
    """HASALLOF(*values) checks that all values are in a collection."""

    def __init__(self, *values):
        """Save arguments as values to check for.

        Attempt to convert values to a set.  If values is not hashable, store them
        as-is.  In either case, set the hashable flag for reference when determining
        equality.

        Args:
          *values: A collection of values to search for.
        """
        try:
            self._values = set(values)
            self._hashable = True
        except TypeError:
            self._values = values
            self._hashable = False

    def __eq__(self, collection):
        """Determine if all self._values are in a given collection.

        Args:
          collection: collection to search for values in.

        Returns:
          True if self._values is not empty and all self._values are in collection,
          False otherwise.
        """
        if not self._values or (not self._hashable and
                                isinstance(collection, collections.Hashable)):
            return False
        if self._hashable:
            try:
                return self._values.issubset(set(collection))
            except TypeError:
                pass
        return all(v in collection for v in self._values)

    def __ne__(self, collection):
        return not self.__eq__(collection)

    def __repr__(self):
        return "<HASALLOF(%s)>" % ", ".join(repr(m) for m in self._values)


class ALLOF(object):
    """ALLOF(*matchers) check object is equal to all matchers."""

    def __init__(self, *matchers):
        self._matchers = matchers

    def __eq__(self, o):
        return self._matchers and all(m == o for m in self._matchers)

    def __ne__(self, o):
        return not self.__eq__(o)

    def __repr__(self):
        return "<ALLOF(%s)>" % ", ".join(repr(m) for m in self._matchers)


class ANYOF(object):
    """ANYOF(*matchers) check object is equal to any matcher."""

    def __init__(self, *matchers):
        self._matchers = matchers

    def __eq__(self, o):
        return self._matchers and any(m == o for m in self._matchers)

    def __ne__(self, o):
        return not self.__eq__(o)

    def __repr__(self):
        return "<ANYOF(%s)>" % ", ".join(repr(m) for m in self._matchers)


class NOT(object):
    """NOT(matcher) negate a matcher."""

    def __init__(self, matcher):
        self._matcher = matcher

    def __eq__(self, o):
        return self._matcher != o

    def __ne__(self, o):
        return not self.__eq__(o)

    def __repr__(self):
        return "<NOT(%r)>" % self._matcher


class HASMETHODVALUE(object):
    """HASMETHODVALUE(method, value) check calling methods returns value.

    This calls the compared object's method and checks if it returned value.
    Short form for HASATTRVALUE(method, IS(lambda x: x() == value)).

    """

    def __init__(self, method, value):
        self._value = value
        self._method = method

    # Deliberately do not catch exceptions because that would swallow invalid
    # parameter signatures (RETURNS(42, b=True) != lambda a:0, silently raising
    # TypeError.)

    def __eq__(self, o):
        return getattr(o, self._method)() == self._value

    def __ne__(self, o):
        return getattr(o, self._method)() != self._value

    def __repr__(self):
        return "<HASMETHODVALUE(%r, %r)>" % (self._method, self._value)


class ArgCaptor(object):
    """Simple argument captor for mocks.

    Defaults to using mock.ANY, but you can override the underlying matcher.

    Example usage:
      Code:
        d = { 'a': { ...}, 'b': {...}, 'c': {...}}
        for key, value in d.iteritems():
          # storage is mocked.
          storage.save(key, value)

      Test:
        captor = ArgCaptor(matcher=mock.ANY)
        mock_storage.save.assert_any_call('b', captor)
        # More asserts on captor.arg

    More discussion at
    https://groups.google.com/a/google.com/forum/#!topic/python-style/HFZyEBnTJbk.
    """

    def __init__(self, matcher=None):
        self._matcher = matcher if matcher is not None else mock.ANY
        self._arg = None

    @property
    def arg(self):
        return self._arg

    def __eq__(self, o):
        if self._matcher == o:
            # Mock asserts will iterate over args passed to the mocked method
            # checking each matcher. Once all matchers match, iteration stops,
            # and captor.arg will contain the value the user is looking for.
            self._arg = o
            return True
        return False

    def __ne__(self, o):
        return not self.__eq__(o)

    def __repr__(self):
        return "<ARGCAPTOR(%s)>" % self._matcher


def _FuncArgCount(f, builtin=1):
    """Counts the number of arguments f takes.

    In detail, this looks for the actual implementation of f through up to
    (arbitrarily) 16 layers of __call__ to find its code object, then returns the
    number of positional arguments that code accepts.

    The self parameter of bound methods is not counted, because it is bound.
    Unbound methods include their self parameter in the count.

    Builtins cannot be introspected this way and are assumed to take the number of
    arguments passed in as the builtin param.

    Non-callables will return None.

    Args:
      f: an arbitrary value
      builtin: int - How many arguments builtins are assumed to take.

    Returns:
      The number of arguments or None.
    """

    # Limit the recursion through the chain of __call__'s so that we don't hang on
    # evil objects. (e.g. class Solipsist: def __getattr__(self, a): return self).
    recursion_limit = 16
    try:
        while getattr(f, '__code__', None) is None and recursion_limit > 0:
            if isinstance(f, type(len)):
                return builtin

            # This accepts a func property as a synonym for __call__ for backwards
            # compatibility with previous versions of this code.
            f = getattr(f, "func", f.__call__)
            recursion_limit -= 1

        # Check for bound methods.
        has_bound_self = 1 if getattr(f, '__self__', None) is not None else 0

        return f.__code__.co_argcount - has_bound_self

    except AttributeError:
        # Probably missing __call__ or __code__
        return None


__all__ = tuple(n for n in locals() if re.match("^[A-Z]+$", n))
