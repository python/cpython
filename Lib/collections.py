__all__ = ['deque', 'defaultdict', 'NamedTuple']

from _collections import deque, defaultdict
from operator import itemgetter as _itemgetter
import sys as _sys

def NamedTuple(typename, s):
    """Returns a new subclass of tuple with named fields.

    >>> Point = NamedTuple('Point', 'x y')
    >>> Point.__doc__           # docstring for the new class
    'Point(x, y)'
    >>> p = Point(11, y=22)     # instantiate with positional args or keywords
    >>> p[0] + p[1]             # works just like the tuple (11, 22)
    33
    >>> x, y = p                # unpacks just like a tuple
    >>> x, y
    (11, 22)
    >>> p.x + p.y               # fields also accessable by name
    33
    >>> p                       # readable __repr__ with name=value style
    Point(x=11, y=22)

    """

    field_names = s.split()
    nargs = len(field_names)

    def __new__(cls, *args, **kwds):
        if kwds:
            try:
                args += tuple(kwds[name] for name in field_names[len(args):])
            except KeyError, name:
                raise TypeError('%s missing required argument: %s' % (typename, name))
        if len(args) != nargs:
            raise TypeError('%s takes exactly %d arguments (%d given)' % (typename, nargs, len(args)))
        return tuple.__new__(cls, args)

    repr_template = '%s(%s)' % (typename, ', '.join('%s=%%r' % name for name in field_names))

    m = dict(vars(tuple))       # pre-lookup superclass methods (for faster lookup)
    m.update(__doc__= '%s(%s)' % (typename, ', '.join(field_names)),
             __slots__ = (),    # no per-instance dict (so instances are same size as tuples)
             __new__ = __new__,
             __repr__ = lambda self, _format=repr_template.__mod__: _format(self),
             __module__ = _sys._getframe(1).f_globals['__name__'],
             )
    m.update((name, property(_itemgetter(index))) for index, name in enumerate(field_names))

    return type(typename, (tuple,), m)


if __name__ == '__main__':
    # verify that instances are pickable
    from cPickle import loads, dumps
    Point = NamedTuple('Point', 'x y')
    p = Point(x=10, y=20)
    assert p == loads(dumps(p))

    import doctest
    TestResults = NamedTuple('TestResults', 'failed attempted')
    print TestResults(*doctest.testmod())
