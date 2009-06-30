import unittest
from test import test_support
import sys
new = test_support.import_module('new', deprecated=True)

class NewTest(unittest.TestCase):
    def test_spam(self):
        class Eggs:
            def get_yolks(self):
                return self.yolks

        m = new.module('Spam')
        m.Eggs = Eggs
        sys.modules['Spam'] = m
        import Spam

        def get_more_yolks(self):
            return self.yolks + 3

        # new.classobj()
        C = new.classobj('Spam', (Spam.Eggs,), {'get_more_yolks': get_more_yolks})

        # new.instance()
        c = new.instance(C, {'yolks': 3})

        o = new.instance(C)
        self.assertEqual(o.__dict__, {}, "new __dict__ should be empty")
        del o
        o = new.instance(C, None)
        self.assertEqual(o.__dict__, {}, "new __dict__ should be empty")
        del o

        def break_yolks(self):
            self.yolks = self.yolks - 2

        # new.instancemethod()
        im = new.instancemethod(break_yolks, c, C)

        self.assertEqual(c.get_yolks(), 3,
            'Broken call of hand-crafted class instance')
        self.assertEqual(c.get_more_yolks(), 6,
            'Broken call of hand-crafted class instance')

        im()
        self.assertEqual(c.get_yolks(), 1,
            'Broken call of hand-crafted instance method')
        self.assertEqual(c.get_more_yolks(), 4,
            'Broken call of hand-crafted instance method')

        im = new.instancemethod(break_yolks, c)
        im()
        self.assertEqual(c.get_yolks(), -1)

        # Verify that dangerous instance method creation is forbidden
        self.assertRaises(TypeError, new.instancemethod, break_yolks, None)

        # Verify that instancemethod() doesn't allow keyword args
        self.assertRaises(TypeError, new.instancemethod, break_yolks, c, kw=1)

    def test_scope(self):
        # It's unclear what the semantics should be for a code object compiled
        # at module scope, but bound and run in a function.  In CPython, `c' is
        # global (by accident?) while in Jython, `c' is local.  The intent of
        # the test clearly is to make `c' global, so let's be explicit about it.
        codestr = '''
        global c
        a = 1
        b = 2
        c = a + b
        '''

        codestr = "\n".join(l.strip() for l in codestr.splitlines())

        ccode = compile(codestr, '<string>', 'exec')
        # Jython doesn't have a __builtins__, so use a portable alternative
        import __builtin__
        g = {'c': 0, '__builtins__': __builtin__}

        # this test could be more robust
        func = new.function(ccode, g)
        func()
        self.assertEqual(g['c'], 3, 'Could not create a proper function object')

    def test_function(self):
        # test the various extended flavors of function.new
        def f(x):
            def g(y):
                return x + y
            return g
        g = f(4)
        new.function(f.func_code, {}, "blah")
        g2 = new.function(g.func_code, {}, "blah", (2,), g.func_closure)
        self.assertEqual(g2(), 6)
        g3 = new.function(g.func_code, {}, "blah", None, g.func_closure)
        self.assertEqual(g3(5), 9)
        def test_closure(func, closure, exc):
            self.assertRaises(exc, new.function, func.func_code, {}, "", None, closure)

        test_closure(g, None, TypeError) # invalid closure
        test_closure(g, (1,), TypeError) # non-cell in closure
        test_closure(g, (1, 1), ValueError) # closure is wrong size
        test_closure(f, g.func_closure, ValueError) # no closure needed

    # Note: Jython will never have new.code()
    if hasattr(new, 'code'):
        def test_code(self):
            # bogus test of new.code()
            def f(a): pass

            c = f.func_code
            argcount = c.co_argcount
            nlocals = c.co_nlocals
            stacksize = c.co_stacksize
            flags = c.co_flags
            codestring = c.co_code
            constants = c.co_consts
            names = c.co_names
            varnames = c.co_varnames
            filename = c.co_filename
            name = c.co_name
            firstlineno = c.co_firstlineno
            lnotab = c.co_lnotab
            freevars = c.co_freevars
            cellvars = c.co_cellvars

            d = new.code(argcount, nlocals, stacksize, flags, codestring,
                         constants, names, varnames, filename, name,
                         firstlineno, lnotab, freevars, cellvars)

            # test backwards-compatibility version with no freevars or cellvars
            d = new.code(argcount, nlocals, stacksize, flags, codestring,
                         constants, names, varnames, filename, name,
                         firstlineno, lnotab)

            # negative co_argcount used to trigger a SystemError
            self.assertRaises(ValueError, new.code,
                -argcount, nlocals, stacksize, flags, codestring,
                constants, names, varnames, filename, name, firstlineno, lnotab)

            # negative co_nlocals used to trigger a SystemError
            self.assertRaises(ValueError, new.code,
                argcount, -nlocals, stacksize, flags, codestring,
                constants, names, varnames, filename, name, firstlineno, lnotab)

            # non-string co_name used to trigger a Py_FatalError
            self.assertRaises(TypeError, new.code,
                argcount, nlocals, stacksize, flags, codestring,
                constants, (5,), varnames, filename, name, firstlineno, lnotab)

            # new.code used to be a way to mutate a tuple...
            class S(str):
                pass
            t = (S("ab"),)
            d = new.code(argcount, nlocals, stacksize, flags, codestring,
                         constants, t, varnames, filename, name,
                         firstlineno, lnotab)
            self.assertTrue(type(t[0]) is S, "eek, tuple changed under us!")

def test_main():
    test_support.run_unittest(NewTest)

if __name__ == "__main__":
    test_main()
