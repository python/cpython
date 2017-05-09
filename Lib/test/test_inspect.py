import re
import sys
import types
import unittest
import inspect
import linecache
import datetime
import textwrap
from UserList import UserList
from UserDict import UserDict

from test.support import run_unittest, check_py3k_warnings, have_unicode

with check_py3k_warnings(
        ("tuple parameter unpacking has been removed", SyntaxWarning),
        quiet=True):
    from test import inspect_fodder as mod
    from test import inspect_fodder2 as mod2

# C module for test_findsource_binary
try:
    import unicodedata
except ImportError:
    unicodedata = None

# Functions tested in this suite:
# ismodule, isclass, ismethod, isfunction, istraceback, isframe, iscode,
# isbuiltin, isroutine, isgenerator, isgeneratorfunction, getmembers,
# getdoc, getfile, getmodule, getsourcefile, getcomments, getsource,
# getclasstree, getargspec, getargvalues, formatargspec, formatargvalues,
# currentframe, stack, trace, isdatadescriptor

# NOTE: There are some additional tests relating to interaction with
#       zipimport in the test_zipimport_support test module.

modfile = mod.__file__
if modfile.endswith(('c', 'o')):
    modfile = modfile[:-1]

import __builtin__

try:
    1 // 0
except:
    tb = sys.exc_traceback

git = mod.StupidGit()

class IsTestBase(unittest.TestCase):
    predicates = set([inspect.isbuiltin, inspect.isclass, inspect.iscode,
                      inspect.isframe, inspect.isfunction, inspect.ismethod,
                      inspect.ismodule, inspect.istraceback,
                      inspect.isgenerator, inspect.isgeneratorfunction])

    def istest(self, predicate, exp):
        obj = eval(exp)
        self.assertTrue(predicate(obj), '%s(%s)' % (predicate.__name__, exp))

        for other in self.predicates - set([predicate]):
            if predicate == inspect.isgeneratorfunction and\
               other == inspect.isfunction:
                continue
            self.assertFalse(other(obj), 'not %s(%s)' % (other.__name__, exp))

def generator_function_example(self):
    for i in xrange(2):
        yield i

class TestPredicates(IsTestBase):
    def test_sixteen(self):
        count = len(filter(lambda x:x.startswith('is'), dir(inspect)))
        # This test is here for remember you to update Doc/library/inspect.rst
        # which claims there are 16 such functions
        expected = 16
        err_msg = "There are %d (not %d) is* functions" % (count, expected)
        self.assertEqual(count, expected, err_msg)


    def test_excluding_predicates(self):
        self.istest(inspect.isbuiltin, 'sys.exit')
        self.istest(inspect.isbuiltin, '[].append')
        self.istest(inspect.iscode, 'mod.spam.func_code')
        self.istest(inspect.isframe, 'tb.tb_frame')
        self.istest(inspect.isfunction, 'mod.spam')
        self.istest(inspect.ismethod, 'mod.StupidGit.abuse')
        self.istest(inspect.ismethod, 'git.argue')
        self.istest(inspect.ismodule, 'mod')
        self.istest(inspect.istraceback, 'tb')
        self.istest(inspect.isdatadescriptor, '__builtin__.file.closed')
        self.istest(inspect.isdatadescriptor, '__builtin__.file.softspace')
        self.istest(inspect.isgenerator, '(x for x in xrange(2))')
        self.istest(inspect.isgeneratorfunction, 'generator_function_example')
        if hasattr(types, 'GetSetDescriptorType'):
            self.istest(inspect.isgetsetdescriptor,
                        'type(tb.tb_frame).f_locals')
        else:
            self.assertFalse(inspect.isgetsetdescriptor(type(tb.tb_frame).f_locals))
        if hasattr(types, 'MemberDescriptorType'):
            self.istest(inspect.ismemberdescriptor, 'datetime.timedelta.days')
        else:
            self.assertFalse(inspect.ismemberdescriptor(datetime.timedelta.days))

    def test_isroutine(self):
        self.assertTrue(inspect.isroutine(mod.spam))
        self.assertTrue(inspect.isroutine([].count))

    def test_isclass(self):
        self.istest(inspect.isclass, 'mod.StupidGit')
        self.assertTrue(inspect.isclass(list))

        class newstyle(object): pass
        self.assertTrue(inspect.isclass(newstyle))

        class CustomGetattr(object):
            def __getattr__(self, attr):
                return None
        self.assertFalse(inspect.isclass(CustomGetattr()))

    def test_get_slot_members(self):
        class C(object):
            __slots__ = ("a", "b")

        x = C()
        x.a = 42
        members = dict(inspect.getmembers(x))
        self.assertIn('a', members)
        self.assertNotIn('b', members)

    def test_isabstract(self):
        from abc import ABCMeta, abstractmethod

        class AbstractClassExample(object):
            __metaclass__ = ABCMeta

            @abstractmethod
            def foo(self):
                pass

        class ClassExample(AbstractClassExample):
            def foo(self):
                pass

        a = ClassExample()

        # Test general behaviour.
        self.assertTrue(inspect.isabstract(AbstractClassExample))
        self.assertFalse(inspect.isabstract(ClassExample))
        self.assertFalse(inspect.isabstract(a))
        self.assertFalse(inspect.isabstract(int))
        self.assertFalse(inspect.isabstract(5))


class TestInterpreterStack(IsTestBase):
    def __init__(self, *args, **kwargs):
        unittest.TestCase.__init__(self, *args, **kwargs)

        git.abuse(7, 8, 9)

    def test_abuse_done(self):
        self.istest(inspect.istraceback, 'git.ex[2]')
        self.istest(inspect.isframe, 'mod.fr')

    def test_stack(self):
        self.assertTrue(len(mod.st) >= 5)
        self.assertEqual(mod.st[0][1:],
             (modfile, 16, 'eggs', ['    st = inspect.stack()\n'], 0))
        self.assertEqual(mod.st[1][1:],
             (modfile, 9, 'spam', ['    eggs(b + d, c + f)\n'], 0))
        self.assertEqual(mod.st[2][1:],
             (modfile, 43, 'argue', ['            spam(a, b, c)\n'], 0))
        self.assertEqual(mod.st[3][1:],
             (modfile, 39, 'abuse', ['        self.argue(a, b, c)\n'], 0))

    def test_trace(self):
        self.assertEqual(len(git.tr), 3)
        self.assertEqual(git.tr[0][1:], (modfile, 43, 'argue',
                                         ['            spam(a, b, c)\n'], 0))
        self.assertEqual(git.tr[1][1:], (modfile, 9, 'spam',
                                         ['    eggs(b + d, c + f)\n'], 0))
        self.assertEqual(git.tr[2][1:], (modfile, 18, 'eggs',
                                         ['    q = y // 0\n'], 0))

    def test_frame(self):
        args, varargs, varkw, locals = inspect.getargvalues(mod.fr)
        self.assertEqual(args, ['x', 'y'])
        self.assertEqual(varargs, None)
        self.assertEqual(varkw, None)
        self.assertEqual(locals, {'x': 11, 'p': 11, 'y': 14})
        self.assertEqual(inspect.formatargvalues(args, varargs, varkw, locals),
                         '(x=11, y=14)')

    def test_previous_frame(self):
        args, varargs, varkw, locals = inspect.getargvalues(mod.fr.f_back)
        self.assertEqual(args, ['a', 'b', 'c', 'd', ['e', ['f']]])
        self.assertEqual(varargs, 'g')
        self.assertEqual(varkw, 'h')
        self.assertEqual(inspect.formatargvalues(args, varargs, varkw, locals),
             '(a=7, b=8, c=9, d=3, (e=4, (f=5,)), *g=(), **h={})')

class GetSourceBase(unittest.TestCase):
    # Subclasses must override.
    fodderFile = None

    def __init__(self, *args, **kwargs):
        unittest.TestCase.__init__(self, *args, **kwargs)

        with open(inspect.getsourcefile(self.fodderFile)) as fp:
            self.source = fp.read()

    def sourcerange(self, top, bottom):
        lines = self.source.split("\n")
        return "\n".join(lines[top-1:bottom]) + "\n"

    def assertSourceEqual(self, obj, top, bottom):
        self.assertEqual(inspect.getsource(obj),
                         self.sourcerange(top, bottom))

class TestRetrievingSourceCode(GetSourceBase):
    fodderFile = mod

    def test_getclasses(self):
        classes = inspect.getmembers(mod, inspect.isclass)
        self.assertEqual(classes,
                         [('FesteringGob', mod.FesteringGob),
                          ('MalodorousPervert', mod.MalodorousPervert),
                          ('ParrotDroppings', mod.ParrotDroppings),
                          ('StupidGit', mod.StupidGit),
                          ('Tit', mod.MalodorousPervert),
                         ])
        tree = inspect.getclasstree([cls[1] for cls in classes])
        self.assertEqual(tree,
                         [(mod.ParrotDroppings, ()),
                          [(mod.FesteringGob, (mod.MalodorousPervert,
                                                  mod.ParrotDroppings))
                           ],
                          (mod.StupidGit, ()),
                          [(mod.MalodorousPervert, (mod.StupidGit,)),
                           [(mod.FesteringGob, (mod.MalodorousPervert,
                                                   mod.ParrotDroppings))
                            ]
                           ]
                          ])
        tree = inspect.getclasstree([cls[1] for cls in classes], True)
        self.assertEqual(tree,
                         [(mod.ParrotDroppings, ()),
                          (mod.StupidGit, ()),
                          [(mod.MalodorousPervert, (mod.StupidGit,)),
                           [(mod.FesteringGob, (mod.MalodorousPervert,
                                                   mod.ParrotDroppings))
                            ]
                           ]
                          ])

    def test_getfunctions(self):
        functions = inspect.getmembers(mod, inspect.isfunction)
        self.assertEqual(functions, [('eggs', mod.eggs),
                                     ('spam', mod.spam)])

    @unittest.skipIf(sys.flags.optimize >= 2,
                     "Docstrings are omitted with -O2 and above")
    def test_getdoc(self):
        self.assertEqual(inspect.getdoc(mod), 'A module docstring.')
        self.assertEqual(inspect.getdoc(mod.StupidGit),
                         'A longer,\n\nindented\n\ndocstring.')
        self.assertEqual(inspect.getdoc(git.abuse),
                         'Another\n\ndocstring\n\ncontaining\n\ntabs')

    def test_cleandoc(self):
        self.assertEqual(inspect.cleandoc('An\n    indented\n    docstring.'),
                         'An\nindented\ndocstring.')

    def test_getcomments(self):
        self.assertEqual(inspect.getcomments(mod), '# line 1\n')
        self.assertEqual(inspect.getcomments(mod.StupidGit), '# line 20\n')

    def test_getmodule(self):
        # Check actual module
        self.assertEqual(inspect.getmodule(mod), mod)
        # Check class (uses __module__ attribute)
        self.assertEqual(inspect.getmodule(mod.StupidGit), mod)
        # Check a method (no __module__ attribute, falls back to filename)
        self.assertEqual(inspect.getmodule(mod.StupidGit.abuse), mod)
        # Do it again (check the caching isn't broken)
        self.assertEqual(inspect.getmodule(mod.StupidGit.abuse), mod)
        # Check a builtin
        self.assertEqual(inspect.getmodule(str), sys.modules["__builtin__"])
        # Check filename override
        self.assertEqual(inspect.getmodule(None, modfile), mod)

    def test_getsource(self):
        self.assertSourceEqual(git.abuse, 29, 39)
        self.assertSourceEqual(mod.StupidGit, 21, 46)

    def test_getsourcefile(self):
        self.assertEqual(inspect.getsourcefile(mod.spam), modfile)
        self.assertEqual(inspect.getsourcefile(git.abuse), modfile)
        fn = "_non_existing_filename_used_for_sourcefile_test.py"
        co = compile("None", fn, "exec")
        self.assertEqual(inspect.getsourcefile(co), None)
        linecache.cache[co.co_filename] = (1, None, "None", co.co_filename)
        self.assertEqual(inspect.getsourcefile(co), fn)

    def test_getfile(self):
        self.assertEqual(inspect.getfile(mod.StupidGit), mod.__file__)

    def test_getmodule_recursion(self):
        from types import ModuleType
        name = '__inspect_dummy'
        m = sys.modules[name] = ModuleType(name)
        m.__file__ = "<string>" # hopefully not a real filename...
        m.__loader__ = "dummy"  # pretend the filename is understood by a loader
        exec "def x(): pass" in m.__dict__
        self.assertEqual(inspect.getsourcefile(m.x.func_code), '<string>')
        del sys.modules[name]
        inspect.getmodule(compile('a=10','','single'))

    def test_proceed_with_fake_filename(self):
        '''doctest monkeypatches linecache to enable inspection'''
        fn, source = '<test>', 'def x(): pass\n'
        getlines = linecache.getlines
        def monkey(filename, module_globals=None):
            if filename == fn:
                return source.splitlines(True)
            else:
                return getlines(filename, module_globals)
        linecache.getlines = monkey
        try:
            ns = {}
            exec compile(source, fn, 'single') in ns
            inspect.getsource(ns["x"])
        finally:
            linecache.getlines = getlines

class TestDecorators(GetSourceBase):
    fodderFile = mod2

    def test_wrapped_decorator(self):
        self.assertSourceEqual(mod2.wrapped, 14, 17)

    def test_replacing_decorator(self):
        self.assertSourceEqual(mod2.gone, 9, 10)

class TestOneliners(GetSourceBase):
    fodderFile = mod2
    def test_oneline_lambda(self):
        # Test inspect.getsource with a one-line lambda function.
        self.assertSourceEqual(mod2.oll, 25, 25)

    def test_threeline_lambda(self):
        # Test inspect.getsource with a three-line lambda function,
        # where the second and third lines are _not_ indented.
        self.assertSourceEqual(mod2.tll, 28, 30)

    def test_twoline_indented_lambda(self):
        # Test inspect.getsource with a two-line lambda function,
        # where the second line _is_ indented.
        self.assertSourceEqual(mod2.tlli, 33, 34)

    def test_onelinefunc(self):
        # Test inspect.getsource with a regular one-line function.
        self.assertSourceEqual(mod2.onelinefunc, 37, 37)

    def test_manyargs(self):
        # Test inspect.getsource with a regular function where
        # the arguments are on two lines and _not_ indented and
        # the body on the second line with the last arguments.
        self.assertSourceEqual(mod2.manyargs, 40, 41)

    def test_twolinefunc(self):
        # Test inspect.getsource with a regular function where
        # the body is on two lines, following the argument list and
        # continued on the next line by a \\.
        self.assertSourceEqual(mod2.twolinefunc, 44, 45)

    def test_lambda_in_list(self):
        # Test inspect.getsource with a one-line lambda function
        # defined in a list, indented.
        self.assertSourceEqual(mod2.a[1], 49, 49)

    def test_anonymous(self):
        # Test inspect.getsource with a lambda function defined
        # as argument to another function.
        self.assertSourceEqual(mod2.anonymous, 55, 55)

class TestBuggyCases(GetSourceBase):
    fodderFile = mod2

    def test_with_comment(self):
        self.assertSourceEqual(mod2.with_comment, 58, 59)

    def test_multiline_sig(self):
        self.assertSourceEqual(mod2.multiline_sig[0], 63, 64)

    def test_nested_class(self):
        self.assertSourceEqual(mod2.func69().func71, 71, 72)

    def test_one_liner_followed_by_non_name(self):
        self.assertSourceEqual(mod2.func77, 77, 77)

    def test_one_liner_dedent_non_name(self):
        self.assertSourceEqual(mod2.cls82.func83, 83, 83)

    def test_with_comment_instead_of_docstring(self):
        self.assertSourceEqual(mod2.func88, 88, 90)

    def test_method_in_dynamic_class(self):
        self.assertSourceEqual(mod2.method_in_dynamic_class, 95, 97)

    @unittest.skipIf(
        not hasattr(unicodedata, '__file__') or
            unicodedata.__file__[-4:] in (".pyc", ".pyo"),
        "unicodedata is not an external binary module")
    def test_findsource_binary(self):
        self.assertRaises(IOError, inspect.getsource, unicodedata)
        self.assertRaises(IOError, inspect.findsource, unicodedata)

    def test_findsource_code_in_linecache(self):
        lines = ["x=1"]
        co = compile(lines[0], "_dynamically_created_file", "exec")
        self.assertRaises(IOError, inspect.findsource, co)
        self.assertRaises(IOError, inspect.getsource, co)
        linecache.cache[co.co_filename] = (1, None, lines, co.co_filename)
        self.assertEqual(inspect.findsource(co), (lines,0))
        self.assertEqual(inspect.getsource(co), lines[0])

    def test_findsource_without_filename(self):
        for fname in ['', '<string>']:
            co = compile('x=1', fname, "exec")
            self.assertRaises(IOError, inspect.findsource, co)
            self.assertRaises(IOError, inspect.getsource, co)


class _BrokenDataDescriptor(object):
    """
    A broken data descriptor. See bug #1785.
    """
    def __get__(*args):
        raise AssertionError("should not __get__ data descriptors")

    def __set__(*args):
        raise RuntimeError

    def __getattr__(*args):
        raise AssertionError("should not __getattr__ data descriptors")


class _BrokenMethodDescriptor(object):
    """
    A broken method descriptor. See bug #1785.
    """
    def __get__(*args):
        raise AssertionError("should not __get__ method descriptors")

    def __getattr__(*args):
        raise AssertionError("should not __getattr__ method descriptors")


# Helper for testing classify_class_attrs.
def attrs_wo_objs(cls):
    return [t[:3] for t in inspect.classify_class_attrs(cls)]


class TestClassesAndFunctions(unittest.TestCase):
    def test_classic_mro(self):
        # Test classic-class method resolution order.
        class A:    pass
        class B(A): pass
        class C(A): pass
        class D(B, C): pass

        expected = (D, B, A, C)
        got = inspect.getmro(D)
        self.assertEqual(expected, got)

    def test_newstyle_mro(self):
        # The same w/ new-class MRO.
        class A(object):    pass
        class B(A): pass
        class C(A): pass
        class D(B, C): pass

        expected = (D, B, C, A, object)
        got = inspect.getmro(D)
        self.assertEqual(expected, got)

    def assertArgSpecEquals(self, routine, args_e, varargs_e = None,
                            varkw_e = None, defaults_e = None,
                            formatted = None):
        args, varargs, varkw, defaults = inspect.getargspec(routine)
        self.assertEqual(args, args_e)
        self.assertEqual(varargs, varargs_e)
        self.assertEqual(varkw, varkw_e)
        self.assertEqual(defaults, defaults_e)
        if formatted is not None:
            self.assertEqual(inspect.formatargspec(args, varargs, varkw, defaults),
                             formatted)

    def test_getargspec(self):
        self.assertArgSpecEquals(mod.eggs, ['x', 'y'], formatted = '(x, y)')

        self.assertArgSpecEquals(mod.spam,
                                 ['a', 'b', 'c', 'd', ['e', ['f']]],
                                 'g', 'h', (3, (4, (5,))),
                                 '(a, b, c, d=3, (e, (f,))=(4, (5,)), *g, **h)')

        with check_py3k_warnings(("tuple parameter unpacking has been removed",
                                  SyntaxWarning),
                                 quiet=True):
            exec(textwrap.dedent('''
                def spam_deref(a, b, c, d=3, (e, (f,))=(4, (5,)), *g, **h):
                    def eggs():
                        return a + b + c + d + e + f + g + h
                    return eggs
            '''))
        self.assertArgSpecEquals(spam_deref,
                                 ['a', 'b', 'c', 'd', ['e', ['f']]],
                                 'g', 'h', (3, (4, (5,))),
                                 '(a, b, c, d=3, (e, (f,))=(4, (5,)), *g, **h)')

    def test_getargspec_method(self):
        class A(object):
            def m(self):
                pass
        self.assertArgSpecEquals(A.m, ['self'])

    def test_getargspec_sublistofone(self):
        with check_py3k_warnings(
                ("tuple parameter unpacking has been removed", SyntaxWarning),
                ("parenthesized argument names are invalid", SyntaxWarning)):
            exec 'def sublistOfOne((foo,)): return 1'
            self.assertArgSpecEquals(sublistOfOne, [['foo']])

            exec 'def sublistOfOne((foo,)): return (lambda: foo)'
            self.assertArgSpecEquals(sublistOfOne, [['foo']])

            exec 'def fakeSublistOfOne((foo)): return 1'
            self.assertArgSpecEquals(fakeSublistOfOne, ['foo'])

            exec 'def sublistOfOne((foo)): return (lambda: foo)'
            self.assertArgSpecEquals(sublistOfOne, ['foo'])


    def _classify_test(self, newstyle):
        """Helper for testing that classify_class_attrs finds a bunch of
        different kinds of attributes on a given class.
        """
        if newstyle:
            base = object
        else:
            class base:
                pass

        class A(base):
            def s(): pass
            s = staticmethod(s)

            def c(cls): pass
            c = classmethod(c)

            def getp(self): pass
            p = property(getp)

            def m(self): pass

            def m1(self): pass

            datablob = '1'

            dd = _BrokenDataDescriptor()
            md = _BrokenMethodDescriptor()

        attrs = attrs_wo_objs(A)
        self.assertIn(('s', 'static method', A), attrs, 'missing static method')
        self.assertIn(('c', 'class method', A), attrs, 'missing class method')
        self.assertIn(('p', 'property', A), attrs, 'missing property')
        self.assertIn(('m', 'method', A), attrs, 'missing plain method')
        self.assertIn(('m1', 'method', A), attrs, 'missing plain method')
        self.assertIn(('datablob', 'data', A), attrs, 'missing data')
        self.assertIn(('md', 'method', A), attrs, 'missing method descriptor')
        self.assertIn(('dd', 'data', A), attrs, 'missing data descriptor')

        class B(A):
            def m(self): pass

        attrs = attrs_wo_objs(B)
        self.assertIn(('s', 'static method', A), attrs, 'missing static method')
        self.assertIn(('c', 'class method', A), attrs, 'missing class method')
        self.assertIn(('p', 'property', A), attrs, 'missing property')
        self.assertIn(('m', 'method', B), attrs, 'missing plain method')
        self.assertIn(('m1', 'method', A), attrs, 'missing plain method')
        self.assertIn(('datablob', 'data', A), attrs, 'missing data')
        self.assertIn(('md', 'method', A), attrs, 'missing method descriptor')
        self.assertIn(('dd', 'data', A), attrs, 'missing data descriptor')


        class C(A):
            def m(self): pass
            def c(self): pass

        attrs = attrs_wo_objs(C)
        self.assertIn(('s', 'static method', A), attrs, 'missing static method')
        self.assertIn(('c', 'method', C), attrs, 'missing plain method')
        self.assertIn(('p', 'property', A), attrs, 'missing property')
        self.assertIn(('m', 'method', C), attrs, 'missing plain method')
        self.assertIn(('m1', 'method', A), attrs, 'missing plain method')
        self.assertIn(('datablob', 'data', A), attrs, 'missing data')
        self.assertIn(('md', 'method', A), attrs, 'missing method descriptor')
        self.assertIn(('dd', 'data', A), attrs, 'missing data descriptor')

        class D(B, C):
            def m1(self): pass

        attrs = attrs_wo_objs(D)
        self.assertIn(('s', 'static method', A), attrs, 'missing static method')
        if newstyle:
            self.assertIn(('c', 'method', C), attrs, 'missing plain method')
        else:
            self.assertIn(('c', 'class method', A), attrs, 'missing class method')
        self.assertIn(('p', 'property', A), attrs, 'missing property')
        self.assertIn(('m', 'method', B), attrs, 'missing plain method')
        self.assertIn(('m1', 'method', D), attrs, 'missing plain method')
        self.assertIn(('datablob', 'data', A), attrs, 'missing data')
        self.assertIn(('md', 'method', A), attrs, 'missing method descriptor')
        self.assertIn(('dd', 'data', A), attrs, 'missing data descriptor')


    def test_classify_oldstyle(self):
        """classify_class_attrs finds static methods, class methods,
        properties, normal methods, and data attributes on an old-style
        class.
        """
        self._classify_test(False)


    def test_classify_newstyle(self):
        """Just like test_classify_oldstyle, but for a new-style class.
        """
        self._classify_test(True)

    def test_classify_builtin_types(self):
        # Simple sanity check that all built-in types can have their
        # attributes classified.
        for name in dir(__builtin__):
            builtin = getattr(__builtin__, name)
            if isinstance(builtin, type):
                inspect.classify_class_attrs(builtin)

    def test_getmembers_method(self):
        # Old-style classes
        class B:
            def f(self):
                pass

        self.assertIn(('f', B.f), inspect.getmembers(B))
        # contrary to spec, ismethod() is also True for unbound methods
        # (see #1785)
        self.assertIn(('f', B.f), inspect.getmembers(B, inspect.ismethod))
        b = B()
        self.assertIn(('f', b.f), inspect.getmembers(b))
        self.assertIn(('f', b.f), inspect.getmembers(b, inspect.ismethod))

        # New-style classes
        class B(object):
            def f(self):
                pass

        self.assertIn(('f', B.f), inspect.getmembers(B))
        self.assertIn(('f', B.f), inspect.getmembers(B, inspect.ismethod))
        b = B()
        self.assertIn(('f', b.f), inspect.getmembers(b))
        self.assertIn(('f', b.f), inspect.getmembers(b, inspect.ismethod))


class TestGetcallargsFunctions(unittest.TestCase):

    # tuple parameters are named '.1', '.2', etc.
    is_tuplename = re.compile(r'^\.\d+$').match

    def assertEqualCallArgs(self, func, call_params_string, locs=None):
        locs = dict(locs or {}, func=func)
        r1 = eval('func(%s)' % call_params_string, None, locs)
        r2 = eval('inspect.getcallargs(func, %s)' % call_params_string, None,
                  locs)
        self.assertEqual(r1, r2)

    def assertEqualException(self, func, call_param_string, locs=None):
        locs = dict(locs or {}, func=func)
        try:
            eval('func(%s)' % call_param_string, None, locs)
        except Exception, ex1:
            pass
        else:
            self.fail('Exception not raised')
        try:
            eval('inspect.getcallargs(func, %s)' % call_param_string, None,
                 locs)
        except Exception, ex2:
            pass
        else:
            self.fail('Exception not raised')
        self.assertIs(type(ex1), type(ex2))
        self.assertEqual(str(ex1), str(ex2))

    def makeCallable(self, signature):
        """Create a function that returns its locals(), excluding the
        autogenerated '.1', '.2', etc. tuple param names (if any)."""
        with check_py3k_warnings(
            ("tuple parameter unpacking has been removed", SyntaxWarning),
            quiet=True):
            code = ("lambda %s: dict(i for i in locals().items() "
                    "if not is_tuplename(i[0]))")
            return eval(code % signature, {'is_tuplename' : self.is_tuplename})

    def test_plain(self):
        f = self.makeCallable('a, b=1')
        self.assertEqualCallArgs(f, '2')
        self.assertEqualCallArgs(f, '2, 3')
        self.assertEqualCallArgs(f, 'a=2')
        self.assertEqualCallArgs(f, 'b=3, a=2')
        self.assertEqualCallArgs(f, '2, b=3')
        # expand *iterable / **mapping
        self.assertEqualCallArgs(f, '*(2,)')
        self.assertEqualCallArgs(f, '*[2]')
        self.assertEqualCallArgs(f, '*(2, 3)')
        self.assertEqualCallArgs(f, '*[2, 3]')
        self.assertEqualCallArgs(f, '**{"a":2}')
        self.assertEqualCallArgs(f, 'b=3, **{"a":2}')
        self.assertEqualCallArgs(f, '2, **{"b":3}')
        self.assertEqualCallArgs(f, '**{"b":3, "a":2}')
        # expand UserList / UserDict
        self.assertEqualCallArgs(f, '*UserList([2])')
        self.assertEqualCallArgs(f, '*UserList([2, 3])')
        self.assertEqualCallArgs(f, '**UserDict(a=2)')
        self.assertEqualCallArgs(f, '2, **UserDict(b=3)')
        self.assertEqualCallArgs(f, 'b=2, **UserDict(a=3)')
        # unicode keyword args
        self.assertEqualCallArgs(f, '**{u"a":2}')
        self.assertEqualCallArgs(f, 'b=3, **{u"a":2}')
        self.assertEqualCallArgs(f, '2, **{u"b":3}')
        self.assertEqualCallArgs(f, '**{u"b":3, u"a":2}')

    def test_varargs(self):
        f = self.makeCallable('a, b=1, *c')
        self.assertEqualCallArgs(f, '2')
        self.assertEqualCallArgs(f, '2, 3')
        self.assertEqualCallArgs(f, '2, 3, 4')
        self.assertEqualCallArgs(f, '*(2,3,4)')
        self.assertEqualCallArgs(f, '2, *[3,4]')
        self.assertEqualCallArgs(f, '2, 3, *UserList([4])')

    def test_varkw(self):
        f = self.makeCallable('a, b=1, **c')
        self.assertEqualCallArgs(f, 'a=2')
        self.assertEqualCallArgs(f, '2, b=3, c=4')
        self.assertEqualCallArgs(f, 'b=3, a=2, c=4')
        self.assertEqualCallArgs(f, 'c=4, **{"a":2, "b":3}')
        self.assertEqualCallArgs(f, '2, c=4, **{"b":3}')
        self.assertEqualCallArgs(f, 'b=2, **{"a":3, "c":4}')
        self.assertEqualCallArgs(f, '**UserDict(a=2, b=3, c=4)')
        self.assertEqualCallArgs(f, '2, c=4, **UserDict(b=3)')
        self.assertEqualCallArgs(f, 'b=2, **UserDict(a=3, c=4)')
        # unicode keyword args
        self.assertEqualCallArgs(f, 'c=4, **{u"a":2, u"b":3}')
        self.assertEqualCallArgs(f, '2, c=4, **{u"b":3}')
        self.assertEqualCallArgs(f, 'b=2, **{u"a":3, u"c":4}')

    def test_varkw_only(self):
        # issue11256:
        f = self.makeCallable('**c')
        self.assertEqualCallArgs(f, '')
        self.assertEqualCallArgs(f, 'a=1')
        self.assertEqualCallArgs(f, 'a=1, b=2')
        self.assertEqualCallArgs(f, 'c=3, **{"a": 1, "b": 2}')
        self.assertEqualCallArgs(f, '**UserDict(a=1, b=2)')
        self.assertEqualCallArgs(f, 'c=3, **UserDict(a=1, b=2)')

    def test_tupleargs(self):
        f = self.makeCallable('(b,c), (d,(e,f))=(0,[1,2])')
        self.assertEqualCallArgs(f, '(2,3)')
        self.assertEqualCallArgs(f, '[2,3]')
        self.assertEqualCallArgs(f, 'UserList([2,3])')
        self.assertEqualCallArgs(f, '(2,3), (4,(5,6))')
        self.assertEqualCallArgs(f, '(2,3), (4,[5,6])')
        self.assertEqualCallArgs(f, '(2,3), [4,UserList([5,6])]')

    def test_multiple_features(self):
        f = self.makeCallable('a, b=2, (c,(d,e))=(3,[4,5]), *f, **g')
        self.assertEqualCallArgs(f, '2, 3, (4,[5,6]), 7')
        self.assertEqualCallArgs(f, '2, 3, *[(4,[5,6]), 7], x=8')
        self.assertEqualCallArgs(f, '2, 3, x=8, *[(4,[5,6]), 7]')
        self.assertEqualCallArgs(f, '2, x=8, *[3, (4,[5,6]), 7], y=9')
        self.assertEqualCallArgs(f, 'x=8, *[2, 3, (4,[5,6])], y=9')
        self.assertEqualCallArgs(f, 'x=8, *UserList([2, 3, (4,[5,6])]), '
                                 '**{"y":9, "z":10}')
        self.assertEqualCallArgs(f, '2, x=8, *UserList([3, (4,[5,6])]), '
                                 '**UserDict(y=9, z=10)')

    def test_errors(self):
        f0 = self.makeCallable('')
        f1 = self.makeCallable('a, b')
        f2 = self.makeCallable('a, b=1')
        # f0 takes no arguments
        self.assertEqualException(f0, '1')
        self.assertEqualException(f0, 'x=1')
        self.assertEqualException(f0, '1,x=1')
        # f1 takes exactly 2 arguments
        self.assertEqualException(f1, '')
        self.assertEqualException(f1, '1')
        self.assertEqualException(f1, 'a=2')
        self.assertEqualException(f1, 'b=3')
        # f2 takes at least 1 argument
        self.assertEqualException(f2, '')
        self.assertEqualException(f2, 'b=3')
        for f in f1, f2:
            # f1/f2 takes exactly/at most 2 arguments
            self.assertEqualException(f, '2, 3, 4')
            self.assertEqualException(f, '1, 2, 3, a=1')
            self.assertEqualException(f, '2, 3, 4, c=5')
            self.assertEqualException(f, '2, 3, 4, a=1, c=5')
            # f got an unexpected keyword argument
            self.assertEqualException(f, 'c=2')
            self.assertEqualException(f, '2, c=3')
            self.assertEqualException(f, '2, 3, c=4')
            self.assertEqualException(f, '2, c=4, b=3')
            if have_unicode:
                self.assertEqualException(f, '**{u"\u03c0\u03b9": 4}')
            # f got multiple values for keyword argument
            self.assertEqualException(f, '1, a=2')
            self.assertEqualException(f, '1, **{"a":2}')
            self.assertEqualException(f, '1, 2, b=3')
            # XXX: Python inconsistency
            # - for functions and bound methods: unexpected keyword 'c'
            # - for unbound methods: multiple values for keyword 'a'
            #self.assertEqualException(f, '1, c=3, a=2')
        f = self.makeCallable('(a,b)=(0,1)')
        self.assertEqualException(f, '1')
        self.assertEqualException(f, '[1]')
        self.assertEqualException(f, '(1,2,3)')
        # issue11256:
        f3 = self.makeCallable('**c')
        self.assertEqualException(f3, '1, 2')
        self.assertEqualException(f3, '1, 2, a=1, b=2')


class TestGetcallargsFunctionsCellVars(TestGetcallargsFunctions):

    def makeCallable(self, signature):
        """Create a function that returns its locals(), excluding the
        autogenerated '.1', '.2', etc. tuple param names (if any)."""
        with check_py3k_warnings(
            ("tuple parameter unpacking has been removed", SyntaxWarning),
            quiet=True):
            code = """lambda %s: (
                    (lambda: a+b+c+d+d+e+f+g+h), # make parameters cell vars
                    dict(i for i in locals().items()
                         if not is_tuplename(i[0]))
                )[1]"""
            return eval(code % signature, {'is_tuplename' : self.is_tuplename})


class TestGetcallargsMethods(TestGetcallargsFunctions):

    def setUp(self):
        class Foo(object):
            pass
        self.cls = Foo
        self.inst = Foo()

    def makeCallable(self, signature):
        assert 'self' not in signature
        mk = super(TestGetcallargsMethods, self).makeCallable
        self.cls.method = mk('self, ' + signature)
        return self.inst.method

class TestGetcallargsUnboundMethods(TestGetcallargsMethods):

    def makeCallable(self, signature):
        super(TestGetcallargsUnboundMethods, self).makeCallable(signature)
        return self.cls.method

    def assertEqualCallArgs(self, func, call_params_string, locs=None):
        return super(TestGetcallargsUnboundMethods, self).assertEqualCallArgs(
            *self._getAssertEqualParams(func, call_params_string, locs))

    def assertEqualException(self, func, call_params_string, locs=None):
        return super(TestGetcallargsUnboundMethods, self).assertEqualException(
            *self._getAssertEqualParams(func, call_params_string, locs))

    def _getAssertEqualParams(self, func, call_params_string, locs=None):
        assert 'inst' not in call_params_string
        locs = dict(locs or {}, inst=self.inst)
        return (func, 'inst,' + call_params_string, locs)

def test_main():
    run_unittest(
        TestDecorators, TestRetrievingSourceCode, TestOneliners, TestBuggyCases,
        TestInterpreterStack, TestClassesAndFunctions, TestPredicates,
        TestGetcallargsFunctions, TestGetcallargsFunctionsCellVars,
        TestGetcallargsMethods, TestGetcallargsUnboundMethods)

if __name__ == "__main__":
    test_main()
