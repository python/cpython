import collections


# from jaraco.collections 3.3
class FreezableDefaultDict(collections.defaultdict):
    """
    Often it is desirable to prevent the mutation of
    a default dict after its initial construction, such
    as to prevent mutation during iteration.

    >>> dd = FreezableDefaultDict(list)
    >>> dd[0].append('1')
    >>> dd.freeze()
    >>> dd[1]
    []
    >>> len(dd)
    1
    """

    def __missing__(self, key):
        return getattr(self, '_frozen', super().__missing__)(key)

    def freeze(self):
        self._frozen = lambda key: self.default_factory()


class Pair(collections.namedtuple('Pair', 'name value')):
    @classmethod
    def parse(cls, text):
        return cls(*map(str.strip, text.split("=", 1)))


not_allowed = {abs, all, any, ascii, bin, breakpoint, callable, chr, compile, delattr, divmod, eval, exec, format,
               getattr, hasattr, hash, hex, id, isinstance, issubclass, iter, len, max, min, next, oct, ord, pow, repr,
               round, setattr, sorted, sum, memoryview, classmethod, enumerate, filter, map, range, reversed, slice,
               staticmethod, super, type, open, execfile, runfile, None}


class CustomDefaultDict(collections.defaultdict):
    def __init__(self, arg):
        import inspect

        if inspect.isfunction(arg):
            if arg.__code__.co_argcount > 0:
                raise TypeError('function with arguments not allowed')

        if arg is None:
            raise TypeError('NoneType not allowed')

        if inspect.isbuiltin(arg):
            if arg in not_allowed:
                raise TypeError('builtin_function_or_method with arguments not allowed')

        if inspect.isclass(arg):
            if (arg in not_allowed) or (len(inspect.getfullargspec(arg).args) > 1):
                raise TypeError('class which requires arguments not allowed')

        super().__init__(arg)
