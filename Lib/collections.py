__all__ = ['deque', 'defaultdict', 'NamedTuple']

from _collections import deque, defaultdict
from operator import itemgetter as _itemgetter
import sys as _sys

def NamedTuple(typename, s):
    """Returns a new subclass of tuple with named fields.

    >>> Point = NamedTuple('Point', 'x y')
    >>> Point.__doc__                   # docstring for the new class
    'Point(x, y)'
    >>> p = Point(11, y=22)             # instantiate with positional args or keywords
    >>> p[0] + p[1]                     # works just like the tuple (11, 22)
    33
    >>> x, y = p                        # unpacks just like a tuple
    >>> x, y
    (11, 22)
    >>> p.x + p.y                       # fields also accessable by name
    33
    >>> p                               # readable __repr__ with name=value style
    Point(x=11, y=22)
    >>> p.__replace__('x', 100)         # __replace__() is like str.replace() but targets a named field
    Point(x=100, y=22)
    >>> d = dict(zip(p.__fields__, p))  # use __fields__ to make a dictionary
    >>> d['x']
    11

    """

    field_names = tuple(s.replace(',', ' ').split())   # names separated by spaces and/or commas
    if not ''.join((typename,) + field_names).replace('_', '').isalnum():
        raise ValueError('Type names and field names can only contain alphanumeric characters and underscores')
    argtxt = ', '.join(field_names)
    reprtxt = ', '.join('%s=%%r' % name for name in field_names)
    template = '''class %(typename)s(tuple):
        '%(typename)s(%(argtxt)s)'
        __slots__ = ()
        __fields__ = %(field_names)r
        def __new__(cls, %(argtxt)s):
            return tuple.__new__(cls, (%(argtxt)s,))
        def __repr__(self):
            return '%(typename)s(%(reprtxt)s)' %% self
        def __replace__(self, field, value):
            'Return a new %(typename)s object replacing one field with a new value'
            return %(typename)s(**dict(zip(%(field_names)r, self) + [(field, value)]))
    ''' % locals()
    for i, name in enumerate(field_names):
        template += '\n        %s = property(itemgetter(%d))\n' % (name, i)
    m = dict(itemgetter=_itemgetter)
    exec template in m
    result = m[typename]
    if hasattr(_sys, '_getframe'):
        result.__module__ = _sys._getframe(1).f_globals['__name__']
    return result






if __name__ == '__main__':
    # verify that instances can be pickled
    from cPickle import loads, dumps
    Point = NamedTuple('Point', 'x, y')
    p = Point(x=10, y=20)
    assert p == loads(dumps(p))

    import doctest
    TestResults = NamedTuple('TestResults', 'failed attempted')
    print TestResults(*doctest.testmod())
