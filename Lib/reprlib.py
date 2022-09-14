"""Redo the builtin repr() (representation) but with limits on most sizes."""

__all__ = ["Repr", "repr", "recursive_repr"]

import builtins
from itertools import islice
from _thread import get_ident


def recursive_repr(fillvalue='...'):
    'Decorator to make a repr function return fillvalue for a recursive call'

    def decorating_function(user_function):
        repr_running = set()

        def wrapper(self):
            key = id(self), get_ident()
            if key in repr_running:
                return fillvalue
            repr_running.add(key)
            try:
                result = user_function(self)
            finally:
                repr_running.discard(key)
            return result

        # Can't use functools.wraps() here because of bootstrap issues
        wrapper.__module__ = getattr(user_function, '__module__')
        wrapper.__doc__ = getattr(user_function, '__doc__')
        wrapper.__name__ = getattr(user_function, '__name__')
        wrapper.__qualname__ = getattr(user_function, '__qualname__')
        wrapper.__annotations__ = getattr(user_function, '__annotations__', {})
        return wrapper

    return decorating_function


class Repr:

    def __init__(
        self, *, maxlevel=6, maxtuple=6, maxlist=6, maxarray=5, maxdict=4,
        maxset=6, maxfrozenset=6, maxdeque=6, maxstring=30, maxlong=40,
        maxother=30, fillvalue='...', indent=None,
    ):
        self.maxlevel = maxlevel
        self.maxtuple = maxtuple
        self.maxlist = maxlist
        self.maxarray = maxarray
        self.maxdict = maxdict
        self.maxset = maxset
        self.maxfrozenset = maxfrozenset
        self.maxdeque = maxdeque
        self.maxstring = maxstring
        self.maxlong = maxlong
        self.maxother = maxother
        self.fillvalue = fillvalue
        self.indent = indent

    def repr(self, x):
        return self.repr1(x, self.maxlevel)

    def repr1(self, x, level):
        typename = type(x).__name__
        if ' ' in typename:
            parts = typename.split()
            typename = '_'.join(parts)
        if hasattr(self, 'repr_' + typename):
            return getattr(self, 'repr_' + typename)(x, level)
        else:
            return self.repr_instance(x, level)

    def _join(self, pieces, level):
        if not pieces:
            return ''

        indent = _validate_indent(self.indent)
        if indent is None:
            return ', '.join(pieces)

        sep = ',\n' + (self.maxlevel - level + 1) * indent
        return sep.join(('', *pieces, ''))[1:-len(indent) or None]

    def _repr_iterable(
        self,
        x,
        level,
        *,
        left,
        right,
        maxiter,
        trailing_comma=False,
    ):
        pieces = list(self._gen_pieces(x, level, maxiter))
        body = self._get_iterable_body(pieces, level, trailing_comma)
        return f'{left}{body}{right}'

    def _get_iterable_body(self, pieces, level, trailing_comma):
        if not pieces:
            return ''

        if level <= 0:
            return self.fillvalue

        result = self._join(pieces, level)
        if self._need_tailing_comma(pieces, trailing_comma):
            return result + ','
        return result

    def _gen_pieces(self, obj, level, maxiter):
        for elem in islice(obj, maxiter):
            yield self.repr1(elem, level - 1)

        if len(obj) > maxiter:
            yield self.fillvalue

    def _need_tailing_comma(self, pieces, trailing_comma):
        return trailing_comma and len(pieces) == 1 and self.indent is None

    def repr_tuple(self, x, level):
        return self._repr_iterable(
            x,
            level,
            left='(',
            right=')',
            maxiter=self.maxtuple,
            trailing_comma=True,
        )

    def repr_list(self, x, level):
        return self._repr_iterable(
            x,
            level,
            left='[',
            right=']',
            maxiter=self.maxlist,
        )

    def repr_array(self, x, level):
        if not x:
            return "array('%s')" % x.typecode
        header = "array('%s', [" % x.typecode
        return self._repr_iterable(
            x,
            level,
            left=header,
            right='])',
            maxiter=self.maxarray,
        )

    def repr_set(self, x, level):
        if not x:
            return 'set()'
        return self._repr_iterable(
            _possibly_sorted(x),
            level,
            left='{',
            right='}',
            maxiter=self.maxset,
        )

    def repr_frozenset(self, x, level):
        if not x:
            return 'frozenset()'
        return self._repr_iterable(
            _possibly_sorted(x),
            level,
            left='frozenset({',
            right='})',
            maxiter=self.maxfrozenset,
        )

    def repr_deque(self, x, level):
        return self._repr_iterable(
            x,
            level,
            left='deque([',
            right='])',
            maxiter=self.maxdeque,
        )

    def repr_dict(self, x, level):
        left = '{'
        right = '}'
        pieces = list(self._gen_dict_pieces(x, level, self.maxdict))
        body = self._get_iterable_body(pieces, level, trailing_comma=False)
        return f'{left}{body}{right}'

    def _gen_dict_pieces(self, obj, level, maxiter):
        for key in islice(_possibly_sorted(obj), maxiter):
            keyrepr = self.repr1(key, level - 1)
            valrepr = self.repr1(obj[key], level - 1)
            yield f'{keyrepr}: {valrepr}'

        if len(obj) > maxiter:
            yield self.fillvalue

    def repr_str(self, x, level):
        s = builtins.repr(x[:self.maxstring])
        if len(s) > self.maxstring:
            i = max(0, (self.maxstring-3)//2)
            j = max(0, self.maxstring-3-i)
            s = builtins.repr(x[:i] + x[len(x)-j:])
            s = s[:i] + self.fillvalue + s[len(s)-j:]
        return s

    def repr_int(self, x, level):
        s = builtins.repr(x)  # XXX Hope this isn't too slow...
        if len(s) > self.maxlong:
            i = max(0, (self.maxlong-3)//2)
            j = max(0, self.maxlong-3-i)
            s = s[:i] + self.fillvalue + s[len(s)-j:]
        return s

    def repr_instance(self, x, level):
        try:
            s = builtins.repr(x)
            # Bugs in x.__repr__() can cause arbitrary
            # exceptions -- then make up something
        except Exception:
            return '<%s instance at %#x>' % (x.__class__.__name__, id(x))
        if len(s) > self.maxother:
            i = max(0, (self.maxother-3)//2)
            j = max(0, self.maxother-3-i)
            s = s[:i] + self.fillvalue + s[len(s)-j:]
        return s


def _validate_indent(indent):
    if isinstance(indent, int):
        if indent < 0:
            raise ValueError(
                f'Repr.indent cannot be negative int (was {indent!r})'
            )
        return ' ' * indent

    if indent is None or isinstance(indent, str):
        return indent

    msg = f'Repr.indent must be a str, int or None, not {type(indent)}'
    raise TypeError(msg)


def _possibly_sorted(x):
    # Since not all sequences of items can be sorted and comparison
    # functions may raise arbitrary exceptions, return an unsorted
    # sequence in that case.
    try:
        return sorted(x)
    except Exception:
        return list(x)


aRepr = Repr()
repr = aRepr.repr
