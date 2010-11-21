import test.test_support
compiler = test.test_support.import_module('compiler', deprecated=True)
from compiler.ast import flatten
import os, sys, time, unittest
from random import random
from StringIO import StringIO

# How much time in seconds can pass before we print a 'Still working' message.
_PRINT_WORKING_MSG_INTERVAL = 5 * 60

class TrivialContext(object):
    def __enter__(self):
        return self
    def __exit__(self, *exc_info):
        pass

class CompilerTest(unittest.TestCase):

    def testCompileLibrary(self):
        # A simple but large test.  Compile all the code in the
        # standard library and its test suite.  This doesn't verify
        # that any of the code is correct, merely the compiler is able
        # to generate some kind of code for it.

        next_time = time.time() + _PRINT_WORKING_MSG_INTERVAL
        # warning: if 'os' or 'test_support' are moved in some other dir,
        # they should be changed here.
        libdir = os.path.dirname(os.__file__)
        testdir = os.path.dirname(test.test_support.__file__)

        for dir in [libdir, testdir]:
            for basename in os.listdir(dir):
                # Print still working message since this test can be really slow
                if next_time <= time.time():
                    next_time = time.time() + _PRINT_WORKING_MSG_INTERVAL
                    print >>sys.__stdout__, \
                       '  testCompileLibrary still working, be patient...'
                    sys.__stdout__.flush()

                if not basename.endswith(".py"):
                    continue
                if not TEST_ALL and random() < 0.98:
                    continue
                path = os.path.join(dir, basename)
                if test.test_support.verbose:
                    print "compiling", path
                f = open(path, "U")
                buf = f.read()
                f.close()
                if "badsyntax" in basename or "bad_coding" in basename:
                    self.assertRaises(SyntaxError, compiler.compile,
                                      buf, basename, "exec")
                else:
                    try:
                        compiler.compile(buf, basename, "exec")
                    except Exception, e:
                        args = list(e.args)
                        args.append("in file %s]" % basename)
                        #args[0] += "[in file %s]" % basename
                        e.args = tuple(args)
                        raise

    def testNewClassSyntax(self):
        compiler.compile("class foo():pass\n\n","<string>","exec")

    def testYieldExpr(self):
        compiler.compile("def g(): yield\n\n", "<string>", "exec")

    def testKeywordAfterStarargs(self):
        def f(*args, **kwargs):
            self.assertEqual((args, kwargs), ((2,3), {'x': 1, 'y': 4}))
        c = compiler.compile('f(x=1, *(2, 3), y=4)', '<string>', 'exec')
        exec c in {'f': f}

        self.assertRaises(SyntaxError, compiler.parse, "foo(a=1, b)")
        self.assertRaises(SyntaxError, compiler.parse, "foo(1, *args, 3)")

    def testTryExceptFinally(self):
        # Test that except and finally clauses in one try stmt are recognized
        c = compiler.compile("try:\n 1//0\nexcept:\n e = 1\nfinally:\n f = 1",
                             "<string>", "exec")
        dct = {}
        exec c in dct
        self.assertEqual(dct.get('e'), 1)
        self.assertEqual(dct.get('f'), 1)

    def testDefaultArgs(self):
        self.assertRaises(SyntaxError, compiler.parse, "def foo(a=1, b): pass")

    def testDocstrings(self):
        c = compiler.compile('"doc"', '<string>', 'exec')
        self.assertIn('__doc__', c.co_names)
        c = compiler.compile('def f():\n "doc"', '<string>', 'exec')
        g = {}
        exec c in g
        self.assertEqual(g['f'].__doc__, "doc")

    def testLineNo(self):
        # Test that all nodes except Module have a correct lineno attribute.
        filename = __file__
        if filename.endswith((".pyc", ".pyo")):
            filename = filename[:-1]
        tree = compiler.parseFile(filename)
        self.check_lineno(tree)

    def check_lineno(self, node):
        try:
            self._check_lineno(node)
        except AssertionError:
            print node.__class__, node.lineno
            raise

    def _check_lineno(self, node):
        if not node.__class__ in NOLINENO:
            self.assertIsInstance(node.lineno, int,
                "lineno=%s on %s" % (node.lineno, node.__class__))
            self.assertTrue(node.lineno > 0,
                "lineno=%s on %s" % (node.lineno, node.__class__))
        for child in node.getChildNodes():
            self.check_lineno(child)

    def testFlatten(self):
        self.assertEqual(flatten([1, [2]]), [1, 2])
        self.assertEqual(flatten((1, (2,))), [1, 2])

    def testNestedScope(self):
        c = compiler.compile('def g():\n'
                             '    a = 1\n'
                             '    def f(): return a + 2\n'
                             '    return f()\n'
                             'result = g()',
                             '<string>',
                             'exec')
        dct = {}
        exec c in dct
        self.assertEqual(dct.get('result'), 3)

    def testGenExp(self):
        c = compiler.compile('list((i,j) for i in range(3) if i < 3'
                             '           for j in range(4) if j > 2)',
                             '<string>',
                             'eval')
        self.assertEqual(eval(c), [(0, 3), (1, 3), (2, 3)])

    def testSetLiteral(self):
        c = compiler.compile('{1, 2, 3}', '<string>', 'eval')
        self.assertEqual(eval(c), {1,2,3})
        c = compiler.compile('{1, 2, 3,}', '<string>', 'eval')
        self.assertEqual(eval(c), {1,2,3})

    def testDictLiteral(self):
        c = compiler.compile('{1:2, 2:3, 3:4}', '<string>', 'eval')
        self.assertEqual(eval(c), {1:2, 2:3, 3:4})
        c = compiler.compile('{1:2, 2:3, 3:4,}', '<string>', 'eval')
        self.assertEqual(eval(c), {1:2, 2:3, 3:4})

    def testSetComp(self):
        c = compiler.compile('{x for x in range(1, 4)}', '<string>', 'eval')
        self.assertEqual(eval(c), {1, 2, 3})
        c = compiler.compile('{x * y for x in range(3) if x != 0'
                             '       for y in range(4) if y != 0}',
                             '<string>',
                             'eval')
        self.assertEqual(eval(c), {1, 2, 3, 4, 6})

    def testDictComp(self):
        c = compiler.compile('{x:x+1 for x in range(1, 4)}', '<string>', 'eval')
        self.assertEqual(eval(c), {1:2, 2:3, 3:4})
        c = compiler.compile('{(x, y) : y for x in range(2) if x != 0'
                             '            for y in range(3) if y != 0}',
                             '<string>',
                             'eval')
        self.assertEqual(eval(c), {(1, 2): 2, (1, 1): 1})

    def testWith(self):
        # SF bug 1638243
        c = compiler.compile('from __future__ import with_statement\n'
                             'def f():\n'
                             '    with TrivialContext():\n'
                             '        return 1\n'
                             'result = f()',
                             '<string>',
                             'exec' )
        dct = {'TrivialContext': TrivialContext}
        exec c in dct
        self.assertEqual(dct.get('result'), 1)

    def testWithAss(self):
        c = compiler.compile('from __future__ import with_statement\n'
                             'def f():\n'
                             '    with TrivialContext() as tc:\n'
                             '        return 1\n'
                             'result = f()',
                             '<string>',
                             'exec' )
        dct = {'TrivialContext': TrivialContext}
        exec c in dct
        self.assertEqual(dct.get('result'), 1)

    def testWithMult(self):
        events = []
        class Ctx:
            def __init__(self, n):
                self.n = n
            def __enter__(self):
                events.append(self.n)
            def __exit__(self, *args):
                pass
        c = compiler.compile('from __future__ import with_statement\n'
                             'def f():\n'
                             '    with Ctx(1) as tc, Ctx(2) as tc2:\n'
                             '        return 1\n'
                             'result = f()',
                             '<string>',
                             'exec' )
        dct = {'Ctx': Ctx}
        exec c in dct
        self.assertEqual(dct.get('result'), 1)
        self.assertEqual(events, [1, 2])

    def testGlobal(self):
        code = compiler.compile('global x\nx=1', '<string>', 'exec')
        d1 = {'__builtins__': {}}
        d2 = {}
        exec code in d1, d2
        # x should be in the globals dict
        self.assertEqual(d1.get('x'), 1)

    def testPrintFunction(self):
        c = compiler.compile('from __future__ import print_function\n'
                             'print("a", "b", sep="**", end="++", '
                                    'file=output)',
                             '<string>',
                             'exec' )
        dct = {'output': StringIO()}
        exec c in dct
        self.assertEqual(dct['output'].getvalue(), 'a**b++')

    def _testErrEnc(self, src, text, offset):
        try:
            compile(src, "", "exec")
        except SyntaxError, e:
            self.assertEqual(e.offset, offset)
            self.assertEqual(e.text, text)

    def testSourceCodeEncodingsError(self):
        # Test SyntaxError with encoding definition
        sjis = "print '\x83\x70\x83\x43\x83\x5c\x83\x93', '\n"
        ascii = "print '12345678', '\n"
        encdef = "#! -*- coding: ShiftJIS -*-\n"

        # ascii source without encdef
        self._testErrEnc(ascii, ascii, 19)

        # ascii source with encdef
        self._testErrEnc(encdef+ascii, ascii, 19)

        # non-ascii source with encdef
        self._testErrEnc(encdef+sjis, sjis, 19)

        # ShiftJIS source without encdef
        self._testErrEnc(sjis, sjis, 19)


NOLINENO = (compiler.ast.Module, compiler.ast.Stmt, compiler.ast.Discard)

###############################################################################
# code below is just used to trigger some possible errors, for the benefit of
# testLineNo
###############################################################################

class Toto:
    """docstring"""
    pass

a, b = 2, 3
[c, d] = 5, 6
l = [(x, y) for x, y in zip(range(5), range(5,10))]
l[0]
l[3:4]
d = {'a': 2}
d = {}
d = {x: y for x, y in zip(range(5), range(5,10))}
s = {x for x in range(10)}
s = {1}
t = ()
t = (1, 2)
l = []
l = [1, 2]
if l:
    pass
else:
    a, b = b, a

try:
    print yo
except:
    yo = 3
else:
    yo += 3

try:
    a += b
finally:
    b = 0

from math import *

###############################################################################

def test_main():
    global TEST_ALL
    TEST_ALL = test.test_support.is_resource_enabled("cpu")
    test.test_support.run_unittest(CompilerTest)

if __name__ == "__main__":
    test_main()
