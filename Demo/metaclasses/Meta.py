"""Generic metaclass.

XXX This is very much a work in progress.

"""

import types

class MetaMethodWrapper:

    def __init__(self, func, inst):
        self.func = func
        self.inst = inst
        self.__name__ = self.func.__name__

    def __call__(self, *args, **kw):
        return apply(self.func, (self.inst,) + args, kw)

class MetaHelper:

    __methodwrapper__ = MetaMethodWrapper # For derived helpers to override

    def __helperinit__(self, formalclass):
        self.__formalclass__ = formalclass

    def __getattr__(self, name):
        # Invoked for any attr not in the instance's __dict__
        try:
            raw = self.__formalclass__.__getattr__(name)
        except AttributeError:
            try:
                ga = self.__formalclass__.__getattr__('__usergetattr__')
            except (KeyError, AttributeError):
                raise AttributeError, name
            return ga(self, name)
        if type(raw) != types.FunctionType:
            return raw
        return self.__methodwrapper__(raw, self)

class MetaClass:

    """A generic metaclass.

    This can be subclassed to implement various kinds of meta-behavior.

    """

    __helper__ = MetaHelper             # For derived metaclasses to override

    __inited = 0

    def __init__(self, name, bases, dict):
        try:
            ga = dict['__getattr__']
        except KeyError:
            pass
        else:
            dict['__usergetattr__'] = ga
            del dict['__getattr__']
        self.__name__ = name
        self.__bases__ = bases
        self.__realdict__ = dict
        self.__inited = 1

    def __getattr__(self, name):
        try:
            return self.__realdict__[name]
        except KeyError:
            for base in self.__bases__:
                try:
                    return base.__getattr__(name)
                except AttributeError:
                    pass
            raise AttributeError, name

    def __setattr__(self, name, value):
        if not self.__inited:
            self.__dict__[name] = value
        else:
            self.__realdict__[name] = value

    def __call__(self, *args, **kw):
        inst = self.__helper__()
        inst.__helperinit__(self)
        try:
            init = inst.__getattr__('__init__')
        except AttributeError:
            init = lambda: None
        apply(init, args, kw)
        return inst


Meta = MetaClass('Meta', (), {})


def _test():
    class C(Meta):
        def __init__(self, *args):
            print "__init__, args =", args
        def m1(self, x):
            print "m1(x=%r)" % (x,)
    print C
    x = C()
    print x
    x.m1(12)
    class D(C):
        def __getattr__(self, name):
            if name[:2] == '__': raise AttributeError, name
            return "getattr:%s" % name
    x = D()
    print x.foo
    print x._foo
##     print x.__foo
##     print x.__foo__


if __name__ == '__main__':
    _test()
