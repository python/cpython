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


not_allowed = {
    'abs': '<built-in function abs>',
    'all': '<built-in function all>',
    'any': '<built-in function any>',
    'ascii': '<built-in function ascii>',
    'bin': '<built-in function bin>',
    'breakpoint': '<built-in function breakpoint>',
    'callable': '<built-in function callable>',
    'chr': '<built-in function chr>',
    'compile': '<built-in function compile>',
    'delattr': '<built-in function delattr>',
    'divmod': '<built-in function divmod>',
    'eval': '<built-in function eval>',
    'exec': '<built-in function exec>',
    'format': '<built-in function format>',
    'getattr': '<built-in function getattr>',
    'hasattr': '<built-in function hasattr>',
    'hash': '<built-in function hash>',
    'hex': '<built-in function hex>',
    'id': '<built-in function id>',
    'isinstance': '<built-in function isinstance>',
    'issubclass': '<built-in function issubclass>',
    'iter': '<built-in function iter>',
    'len': '<built-in function len>',
    'max': '<built-in function max>',
    'min': '<built-in function min>',
    'next': '<built-in function next>',
    'oct': '<built-in function oct>',
    'ord': '<built-in function ord>',
    'pow': '<built-in function pow>',
    'repr': '<built-in function repr>',
    'round': '<built-in function round>',
    'setattr': '<built-in function setattr>',
    'sorted': '<built-in function sorted>',
    'sum': '<built-in function sum>',
    'None': 'None',
    'memoryview': "<class 'memoryview'>",
    'classmethod': "<class 'classmethod'>",
    'enumerate': "<class 'enumerate'>",
    'filter': "<class 'filter'>",
    'map': "<class 'map'>",
    'range': "<class 'range'>",
    'reversed': "<class 'reversed'>",
    'slice': "<class 'slice'>",
    'staticmethod': "<class 'staticmethod'>",
    'super': "<class 'super'>",
    'type': "<class 'type'>",
    'open': '<built-in function open>',
    'execfile': '<function execfile at 0x103555820>',
    'runfile': '<function runfile at 0x104847310>'
}


class CustomDefaultDict(collections.defaultdict):
    def __init__(self, arg):
        import inspect

        if inspect.isfunction(arg):
            if arg.__code__.co_argcount > 0:
                raise TypeError('function with arguments not allowed')

        if arg is None:
            raise TypeError('NoneType not allowed')

        if inspect.isbuiltin(arg):
            if str(arg) in not_allowed.values():
                raise TypeError('builtin_function_or_method with arguments not allowed')

        if isinstance(arg, type):
            if str(arg) in not_allowed.values():
                raise TypeError('class which requires arguments not allowed')

        super().__init__(arg)
