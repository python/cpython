"""Tracing metaclass.

XXX This is very much a work in progress.

"""

import types, sys

class TraceMetaClass:
    """Metaclass for tracing.

    Classes defined using this metaclass have an automatic tracing
    feature -- by setting the __trace_output__ instance (or class)
    variable to a file object, trace messages about all calls are
    written to the file.  The trace formatting can be changed by
    defining a suitable __trace_call__ method.

    """

    __inited = 0

    def __init__(self, name, bases, dict):
        self.__name__ = name
        self.__bases__ = bases
        self.__dict = dict
        # XXX Can't define __dict__, alas
        self.__inited = 1

    def __getattr__(self, name):
        try:
            return self.__dict[name]
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
            self.__dict[name] = value

    def __call__(self, *args, **kw):
        inst = TracingInstance()
        inst.__meta_init__(self)
        try:
            init = inst.__getattr__('__init__')
        except AttributeError:
            init = lambda: None
        apply(init, args, kw)
        return inst

    __trace_output__ = None

class TracingInstance:
    """Helper class to represent an instance of a tracing class."""

    def __trace_call__(self, fp, fmt, *args):
        fp.write((fmt+'\n') % args)

    def __meta_init__(self, klass):
        self.__class = klass

    def __getattr__(self, name):
        # Invoked for any attr not in the instance's __dict__
        try:
            raw = self.__class.__getattr__(name)
        except AttributeError:
            raise AttributeError, name
        if type(raw) != types.FunctionType:
            return raw
        # It's a function
        fullname = self.__class.__name__ + "." + name
        if not self.__trace_output__ or name == '__trace_call__':
            return NotTracingWrapper(fullname, raw, self)
        else:
            return TracingWrapper(fullname, raw, self)

class NotTracingWrapper:
    def __init__(self, name, func, inst):
        self.__name__ = name
        self.func = func
        self.inst = inst
    def __call__(self, *args, **kw):
        return apply(self.func, (self.inst,) + args, kw)

class TracingWrapper(NotTracingWrapper):
    def __call__(self, *args, **kw):
        self.inst.__trace_call__(self.inst.__trace_output__,
                                 "calling %s, inst=%s, args=%s, kw=%s",
                                 self.__name__, self.inst, args, kw)
        try:
            rv = apply(self.func, (self.inst,) + args, kw)
        except:
            t, v, tb = sys.exc_info()
            self.inst.__trace_call__(self.inst.__trace_output__,
                                     "returning from %s with exception %s: %s",
                                     self.__name__, t, v)
            raise t, v, tb
        else:
            self.inst.__trace_call__(self.inst.__trace_output__,
                                     "returning from %s with value %s",
                                     self.__name__, rv)
            return rv

Traced = TraceMetaClass('Traced', (), {'__trace_output__': None})


def _test():
    global C, D
    class C(Traced):
        def __init__(self, x=0): self.x = x
        def m1(self, x): self.x = x
        def m2(self, y): return self.x + y
        __trace_output__ = sys.stdout
    class D(C):
        def m2(self, y): print "D.m2(%r)" % (y,); return C.m2(self, y)
        __trace_output__ = None
    x = C(4321)
    print x
    print x.x
    print x.m1(100)
    print x.m1(10)
    print x.m2(33)
    print x.m1(5)
    print x.m2(4000)
    print x.x

    print C.__init__
    print C.m2
    print D.__init__
    print D.m2

    y = D()
    print y
    print y.m1(10)
    print y.m2(100)
    print y.x

if __name__ == '__main__':
    _test()
