import unittest
from test import test_support

def funcattrs(**kwds):
    def decorate(func):
        func.__dict__.update(kwds)
        return func
    return decorate

class MiscDecorators (object):
    @staticmethod
    def author(name):
        def decorate(func):
            func.__dict__['author'] = name
            return func
        return decorate

# -----------------------------------------------

class DbcheckError (Exception):
    def __init__(self, exprstr, func, args, kwds):
        # A real version of this would set attributes here
        Exception.__init__(self, "dbcheck %r failed (func=%s args=%s kwds=%s)" %
                           (exprstr, func, args, kwds))


def dbcheck(exprstr, globals=None, locals=None):
    "Decorator to implement debugging assertions"
    def decorate(func):
        expr = compile(exprstr, "dbcheck-%s" % func.func_name, "eval")
        def check(*args, **kwds):
            if not eval(expr, globals, locals):
                raise DbcheckError(exprstr, func, args, kwds)
            return func(*args, **kwds)
        return check
    return decorate

# -----------------------------------------------

def countcalls(counts):
    "Decorator to count calls to a function"
    def decorate(func):
        name = func.func_name
        counts[name] = 0
        def call(*args, **kwds):
            counts[name] += 1
            return func(*args, **kwds)
        # XXX: Would like to say: call.func_name = func.func_name here
        #      to make nested decorators work in any order, but func_name
        #      is a readonly attribute
        return call
    return decorate

# -----------------------------------------------

def memoize(func):
    saved = {}
    def call(*args):
        try:
            return saved[args]
        except KeyError:
            res = func(*args)
            saved[args] = res
            return res
        except TypeError:
            # Unhashable argument
            return func(*args)
    return call

# -----------------------------------------------

class TestDecorators(unittest.TestCase):

    def test_single(self):
        class C(object):
            @staticmethod
            def foo(): return 42
        self.assertEqual(C.foo(), 42)
        self.assertEqual(C().foo(), 42)

    def test_staticmethod_function(self):
        @staticmethod
        def notamethod(x):
            return x
        self.assertRaises(TypeError, notamethod, 1)

    def test_dotted(self):
        decorators = MiscDecorators()
        @decorators.author('Cleese')
        def foo(): return 42
        self.assertEqual(foo(), 42)
        self.assertEqual(foo.author, 'Cleese')

    def test_argforms(self):
        # A few tests of argument passing, as we use restricted form
        # of expressions for decorators.

        def noteargs(*args, **kwds):
            def decorate(func):
                setattr(func, 'dbval', (args, kwds))
                return func
            return decorate

        args = ( 'Now', 'is', 'the', 'time' )
        kwds = dict(one=1, two=2)
        @noteargs(*args, **kwds)
        def f1(): return 42
        self.assertEqual(f1(), 42)
        self.assertEqual(f1.dbval, (args, kwds))

        @noteargs('terry', 'gilliam', eric='idle', john='cleese')
        def f2(): return 84
        self.assertEqual(f2(), 84)
        self.assertEqual(f2.dbval, (('terry', 'gilliam'),
                                     dict(eric='idle', john='cleese')))

        @noteargs(1, 2,)
        def f3(): pass
        self.assertEqual(f3.dbval, ((1, 2), {}))

    def test_dbcheck(self):
        @dbcheck('args[1] is not None')
        def f(a, b):
            return a + b
        self.assertEqual(f(1, 2), 3)
        self.assertRaises(DbcheckError, f, 1, None)

    def test_memoize(self):
        # XXX: This doesn't work unless memoize is the last decorator -
        #      see the comment in countcalls.
        counts = {}
        @countcalls(counts) @memoize
        def double(x):
            return x * 2

        self.assertEqual(counts, dict(double=0))

        # Only the first call with a given argument bumps the call count:
        #
        self.assertEqual(double(2), 4)
        self.assertEqual(counts['double'], 1)
        self.assertEqual(double(2), 4)
        self.assertEqual(counts['double'], 1)
        self.assertEqual(double(3), 6)
        self.assertEqual(counts['double'], 2)

        # Unhashable arguments do not get memoized:
        #
        self.assertEqual(double([10]), [10, 10])
        self.assertEqual(counts['double'], 3)
        self.assertEqual(double([10]), [10, 10])
        self.assertEqual(counts['double'], 4)

    def test_errors(self):
        # Test syntax restrictions - these are all compile-time errors:
        #
        for expr in [ "1+2", "x[3]", "(1, 2)" ]:
            # Sanity check: is expr is a valid expression by itself?
            compile(expr, "testexpr", "exec")

            codestr = "@%s\ndef f(): pass" % expr
            self.assertRaises(SyntaxError, compile, codestr, "test", "exec")

        # Test runtime errors

        def unimp(func):
            raise NotImplementedError
        context = dict(nullval=None, unimp=unimp)

        for expr, exc in [ ("undef", NameError),
                           ("nullval", TypeError),
                           ("nullval.attr", AttributeError),
                           ("unimp", NotImplementedError)]:
            codestr = "@%s\ndef f(): pass\nassert f() is None" % expr
            code = compile(codestr, "test", "exec")
            self.assertRaises(exc, eval, code, context)

    def test_double(self):
        class C(object):
            @funcattrs(abc=1, xyz="haha")
            @funcattrs(booh=42)
            def foo(self): return 42
        self.assertEqual(C().foo(), 42)
        self.assertEqual(C.foo.abc, 1)
        self.assertEqual(C.foo.xyz, "haha")
        self.assertEqual(C.foo.booh, 42)

    def test_order(self):
        class C(object):
            @funcattrs(abc=1) @staticmethod
            def foo(): return 42
        # This wouldn't work if staticmethod was called first
        self.assertEqual(C.foo(), 42)
        self.assertEqual(C().foo(), 42)

def test_main():
    test_support.run_unittest(TestDecorators)

if __name__=="__main__":
    test_main()
