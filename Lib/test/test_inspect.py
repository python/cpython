import builtins
import collections
import datetime
import functools
import importlib
import inspect
import io
import linecache
import os
from os.path import normcase
import _pickle
import pickle
import shutil
import sys
import types
import textwrap
import unicodedata
import unittest
import unittest.mock
import warnings

try:
    from concurrent.futures import ThreadPoolExecutor
except ImportError:
    ThreadPoolExecutor = None

from test.support import run_unittest, cpython_only
from test.support import MISSING_C_DOCSTRINGS, ALWAYS_EQ
from test.support.import_helper import DirsOnSysPath
from test.support.os_helper import TESTFN
from test.support.script_helper import assert_python_ok, assert_python_failure
from test import inspect_fodder as mod
from test import inspect_fodder2 as mod2
from test import support
from test import inspect_stock_annotations
from test import inspect_stringized_annotations
from test import inspect_stringized_annotations_2

from test.test_import import _ready_to_import


# Functions tested in this suite:
# ismodule, isclass, ismethod, isfunction, istraceback, isframe, iscode,
# isbuiltin, isroutine, isgenerator, isgeneratorfunction, getmembers,
# getdoc, getfile, getmodule, getsourcefile, getcomments, getsource,
# getclasstree, getargvalues, formatargspec, formatargvalues,
# currentframe, stack, trace, isdatadescriptor

# NOTE: There are some additional tests relating to interaction with
#       zipimport in the test_zipimport_support test module.

modfile = mod.__file__
if modfile.endswith(('c', 'o')):
    modfile = modfile[:-1]

# Normalize file names: on Windows, the case of file names of compiled
# modules depends on the path used to start the python executable.
modfile = normcase(modfile)

def revise(filename, *args):
    return (normcase(filename),) + args

git = mod.StupidGit()


def signatures_with_lexicographic_keyword_only_parameters():
    """
    Yields a whole bunch of functions with only keyword-only parameters,
    where those parameters are always in lexicographically sorted order.
    """
    parameters = ['a', 'bar', 'c', 'delta', 'ephraim', 'magical', 'yoyo', 'z']
    for i in range(1, 2**len(parameters)):
        p = []
        bit = 1
        for j in range(len(parameters)):
            if i & (bit << j):
                p.append(parameters[j])
        fn_text = "def foo(*, " + ", ".join(p) + "): pass"
        symbols = {}
        exec(fn_text, symbols, symbols)
        yield symbols['foo']


def unsorted_keyword_only_parameters_fn(*, throw, out, the, baby, with_,
                                        the_, bathwater):
    pass

unsorted_keyword_only_parameters = 'throw out the baby with_ the_ bathwater'.split()

class IsTestBase(unittest.TestCase):
    predicates = set([inspect.isbuiltin, inspect.isclass, inspect.iscode,
                      inspect.isframe, inspect.isfunction, inspect.ismethod,
                      inspect.ismodule, inspect.istraceback,
                      inspect.isgenerator, inspect.isgeneratorfunction,
                      inspect.iscoroutine, inspect.iscoroutinefunction,
                      inspect.isasyncgen, inspect.isasyncgenfunction])

    def istest(self, predicate, exp):
        obj = eval(exp)
        self.assertTrue(predicate(obj), '%s(%s)' % (predicate.__name__, exp))

        for other in self.predicates - set([predicate]):
            if (predicate == inspect.isgeneratorfunction or \
               predicate == inspect.isasyncgenfunction or \
               predicate == inspect.iscoroutinefunction) and \
               other == inspect.isfunction:
                continue
            self.assertFalse(other(obj), 'not %s(%s)' % (other.__name__, exp))

def generator_function_example(self):
    for i in range(2):
        yield i

async def async_generator_function_example(self):
    async for i in range(2):
        yield i

async def coroutine_function_example(self):
    return 'spam'

@types.coroutine
def gen_coroutine_function_example(self):
    yield
    return 'spam'

class TestPredicates(IsTestBase):

    def test_excluding_predicates(self):
        global tb
        self.istest(inspect.isbuiltin, 'sys.exit')
        self.istest(inspect.isbuiltin, '[].append')
        self.istest(inspect.iscode, 'mod.spam.__code__')
        try:
            1/0
        except:
            tb = sys.exc_info()[2]
            self.istest(inspect.isframe, 'tb.tb_frame')
            self.istest(inspect.istraceback, 'tb')
            if hasattr(types, 'GetSetDescriptorType'):
                self.istest(inspect.isgetsetdescriptor,
                            'type(tb.tb_frame).f_locals')
            else:
                self.assertFalse(inspect.isgetsetdescriptor(type(tb.tb_frame).f_locals))
        finally:
            # Clear traceback and all the frames and local variables hanging to it.
            tb = None
        self.istest(inspect.isfunction, 'mod.spam')
        self.istest(inspect.isfunction, 'mod.StupidGit.abuse')
        self.istest(inspect.ismethod, 'git.argue')
        self.istest(inspect.ismethod, 'mod.custom_method')
        self.istest(inspect.ismodule, 'mod')
        self.istest(inspect.isdatadescriptor, 'collections.defaultdict.default_factory')
        self.istest(inspect.isgenerator, '(x for x in range(2))')
        self.istest(inspect.isgeneratorfunction, 'generator_function_example')
        self.istest(inspect.isasyncgen,
                    'async_generator_function_example(1)')
        self.istest(inspect.isasyncgenfunction,
                    'async_generator_function_example')

        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            self.istest(inspect.iscoroutine, 'coroutine_function_example(1)')
            self.istest(inspect.iscoroutinefunction, 'coroutine_function_example')

        if hasattr(types, 'MemberDescriptorType'):
            self.istest(inspect.ismemberdescriptor, 'datetime.timedelta.days')
        else:
            self.assertFalse(inspect.ismemberdescriptor(datetime.timedelta.days))

    def test_iscoroutine(self):
        async_gen_coro = async_generator_function_example(1)
        gen_coro = gen_coroutine_function_example(1)
        coro = coroutine_function_example(1)

        self.assertFalse(
            inspect.iscoroutinefunction(gen_coroutine_function_example))
        self.assertFalse(
            inspect.iscoroutinefunction(
                functools.partial(functools.partial(
                    gen_coroutine_function_example))))
        self.assertFalse(inspect.iscoroutine(gen_coro))

        self.assertTrue(
            inspect.isgeneratorfunction(gen_coroutine_function_example))
        self.assertTrue(
            inspect.isgeneratorfunction(
                functools.partial(functools.partial(
                    gen_coroutine_function_example))))
        self.assertTrue(inspect.isgenerator(gen_coro))

        self.assertTrue(
            inspect.iscoroutinefunction(coroutine_function_example))
        self.assertTrue(
            inspect.iscoroutinefunction(
                functools.partial(functools.partial(
                    coroutine_function_example))))
        self.assertTrue(inspect.iscoroutine(coro))

        self.assertFalse(
            inspect.isgeneratorfunction(coroutine_function_example))
        self.assertFalse(
            inspect.isgeneratorfunction(
                functools.partial(functools.partial(
                    coroutine_function_example))))
        self.assertFalse(inspect.isgenerator(coro))

        self.assertTrue(
            inspect.isasyncgenfunction(async_generator_function_example))
        self.assertTrue(
            inspect.isasyncgenfunction(
                functools.partial(functools.partial(
                    async_generator_function_example))))
        self.assertTrue(inspect.isasyncgen(async_gen_coro))

        coro.close(); gen_coro.close(); # silence warnings

    def test_isawaitable(self):
        def gen(): yield
        self.assertFalse(inspect.isawaitable(gen()))

        coro = coroutine_function_example(1)
        gen_coro = gen_coroutine_function_example(1)

        self.assertTrue(inspect.isawaitable(coro))
        self.assertTrue(inspect.isawaitable(gen_coro))

        class Future:
            def __await__():
                pass
        self.assertTrue(inspect.isawaitable(Future()))
        self.assertFalse(inspect.isawaitable(Future))

        class NotFuture: pass
        not_fut = NotFuture()
        not_fut.__await__ = lambda: None
        self.assertFalse(inspect.isawaitable(not_fut))

        coro.close(); gen_coro.close() # silence warnings

    def test_isroutine(self):
        self.assertTrue(inspect.isroutine(mod.spam))
        self.assertTrue(inspect.isroutine([].count))

    def test_isclass(self):
        self.istest(inspect.isclass, 'mod.StupidGit')
        self.assertTrue(inspect.isclass(list))

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

        class AbstractClassExample(metaclass=ABCMeta):

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

    def test_isabstract_during_init_subclass(self):
        from abc import ABCMeta, abstractmethod
        isabstract_checks = []
        class AbstractChecker(metaclass=ABCMeta):
            def __init_subclass__(cls):
                isabstract_checks.append(inspect.isabstract(cls))
        class AbstractClassExample(AbstractChecker):
            @abstractmethod
            def foo(self):
                pass
        class ClassExample(AbstractClassExample):
            def foo(self):
                pass
        self.assertEqual(isabstract_checks, [True, False])

        isabstract_checks.clear()
        class AbstractChild(AbstractClassExample):
            pass
        class AbstractGrandchild(AbstractChild):
            pass
        class ConcreteGrandchild(ClassExample):
            pass
        self.assertEqual(isabstract_checks, [True, True, False])


class TestInterpreterStack(IsTestBase):
    def __init__(self, *args, **kwargs):
        unittest.TestCase.__init__(self, *args, **kwargs)

        git.abuse(7, 8, 9)

    def test_abuse_done(self):
        self.istest(inspect.istraceback, 'git.ex[2]')
        self.istest(inspect.isframe, 'mod.fr')

    def test_stack(self):
        self.assertTrue(len(mod.st) >= 5)
        self.assertEqual(revise(*mod.st[0][1:]),
             (modfile, 16, 'eggs', ['    st = inspect.stack()\n'], 0))
        self.assertEqual(revise(*mod.st[1][1:]),
             (modfile, 9, 'spam', ['    eggs(b + d, c + f)\n'], 0))
        self.assertEqual(revise(*mod.st[2][1:]),
             (modfile, 43, 'argue', ['            spam(a, b, c)\n'], 0))
        self.assertEqual(revise(*mod.st[3][1:]),
             (modfile, 39, 'abuse', ['        self.argue(a, b, c)\n'], 0))
        # Test named tuple fields
        record = mod.st[0]
        self.assertIs(record.frame, mod.fr)
        self.assertEqual(record.lineno, 16)
        self.assertEqual(record.filename, mod.__file__)
        self.assertEqual(record.function, 'eggs')
        self.assertIn('inspect.stack()', record.code_context[0])
        self.assertEqual(record.index, 0)

    def test_trace(self):
        self.assertEqual(len(git.tr), 3)
        self.assertEqual(revise(*git.tr[0][1:]),
             (modfile, 43, 'argue', ['            spam(a, b, c)\n'], 0))
        self.assertEqual(revise(*git.tr[1][1:]),
             (modfile, 9, 'spam', ['    eggs(b + d, c + f)\n'], 0))
        self.assertEqual(revise(*git.tr[2][1:]),
             (modfile, 18, 'eggs', ['    q = y / 0\n'], 0))

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
        self.assertEqual(args, ['a', 'b', 'c', 'd', 'e', 'f'])
        self.assertEqual(varargs, 'g')
        self.assertEqual(varkw, 'h')
        self.assertEqual(inspect.formatargvalues(args, varargs, varkw, locals),
             '(a=7, b=8, c=9, d=3, e=4, f=5, *g=(), **h={})')

class GetSourceBase(unittest.TestCase):
    # Subclasses must override.
    fodderModule = None

    def setUp(self):
        with open(inspect.getsourcefile(self.fodderModule), encoding="utf-8") as fp:
            self.source = fp.read()

    def sourcerange(self, top, bottom):
        lines = self.source.split("\n")
        return "\n".join(lines[top-1:bottom]) + ("\n" if bottom else "")

    def assertSourceEqual(self, obj, top, bottom):
        self.assertEqual(inspect.getsource(obj),
                         self.sourcerange(top, bottom))

class SlotUser:
    'Docstrings for __slots__'
    __slots__ = {'power': 'measured in kilowatts',
                 'distance': 'measured in kilometers'}

class TestRetrievingSourceCode(GetSourceBase):
    fodderModule = mod

    def test_getclasses(self):
        classes = inspect.getmembers(mod, inspect.isclass)
        self.assertEqual(classes,
                         [('FesteringGob', mod.FesteringGob),
                          ('MalodorousPervert', mod.MalodorousPervert),
                          ('ParrotDroppings', mod.ParrotDroppings),
                          ('StupidGit', mod.StupidGit),
                          ('Tit', mod.MalodorousPervert),
                          ('WhichComments', mod.WhichComments),
                         ])
        tree = inspect.getclasstree([cls[1] for cls in classes])
        self.assertEqual(tree,
                         [(object, ()),
                          [(mod.ParrotDroppings, (object,)),
                           [(mod.FesteringGob, (mod.MalodorousPervert,
                                                   mod.ParrotDroppings))
                            ],
                           (mod.StupidGit, (object,)),
                           [(mod.MalodorousPervert, (mod.StupidGit,)),
                            [(mod.FesteringGob, (mod.MalodorousPervert,
                                                    mod.ParrotDroppings))
                             ]
                            ],
                            (mod.WhichComments, (object,),)
                           ]
                          ])
        tree = inspect.getclasstree([cls[1] for cls in classes], True)
        self.assertEqual(tree,
                         [(object, ()),
                          [(mod.ParrotDroppings, (object,)),
                           (mod.StupidGit, (object,)),
                           [(mod.MalodorousPervert, (mod.StupidGit,)),
                            [(mod.FesteringGob, (mod.MalodorousPervert,
                                                    mod.ParrotDroppings))
                             ]
                            ],
                            (mod.WhichComments, (object,),)
                           ]
                          ])

    def test_getfunctions(self):
        functions = inspect.getmembers(mod, inspect.isfunction)
        self.assertEqual(functions, [('eggs', mod.eggs),
                                     ('lobbest', mod.lobbest),
                                     ('spam', mod.spam)])

    @unittest.skipIf(sys.flags.optimize >= 2,
                     "Docstrings are omitted with -O2 and above")
    def test_getdoc(self):
        self.assertEqual(inspect.getdoc(mod), 'A module docstring.')
        self.assertEqual(inspect.getdoc(mod.StupidGit),
                         'A longer,\n\nindented\n\ndocstring.')
        self.assertEqual(inspect.getdoc(git.abuse),
                         'Another\n\ndocstring\n\ncontaining\n\ntabs')
        self.assertEqual(inspect.getdoc(SlotUser.power),
                         'measured in kilowatts')
        self.assertEqual(inspect.getdoc(SlotUser.distance),
                         'measured in kilometers')

    @unittest.skipIf(sys.flags.optimize >= 2,
                     "Docstrings are omitted with -O2 and above")
    def test_getdoc_inherited(self):
        self.assertEqual(inspect.getdoc(mod.FesteringGob),
                         'A longer,\n\nindented\n\ndocstring.')
        self.assertEqual(inspect.getdoc(mod.FesteringGob.abuse),
                         'Another\n\ndocstring\n\ncontaining\n\ntabs')
        self.assertEqual(inspect.getdoc(mod.FesteringGob().abuse),
                         'Another\n\ndocstring\n\ncontaining\n\ntabs')
        self.assertEqual(inspect.getdoc(mod.FesteringGob.contradiction),
                         'The automatic gainsaying.')

    @unittest.skipIf(MISSING_C_DOCSTRINGS, "test requires docstrings")
    def test_finddoc(self):
        finddoc = inspect._finddoc
        self.assertEqual(finddoc(int), int.__doc__)
        self.assertEqual(finddoc(int.to_bytes), int.to_bytes.__doc__)
        self.assertEqual(finddoc(int().to_bytes), int.to_bytes.__doc__)
        self.assertEqual(finddoc(int.from_bytes), int.from_bytes.__doc__)
        self.assertEqual(finddoc(int.real), int.real.__doc__)

    def test_cleandoc(self):
        self.assertEqual(inspect.cleandoc('An\n    indented\n    docstring.'),
                         'An\nindented\ndocstring.')

    def test_getcomments(self):
        self.assertEqual(inspect.getcomments(mod), '# line 1\n')
        self.assertEqual(inspect.getcomments(mod.StupidGit), '# line 20\n')
        self.assertEqual(inspect.getcomments(mod2.cls160), '# line 159\n')
        # If the object source file is not available, return None.
        co = compile('x=1', '_non_existing_filename.py', 'exec')
        self.assertIsNone(inspect.getcomments(co))
        # If the object has been defined in C, return None.
        self.assertIsNone(inspect.getcomments(list))

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
        self.assertEqual(inspect.getmodule(str), sys.modules["builtins"])
        # Check filename override
        self.assertEqual(inspect.getmodule(None, modfile), mod)

    def test_getframeinfo_get_first_line(self):
        frame_info = inspect.getframeinfo(self.fodderModule.fr, 50)
        self.assertEqual(frame_info.code_context[0], "# line 1\n")
        self.assertEqual(frame_info.code_context[1], "'A module docstring.'\n")

    def test_getsource(self):
        self.assertSourceEqual(git.abuse, 29, 39)
        self.assertSourceEqual(mod.StupidGit, 21, 51)
        self.assertSourceEqual(mod.lobbest, 75, 76)

    def test_getsourcefile(self):
        self.assertEqual(normcase(inspect.getsourcefile(mod.spam)), modfile)
        self.assertEqual(normcase(inspect.getsourcefile(git.abuse)), modfile)
        fn = "_non_existing_filename_used_for_sourcefile_test.py"
        co = compile("x=1", fn, "exec")
        self.assertEqual(inspect.getsourcefile(co), None)
        linecache.cache[co.co_filename] = (1, None, "None", co.co_filename)
        try:
            self.assertEqual(normcase(inspect.getsourcefile(co)), fn)
        finally:
            del linecache.cache[co.co_filename]

    def test_getfile(self):
        self.assertEqual(inspect.getfile(mod.StupidGit), mod.__file__)

    def test_getfile_builtin_module(self):
        with self.assertRaises(TypeError) as e:
            inspect.getfile(sys)
        self.assertTrue(str(e.exception).startswith('<module'))

    def test_getfile_builtin_class(self):
        with self.assertRaises(TypeError) as e:
            inspect.getfile(int)
        self.assertTrue(str(e.exception).startswith('<class'))

    def test_getfile_builtin_function_or_method(self):
        with self.assertRaises(TypeError) as e_abs:
            inspect.getfile(abs)
        self.assertIn('expected, got', str(e_abs.exception))
        with self.assertRaises(TypeError) as e_append:
            inspect.getfile(list.append)
        self.assertIn('expected, got', str(e_append.exception))

    def test_getfile_class_without_module(self):
        class CM(type):
            @property
            def __module__(cls):
                raise AttributeError
        class C(metaclass=CM):
            pass
        with self.assertRaises(TypeError):
            inspect.getfile(C)

    def test_getfile_broken_repr(self):
        class ErrorRepr:
            def __repr__(self):
                raise Exception('xyz')
        er = ErrorRepr()
        with self.assertRaises(TypeError):
            inspect.getfile(er)

    def test_getmodule_recursion(self):
        from types import ModuleType
        name = '__inspect_dummy'
        m = sys.modules[name] = ModuleType(name)
        m.__file__ = "<string>" # hopefully not a real filename...
        m.__loader__ = "dummy"  # pretend the filename is understood by a loader
        exec("def x(): pass", m.__dict__)
        self.assertEqual(inspect.getsourcefile(m.x.__code__), '<string>')
        del sys.modules[name]
        inspect.getmodule(compile('a=10','','single'))

    def test_proceed_with_fake_filename(self):
        '''doctest monkeypatches linecache to enable inspection'''
        fn, source = '<test>', 'def x(): pass\n'
        getlines = linecache.getlines
        def monkey(filename, module_globals=None):
            if filename == fn:
                return source.splitlines(keepends=True)
            else:
                return getlines(filename, module_globals)
        linecache.getlines = monkey
        try:
            ns = {}
            exec(compile(source, fn, 'single'), ns)
            inspect.getsource(ns["x"])
        finally:
            linecache.getlines = getlines

    def test_getsource_on_code_object(self):
        self.assertSourceEqual(mod.eggs.__code__, 12, 18)

class TestGettingSourceOfToplevelFrames(GetSourceBase):
    fodderModule = mod

    def test_range_toplevel_frame(self):
        self.maxDiff = None
        self.assertSourceEqual(mod.currentframe, 1, None)

    def test_range_traceback_toplevel_frame(self):
        self.assertSourceEqual(mod.tb, 1, None)

class TestDecorators(GetSourceBase):
    fodderModule = mod2

    def test_wrapped_decorator(self):
        self.assertSourceEqual(mod2.wrapped, 14, 17)

    def test_replacing_decorator(self):
        self.assertSourceEqual(mod2.gone, 9, 10)

    def test_getsource_unwrap(self):
        self.assertSourceEqual(mod2.real, 130, 132)

    def test_decorator_with_lambda(self):
        self.assertSourceEqual(mod2.func114, 113, 115)

class TestOneliners(GetSourceBase):
    fodderModule = mod2
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

class TestBlockComments(GetSourceBase):
    fodderModule = mod

    def test_toplevel_class(self):
        self.assertSourceEqual(mod.WhichComments, 96, 114)

    def test_class_method(self):
        self.assertSourceEqual(mod.WhichComments.f, 99, 104)

    def test_class_async_method(self):
        self.assertSourceEqual(mod.WhichComments.asyncf, 109, 112)

class TestBuggyCases(GetSourceBase):
    fodderModule = mod2

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

    # This should not skip for CPython, but might on a repackaged python where
    # unicodedata is not an external module, or on pypy.
    @unittest.skipIf(not hasattr(unicodedata, '__file__') or
                                 unicodedata.__file__.endswith('.py'),
                     "unicodedata is not an external binary module")
    def test_findsource_binary(self):
        self.assertRaises(OSError, inspect.getsource, unicodedata)
        self.assertRaises(OSError, inspect.findsource, unicodedata)

    def test_findsource_code_in_linecache(self):
        lines = ["x=1"]
        co = compile(lines[0], "_dynamically_created_file", "exec")
        self.assertRaises(OSError, inspect.findsource, co)
        self.assertRaises(OSError, inspect.getsource, co)
        linecache.cache[co.co_filename] = (1, None, lines, co.co_filename)
        try:
            self.assertEqual(inspect.findsource(co), (lines,0))
            self.assertEqual(inspect.getsource(co), lines[0])
        finally:
            del linecache.cache[co.co_filename]

    def test_findsource_without_filename(self):
        for fname in ['', '<string>']:
            co = compile('x=1', fname, "exec")
            self.assertRaises(IOError, inspect.findsource, co)
            self.assertRaises(IOError, inspect.getsource, co)

    def test_findsource_with_out_of_bounds_lineno(self):
        mod_len = len(inspect.getsource(mod))
        src = '\n' * 2* mod_len + "def f(): pass"
        co = compile(src, mod.__file__, "exec")
        g, l = {}, {}
        eval(co, g, l)
        func = l['f']
        self.assertEqual(func.__code__.co_firstlineno, 1+2*mod_len)
        with self.assertRaisesRegex(IOError, "lineno is out of bounds"):
            inspect.findsource(func)

    def test_getsource_on_method(self):
        self.assertSourceEqual(mod2.ClassWithMethod.method, 118, 119)

    def test_nested_func(self):
        self.assertSourceEqual(mod2.cls135.func136, 136, 139)

    def test_class_definition_in_multiline_string_definition(self):
        self.assertSourceEqual(mod2.cls149, 149, 152)

    def test_class_definition_in_multiline_comment(self):
        self.assertSourceEqual(mod2.cls160, 160, 163)

    def test_nested_class_definition_indented_string(self):
        self.assertSourceEqual(mod2.cls173.cls175, 175, 176)

    def test_nested_class_definition(self):
        self.assertSourceEqual(mod2.cls183, 183, 188)
        self.assertSourceEqual(mod2.cls183.cls185, 185, 188)

    def test_class_decorator(self):
        self.assertSourceEqual(mod2.cls196, 194, 201)
        self.assertSourceEqual(mod2.cls196.cls200, 198, 201)

    def test_class_inside_conditional(self):
        self.assertSourceEqual(mod2.cls238, 238, 240)
        self.assertSourceEqual(mod2.cls238.cls239, 239, 240)

    def test_multiple_children_classes(self):
        self.assertSourceEqual(mod2.cls203, 203, 209)
        self.assertSourceEqual(mod2.cls203.cls204, 204, 206)
        self.assertSourceEqual(mod2.cls203.cls204.cls205, 205, 206)
        self.assertSourceEqual(mod2.cls203.cls207, 207, 209)
        self.assertSourceEqual(mod2.cls203.cls207.cls205, 208, 209)

    def test_nested_class_definition_inside_function(self):
        self.assertSourceEqual(mod2.func212(), 213, 214)
        self.assertSourceEqual(mod2.cls213, 218, 222)
        self.assertSourceEqual(mod2.cls213().func219(), 220, 221)

    def test_nested_class_definition_inside_async_function(self):
        import asyncio
        self.addCleanup(asyncio.set_event_loop_policy, None)
        self.assertSourceEqual(asyncio.run(mod2.func225()), 226, 227)
        self.assertSourceEqual(mod2.cls226, 231, 235)
        self.assertSourceEqual(asyncio.run(mod2.cls226().func232()), 233, 234)

class TestNoEOL(GetSourceBase):
    def setUp(self):
        self.tempdir = TESTFN + '_dir'
        os.mkdir(self.tempdir)
        with open(os.path.join(self.tempdir, 'inspect_fodder3%spy' % os.extsep),
                  'w', encoding='utf-8') as f:
            f.write("class X:\n    pass # No EOL")
        with DirsOnSysPath(self.tempdir):
            import inspect_fodder3 as mod3
        self.fodderModule = mod3
        super().setUp()

    def tearDown(self):
        shutil.rmtree(self.tempdir)

    def test_class(self):
        self.assertSourceEqual(self.fodderModule.X, 1, 2)


class _BrokenDataDescriptor(object):
    """
    A broken data descriptor. See bug #1785.
    """
    def __get__(*args):
        raise AttributeError("broken data descriptor")

    def __set__(*args):
        raise RuntimeError

    def __getattr__(*args):
        raise AttributeError("broken data descriptor")


class _BrokenMethodDescriptor(object):
    """
    A broken method descriptor. See bug #1785.
    """
    def __get__(*args):
        raise AttributeError("broken method descriptor")

    def __getattr__(*args):
        raise AttributeError("broken method descriptor")


# Helper for testing classify_class_attrs.
def attrs_wo_objs(cls):
    return [t[:3] for t in inspect.classify_class_attrs(cls)]


class TestClassesAndFunctions(unittest.TestCase):
    def test_newstyle_mro(self):
        # The same w/ new-class MRO.
        class A(object):    pass
        class B(A): pass
        class C(A): pass
        class D(B, C): pass

        expected = (D, B, C, A, object)
        got = inspect.getmro(D)
        self.assertEqual(expected, got)

    def assertArgSpecEquals(self, routine, args_e, varargs_e=None,
                            varkw_e=None, defaults_e=None, formatted=None):
        with self.assertWarns(DeprecationWarning):
            args, varargs, varkw, defaults = inspect.getargspec(routine)
        self.assertEqual(args, args_e)
        self.assertEqual(varargs, varargs_e)
        self.assertEqual(varkw, varkw_e)
        self.assertEqual(defaults, defaults_e)
        if formatted is not None:
            with self.assertWarns(DeprecationWarning):
                self.assertEqual(inspect.formatargspec(args, varargs, varkw, defaults),
                                 formatted)

    def assertFullArgSpecEquals(self, routine, args_e, varargs_e=None,
                                    varkw_e=None, defaults_e=None,
                                    posonlyargs_e=[], kwonlyargs_e=[],
                                    kwonlydefaults_e=None,
                                    ann_e={}, formatted=None):
        args, varargs, varkw, defaults, kwonlyargs, kwonlydefaults, ann = \
            inspect.getfullargspec(routine)
        self.assertEqual(args, args_e)
        self.assertEqual(varargs, varargs_e)
        self.assertEqual(varkw, varkw_e)
        self.assertEqual(defaults, defaults_e)
        self.assertEqual(kwonlyargs, kwonlyargs_e)
        self.assertEqual(kwonlydefaults, kwonlydefaults_e)
        self.assertEqual(ann, ann_e)
        if formatted is not None:
            with self.assertWarns(DeprecationWarning):
                self.assertEqual(inspect.formatargspec(args, varargs, varkw, defaults,
                                                       kwonlyargs, kwonlydefaults, ann),
                                 formatted)

    def test_getargspec(self):
        self.assertArgSpecEquals(mod.eggs, ['x', 'y'], formatted='(x, y)')

        self.assertArgSpecEquals(mod.spam,
                                 ['a', 'b', 'c', 'd', 'e', 'f'],
                                 'g', 'h', (3, 4, 5),
                                 '(a, b, c, d=3, e=4, f=5, *g, **h)')

        self.assertRaises(ValueError, self.assertArgSpecEquals,
                          mod2.keyworded, [])

        self.assertRaises(ValueError, self.assertArgSpecEquals,
                          mod2.annotated, [])
        self.assertRaises(ValueError, self.assertArgSpecEquals,
                          mod2.keyword_only_arg, [])


    def test_getfullargspec(self):
        self.assertFullArgSpecEquals(mod2.keyworded, [], varargs_e='arg1',
                                     kwonlyargs_e=['arg2'],
                                     kwonlydefaults_e={'arg2':1},
                                     formatted='(*arg1, arg2=1)')

        self.assertFullArgSpecEquals(mod2.annotated, ['arg1'],
                                     ann_e={'arg1' : list},
                                     formatted='(arg1: list)')
        self.assertFullArgSpecEquals(mod2.keyword_only_arg, [],
                                     kwonlyargs_e=['arg'],
                                     formatted='(*, arg)')

        self.assertFullArgSpecEquals(mod2.all_markers, ['a', 'b', 'c', 'd'],
                                     kwonlyargs_e=['e', 'f'],
                                     formatted='(a, b, c, d, *, e, f)')

        self.assertFullArgSpecEquals(mod2.all_markers_with_args_and_kwargs,
                                     ['a', 'b', 'c', 'd'],
                                     varargs_e='args',
                                     varkw_e='kwargs',
                                     kwonlyargs_e=['e', 'f'],
                                     formatted='(a, b, c, d, *args, e, f, **kwargs)')

        self.assertFullArgSpecEquals(mod2.all_markers_with_defaults, ['a', 'b', 'c', 'd'],
                                     defaults_e=(1,2,3),
                                     kwonlyargs_e=['e', 'f'],
                                     kwonlydefaults_e={'e': 4, 'f': 5},
                                     formatted='(a, b=1, c=2, d=3, *, e=4, f=5)')

    def test_argspec_api_ignores_wrapped(self):
        # Issue 20684: low level introspection API must ignore __wrapped__
        @functools.wraps(mod.spam)
        def ham(x, y):
            pass
        # Basic check
        self.assertArgSpecEquals(ham, ['x', 'y'], formatted='(x, y)')
        self.assertFullArgSpecEquals(ham, ['x', 'y'], formatted='(x, y)')
        self.assertFullArgSpecEquals(functools.partial(ham),
                                     ['x', 'y'], formatted='(x, y)')
        # Other variants
        def check_method(f):
            self.assertArgSpecEquals(f, ['self', 'x', 'y'],
                                        formatted='(self, x, y)')
        class C:
            @functools.wraps(mod.spam)
            def ham(self, x, y):
                pass
            pham = functools.partialmethod(ham)
            @functools.wraps(mod.spam)
            def __call__(self, x, y):
                pass
        check_method(C())
        check_method(C.ham)
        check_method(C().ham)
        check_method(C.pham)
        check_method(C().pham)

        class C_new:
            @functools.wraps(mod.spam)
            def __new__(self, x, y):
                pass
        check_method(C_new)

        class C_init:
            @functools.wraps(mod.spam)
            def __init__(self, x, y):
                pass
        check_method(C_init)

    def test_getfullargspec_signature_attr(self):
        def test():
            pass
        spam_param = inspect.Parameter('spam', inspect.Parameter.POSITIONAL_ONLY)
        test.__signature__ = inspect.Signature(parameters=(spam_param,))

        self.assertFullArgSpecEquals(test, ['spam'], formatted='(spam)')

    def test_getfullargspec_signature_annos(self):
        def test(a:'spam') -> 'ham': pass
        spec = inspect.getfullargspec(test)
        self.assertEqual(test.__annotations__, spec.annotations)

        def test(): pass
        spec = inspect.getfullargspec(test)
        self.assertEqual(test.__annotations__, spec.annotations)

    @unittest.skipIf(MISSING_C_DOCSTRINGS,
                     "Signature information for builtins requires docstrings")
    def test_getfullargspec_builtin_methods(self):
        self.assertFullArgSpecEquals(_pickle.Pickler.dump, ['self', 'obj'],
                                     formatted='(self, obj)')

        self.assertFullArgSpecEquals(_pickle.Pickler(io.BytesIO()).dump, ['self', 'obj'],
                                     formatted='(self, obj)')

        self.assertFullArgSpecEquals(
             os.stat,
             args_e=['path'],
             kwonlyargs_e=['dir_fd', 'follow_symlinks'],
             kwonlydefaults_e={'dir_fd': None, 'follow_symlinks': True},
             formatted='(path, *, dir_fd=None, follow_symlinks=True)')

    @cpython_only
    @unittest.skipIf(MISSING_C_DOCSTRINGS,
                     "Signature information for builtins requires docstrings")
    def test_getfullargspec_builtin_func(self):
        import _testcapi
        builtin = _testcapi.docstring_with_signature_with_defaults
        spec = inspect.getfullargspec(builtin)
        self.assertEqual(spec.defaults[0], 'avocado')

    @cpython_only
    @unittest.skipIf(MISSING_C_DOCSTRINGS,
                     "Signature information for builtins requires docstrings")
    def test_getfullargspec_builtin_func_no_signature(self):
        import _testcapi
        builtin = _testcapi.docstring_no_signature
        with self.assertRaises(TypeError):
            inspect.getfullargspec(builtin)

    def test_getfullargspec_definition_order_preserved_on_kwonly(self):
        for fn in signatures_with_lexicographic_keyword_only_parameters():
            signature = inspect.getfullargspec(fn)
            l = list(signature.kwonlyargs)
            sorted_l = sorted(l)
            self.assertTrue(l)
            self.assertEqual(l, sorted_l)
        signature = inspect.getfullargspec(unsorted_keyword_only_parameters_fn)
        l = list(signature.kwonlyargs)
        self.assertEqual(l, unsorted_keyword_only_parameters)

    def test_getargspec_method(self):
        class A(object):
            def m(self):
                pass
        self.assertArgSpecEquals(A.m, ['self'])

    def test_classify_newstyle(self):
        class A(object):

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

        self.assertIn(('__new__', 'static method', object), attrs,
                      'missing __new__')
        self.assertIn(('__init__', 'method', object), attrs, 'missing __init__')

        self.assertIn(('s', 'static method', A), attrs, 'missing static method')
        self.assertIn(('c', 'class method', A), attrs, 'missing class method')
        self.assertIn(('p', 'property', A), attrs, 'missing property')
        self.assertIn(('m', 'method', A), attrs,
                      'missing plain method: %r' % attrs)
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
        self.assertIn(('c', 'method', C), attrs, 'missing plain method')
        self.assertIn(('p', 'property', A), attrs, 'missing property')
        self.assertIn(('m', 'method', B), attrs, 'missing plain method')
        self.assertIn(('m1', 'method', D), attrs, 'missing plain method')
        self.assertIn(('datablob', 'data', A), attrs, 'missing data')
        self.assertIn(('md', 'method', A), attrs, 'missing method descriptor')
        self.assertIn(('dd', 'data', A), attrs, 'missing data descriptor')

    def test_classify_builtin_types(self):
        # Simple sanity check that all built-in types can have their
        # attributes classified.
        for name in dir(__builtins__):
            builtin = getattr(__builtins__, name)
            if isinstance(builtin, type):
                inspect.classify_class_attrs(builtin)

        attrs = attrs_wo_objs(bool)
        self.assertIn(('__new__', 'static method', bool), attrs,
                      'missing __new__')
        self.assertIn(('from_bytes', 'class method', int), attrs,
                      'missing class method')
        self.assertIn(('to_bytes', 'method', int), attrs,
                      'missing plain method')
        self.assertIn(('__add__', 'method', int), attrs,
                      'missing plain method')
        self.assertIn(('__and__', 'method', bool), attrs,
                      'missing plain method')

    def test_classify_DynamicClassAttribute(self):
        class Meta(type):
            def __getattr__(self, name):
                if name == 'ham':
                    return 'spam'
                return super().__getattr__(name)
        class VA(metaclass=Meta):
            @types.DynamicClassAttribute
            def ham(self):
                return 'eggs'
        should_find_dca = inspect.Attribute('ham', 'data', VA, VA.__dict__['ham'])
        self.assertIn(should_find_dca, inspect.classify_class_attrs(VA))
        should_find_ga = inspect.Attribute('ham', 'data', Meta, 'spam')
        self.assertIn(should_find_ga, inspect.classify_class_attrs(VA))

    def test_classify_overrides_bool(self):
        class NoBool(object):
            def __eq__(self, other):
                return NoBool()

            def __bool__(self):
                raise NotImplementedError(
                    "This object does not specify a boolean value")

        class HasNB(object):
            dd = NoBool()

        should_find_attr = inspect.Attribute('dd', 'data', HasNB, HasNB.dd)
        self.assertIn(should_find_attr, inspect.classify_class_attrs(HasNB))

    def test_classify_metaclass_class_attribute(self):
        class Meta(type):
            fish = 'slap'
            def __dir__(self):
                return ['__class__', '__module__', '__name__', 'fish']
        class Class(metaclass=Meta):
            pass
        should_find = inspect.Attribute('fish', 'data', Meta, 'slap')
        self.assertIn(should_find, inspect.classify_class_attrs(Class))

    def test_classify_VirtualAttribute(self):
        class Meta(type):
            def __dir__(cls):
                return ['__class__', '__module__', '__name__', 'BOOM']
            def __getattr__(self, name):
                if name =='BOOM':
                    return 42
                return super().__getattr(name)
        class Class(metaclass=Meta):
            pass
        should_find = inspect.Attribute('BOOM', 'data', Meta, 42)
        self.assertIn(should_find, inspect.classify_class_attrs(Class))

    def test_classify_VirtualAttribute_multi_classes(self):
        class Meta1(type):
            def __dir__(cls):
                return ['__class__', '__module__', '__name__', 'one']
            def __getattr__(self, name):
                if name =='one':
                    return 1
                return super().__getattr__(name)
        class Meta2(type):
            def __dir__(cls):
                return ['__class__', '__module__', '__name__', 'two']
            def __getattr__(self, name):
                if name =='two':
                    return 2
                return super().__getattr__(name)
        class Meta3(Meta1, Meta2):
            def __dir__(cls):
                return list(sorted(set(['__class__', '__module__', '__name__', 'three'] +
                    Meta1.__dir__(cls) + Meta2.__dir__(cls))))
            def __getattr__(self, name):
                if name =='three':
                    return 3
                return super().__getattr__(name)
        class Class1(metaclass=Meta1):
            pass
        class Class2(Class1, metaclass=Meta3):
            pass

        should_find1 = inspect.Attribute('one', 'data', Meta1, 1)
        should_find2 = inspect.Attribute('two', 'data', Meta2, 2)
        should_find3 = inspect.Attribute('three', 'data', Meta3, 3)
        cca = inspect.classify_class_attrs(Class2)
        for sf in (should_find1, should_find2, should_find3):
            self.assertIn(sf, cca)

    def test_classify_class_attrs_with_buggy_dir(self):
        class M(type):
            def __dir__(cls):
                return ['__class__', '__name__', 'missing']
        class C(metaclass=M):
            pass
        attrs = [a[0] for a in inspect.classify_class_attrs(C)]
        self.assertNotIn('missing', attrs)

    def test_getmembers_descriptors(self):
        class A(object):
            dd = _BrokenDataDescriptor()
            md = _BrokenMethodDescriptor()

        def pred_wrapper(pred):
            # A quick'n'dirty way to discard standard attributes of new-style
            # classes.
            class Empty(object):
                pass
            def wrapped(x):
                if '__name__' in dir(x) and hasattr(Empty, x.__name__):
                    return False
                return pred(x)
            return wrapped

        ismethoddescriptor = pred_wrapper(inspect.ismethoddescriptor)
        isdatadescriptor = pred_wrapper(inspect.isdatadescriptor)

        self.assertEqual(inspect.getmembers(A, ismethoddescriptor),
            [('md', A.__dict__['md'])])
        self.assertEqual(inspect.getmembers(A, isdatadescriptor),
            [('dd', A.__dict__['dd'])])

        class B(A):
            pass

        self.assertEqual(inspect.getmembers(B, ismethoddescriptor),
            [('md', A.__dict__['md'])])
        self.assertEqual(inspect.getmembers(B, isdatadescriptor),
            [('dd', A.__dict__['dd'])])

    def test_getmembers_method(self):
        class B:
            def f(self):
                pass

        self.assertIn(('f', B.f), inspect.getmembers(B))
        self.assertNotIn(('f', B.f), inspect.getmembers(B, inspect.ismethod))
        b = B()
        self.assertIn(('f', b.f), inspect.getmembers(b))
        self.assertIn(('f', b.f), inspect.getmembers(b, inspect.ismethod))

    def test_getmembers_VirtualAttribute(self):
        class M(type):
            def __getattr__(cls, name):
                if name == 'eggs':
                    return 'scrambled'
                return super().__getattr__(name)
        class A(metaclass=M):
            @types.DynamicClassAttribute
            def eggs(self):
                return 'spam'
        self.assertIn(('eggs', 'scrambled'), inspect.getmembers(A))
        self.assertIn(('eggs', 'spam'), inspect.getmembers(A()))

    def test_getmembers_with_buggy_dir(self):
        class M(type):
            def __dir__(cls):
                return ['__class__', '__name__', 'missing']
        class C(metaclass=M):
            pass
        attrs = [a[0] for a in inspect.getmembers(C)]
        self.assertNotIn('missing', attrs)

    def test_get_annotations_with_stock_annotations(self):
        def foo(a:int, b:str): pass
        self.assertEqual(inspect.get_annotations(foo), {'a': int, 'b': str})

        foo.__annotations__ = {'a': 'foo', 'b':'str'}
        self.assertEqual(inspect.get_annotations(foo), {'a': 'foo', 'b': 'str'})

        self.assertEqual(inspect.get_annotations(foo, eval_str=True, locals=locals()), {'a': foo, 'b': str})
        self.assertEqual(inspect.get_annotations(foo, eval_str=True, globals=locals()), {'a': foo, 'b': str})

        isa = inspect_stock_annotations
        self.assertEqual(inspect.get_annotations(isa), {'a': int, 'b': str})
        self.assertEqual(inspect.get_annotations(isa.MyClass), {'a': int, 'b': str})
        self.assertEqual(inspect.get_annotations(isa.function), {'a': int, 'b': str, 'return': isa.MyClass})
        self.assertEqual(inspect.get_annotations(isa.function2), {'a': int, 'b': 'str', 'c': isa.MyClass, 'return': isa.MyClass})
        self.assertEqual(inspect.get_annotations(isa.function3), {'a': 'int', 'b': 'str', 'c': 'MyClass'})
        self.assertEqual(inspect.get_annotations(inspect), {}) # inspect module has no annotations
        self.assertEqual(inspect.get_annotations(isa.UnannotatedClass), {})
        self.assertEqual(inspect.get_annotations(isa.unannotated_function), {})

        self.assertEqual(inspect.get_annotations(isa, eval_str=True), {'a': int, 'b': str})
        self.assertEqual(inspect.get_annotations(isa.MyClass, eval_str=True), {'a': int, 'b': str})
        self.assertEqual(inspect.get_annotations(isa.function, eval_str=True), {'a': int, 'b': str, 'return': isa.MyClass})
        self.assertEqual(inspect.get_annotations(isa.function2, eval_str=True), {'a': int, 'b': str, 'c': isa.MyClass, 'return': isa.MyClass})
        self.assertEqual(inspect.get_annotations(isa.function3, eval_str=True), {'a': int, 'b': str, 'c': isa.MyClass})
        self.assertEqual(inspect.get_annotations(inspect, eval_str=True), {})
        self.assertEqual(inspect.get_annotations(isa.UnannotatedClass, eval_str=True), {})
        self.assertEqual(inspect.get_annotations(isa.unannotated_function, eval_str=True), {})

        self.assertEqual(inspect.get_annotations(isa, eval_str=False), {'a': int, 'b': str})
        self.assertEqual(inspect.get_annotations(isa.MyClass, eval_str=False), {'a': int, 'b': str})
        self.assertEqual(inspect.get_annotations(isa.function, eval_str=False), {'a': int, 'b': str, 'return': isa.MyClass})
        self.assertEqual(inspect.get_annotations(isa.function2, eval_str=False), {'a': int, 'b': 'str', 'c': isa.MyClass, 'return': isa.MyClass})
        self.assertEqual(inspect.get_annotations(isa.function3, eval_str=False), {'a': 'int', 'b': 'str', 'c': 'MyClass'})
        self.assertEqual(inspect.get_annotations(inspect, eval_str=False), {})
        self.assertEqual(inspect.get_annotations(isa.UnannotatedClass, eval_str=False), {})
        self.assertEqual(inspect.get_annotations(isa.unannotated_function, eval_str=False), {})

        def times_three(fn):
            @functools.wraps(fn)
            def wrapper(a, b):
                return fn(a*3, b*3)
            return wrapper

        wrapped = times_three(isa.function)
        self.assertEqual(wrapped(1, 'x'), isa.MyClass(3, 'xxx'))
        self.assertIsNot(wrapped.__globals__, isa.function.__globals__)
        self.assertEqual(inspect.get_annotations(wrapped), {'a': int, 'b': str, 'return': isa.MyClass})
        self.assertEqual(inspect.get_annotations(wrapped, eval_str=True), {'a': int, 'b': str, 'return': isa.MyClass})
        self.assertEqual(inspect.get_annotations(wrapped, eval_str=False), {'a': int, 'b': str, 'return': isa.MyClass})

    def test_get_annotations_with_stringized_annotations(self):
        isa = inspect_stringized_annotations
        self.assertEqual(inspect.get_annotations(isa), {'a': 'int', 'b': 'str'})
        self.assertEqual(inspect.get_annotations(isa.MyClass), {'a': 'int', 'b': 'str'})
        self.assertEqual(inspect.get_annotations(isa.function), {'a': 'int', 'b': 'str', 'return': 'MyClass'})
        self.assertEqual(inspect.get_annotations(isa.function2), {'a': 'int', 'b': "'str'", 'c': 'MyClass', 'return': 'MyClass'})
        self.assertEqual(inspect.get_annotations(isa.function3), {'a': "'int'", 'b': "'str'", 'c': "'MyClass'"})
        self.assertEqual(inspect.get_annotations(isa.UnannotatedClass), {})
        self.assertEqual(inspect.get_annotations(isa.unannotated_function), {})

        self.assertEqual(inspect.get_annotations(isa, eval_str=True), {'a': int, 'b': str})
        self.assertEqual(inspect.get_annotations(isa.MyClass, eval_str=True), {'a': int, 'b': str})
        self.assertEqual(inspect.get_annotations(isa.function, eval_str=True), {'a': int, 'b': str, 'return': isa.MyClass})
        self.assertEqual(inspect.get_annotations(isa.function2, eval_str=True), {'a': int, 'b': 'str', 'c': isa.MyClass, 'return': isa.MyClass})
        self.assertEqual(inspect.get_annotations(isa.function3, eval_str=True), {'a': 'int', 'b': 'str', 'c': 'MyClass'})
        self.assertEqual(inspect.get_annotations(isa.UnannotatedClass, eval_str=True), {})
        self.assertEqual(inspect.get_annotations(isa.unannotated_function, eval_str=True), {})

        self.assertEqual(inspect.get_annotations(isa, eval_str=False), {'a': 'int', 'b': 'str'})
        self.assertEqual(inspect.get_annotations(isa.MyClass, eval_str=False), {'a': 'int', 'b': 'str'})
        self.assertEqual(inspect.get_annotations(isa.function, eval_str=False), {'a': 'int', 'b': 'str', 'return': 'MyClass'})
        self.assertEqual(inspect.get_annotations(isa.function2, eval_str=False), {'a': 'int', 'b': "'str'", 'c': 'MyClass', 'return': 'MyClass'})
        self.assertEqual(inspect.get_annotations(isa.function3, eval_str=False), {'a': "'int'", 'b': "'str'", 'c': "'MyClass'"})
        self.assertEqual(inspect.get_annotations(isa.UnannotatedClass, eval_str=False), {})
        self.assertEqual(inspect.get_annotations(isa.unannotated_function, eval_str=False), {})

        isa2 = inspect_stringized_annotations_2
        self.assertEqual(inspect.get_annotations(isa2), {})
        self.assertEqual(inspect.get_annotations(isa2, eval_str=True), {})
        self.assertEqual(inspect.get_annotations(isa2, eval_str=False), {})

        def times_three(fn):
            @functools.wraps(fn)
            def wrapper(a, b):
                return fn(a*3, b*3)
            return wrapper

        wrapped = times_three(isa.function)
        self.assertEqual(wrapped(1, 'x'), isa.MyClass(3, 'xxx'))
        self.assertIsNot(wrapped.__globals__, isa.function.__globals__)
        self.assertEqual(inspect.get_annotations(wrapped), {'a': 'int', 'b': 'str', 'return': 'MyClass'})
        self.assertEqual(inspect.get_annotations(wrapped, eval_str=True), {'a': int, 'b': str, 'return': isa.MyClass})
        self.assertEqual(inspect.get_annotations(wrapped, eval_str=False), {'a': 'int', 'b': 'str', 'return': 'MyClass'})

        # test that local namespace lookups work
        self.assertEqual(inspect.get_annotations(isa.MyClassWithLocalAnnotations), {'x': 'mytype'})
        self.assertEqual(inspect.get_annotations(isa.MyClassWithLocalAnnotations, eval_str=True), {'x': int})


class TestIsDataDescriptor(unittest.TestCase):

    def test_custom_descriptors(self):
        class NonDataDescriptor:
            def __get__(self, value, type=None): pass
        class DataDescriptor0:
            def __set__(self, name, value): pass
        class DataDescriptor1:
            def __delete__(self, name): pass
        class DataDescriptor2:
            __set__ = None
        self.assertFalse(inspect.isdatadescriptor(NonDataDescriptor()),
                         'class with only __get__ not a data descriptor')
        self.assertTrue(inspect.isdatadescriptor(DataDescriptor0()),
                        'class with __set__ is a data descriptor')
        self.assertTrue(inspect.isdatadescriptor(DataDescriptor1()),
                        'class with __delete__ is a data descriptor')
        self.assertTrue(inspect.isdatadescriptor(DataDescriptor2()),
                        'class with __set__ = None is a data descriptor')

    def test_slot(self):
        class Slotted:
            __slots__ = 'foo',
        self.assertTrue(inspect.isdatadescriptor(Slotted.foo),
                        'a slot is a data descriptor')

    def test_property(self):
        class Propertied:
            @property
            def a_property(self):
                pass
        self.assertTrue(inspect.isdatadescriptor(Propertied.a_property),
                        'a property is a data descriptor')

    def test_functions(self):
        class Test(object):
            def instance_method(self): pass
            @classmethod
            def class_method(cls): pass
            @staticmethod
            def static_method(): pass
        def function():
            pass
        a_lambda = lambda: None
        self.assertFalse(inspect.isdatadescriptor(Test().instance_method),
                         'a instance method is not a data descriptor')
        self.assertFalse(inspect.isdatadescriptor(Test().class_method),
                         'a class method is not a data descriptor')
        self.assertFalse(inspect.isdatadescriptor(Test().static_method),
                         'a static method is not a data descriptor')
        self.assertFalse(inspect.isdatadescriptor(function),
                         'a function is not a data descriptor')
        self.assertFalse(inspect.isdatadescriptor(a_lambda),
                         'a lambda is not a data descriptor')


_global_ref = object()
class TestGetClosureVars(unittest.TestCase):

    def test_name_resolution(self):
        # Basic test of the 4 different resolution mechanisms
        def f(nonlocal_ref):
            def g(local_ref):
                print(local_ref, nonlocal_ref, _global_ref, unbound_ref)
            return g
        _arg = object()
        nonlocal_vars = {"nonlocal_ref": _arg}
        global_vars = {"_global_ref": _global_ref}
        builtin_vars = {"print": print}
        unbound_names = {"unbound_ref"}
        expected = inspect.ClosureVars(nonlocal_vars, global_vars,
                                       builtin_vars, unbound_names)
        self.assertEqual(inspect.getclosurevars(f(_arg)), expected)

    def test_generator_closure(self):
        def f(nonlocal_ref):
            def g(local_ref):
                print(local_ref, nonlocal_ref, _global_ref, unbound_ref)
                yield
            return g
        _arg = object()
        nonlocal_vars = {"nonlocal_ref": _arg}
        global_vars = {"_global_ref": _global_ref}
        builtin_vars = {"print": print}
        unbound_names = {"unbound_ref"}
        expected = inspect.ClosureVars(nonlocal_vars, global_vars,
                                       builtin_vars, unbound_names)
        self.assertEqual(inspect.getclosurevars(f(_arg)), expected)

    def test_method_closure(self):
        class C:
            def f(self, nonlocal_ref):
                def g(local_ref):
                    print(local_ref, nonlocal_ref, _global_ref, unbound_ref)
                return g
        _arg = object()
        nonlocal_vars = {"nonlocal_ref": _arg}
        global_vars = {"_global_ref": _global_ref}
        builtin_vars = {"print": print}
        unbound_names = {"unbound_ref"}
        expected = inspect.ClosureVars(nonlocal_vars, global_vars,
                                       builtin_vars, unbound_names)
        self.assertEqual(inspect.getclosurevars(C().f(_arg)), expected)

    def test_nonlocal_vars(self):
        # More complex tests of nonlocal resolution
        def _nonlocal_vars(f):
            return inspect.getclosurevars(f).nonlocals

        def make_adder(x):
            def add(y):
                return x + y
            return add

        def curry(func, arg1):
            return lambda arg2: func(arg1, arg2)

        def less_than(a, b):
            return a < b

        # The infamous Y combinator.
        def Y(le):
            def g(f):
                return le(lambda x: f(f)(x))
            Y.g_ref = g
            return g(g)

        def check_y_combinator(func):
            self.assertEqual(_nonlocal_vars(func), {'f': Y.g_ref})

        inc = make_adder(1)
        add_two = make_adder(2)
        greater_than_five = curry(less_than, 5)

        self.assertEqual(_nonlocal_vars(inc), {'x': 1})
        self.assertEqual(_nonlocal_vars(add_two), {'x': 2})
        self.assertEqual(_nonlocal_vars(greater_than_five),
                         {'arg1': 5, 'func': less_than})
        self.assertEqual(_nonlocal_vars((lambda x: lambda y: x + y)(3)),
                         {'x': 3})
        Y(check_y_combinator)

    def test_getclosurevars_empty(self):
        def foo(): pass
        _empty = inspect.ClosureVars({}, {}, {}, set())
        self.assertEqual(inspect.getclosurevars(lambda: True), _empty)
        self.assertEqual(inspect.getclosurevars(foo), _empty)

    def test_getclosurevars_error(self):
        class T: pass
        self.assertRaises(TypeError, inspect.getclosurevars, 1)
        self.assertRaises(TypeError, inspect.getclosurevars, list)
        self.assertRaises(TypeError, inspect.getclosurevars, {})

    def _private_globals(self):
        code = """def f(): print(path)"""
        ns = {}
        exec(code, ns)
        return ns["f"], ns

    def test_builtins_fallback(self):
        f, ns = self._private_globals()
        ns.pop("__builtins__", None)
        expected = inspect.ClosureVars({}, {}, {"print":print}, {"path"})
        self.assertEqual(inspect.getclosurevars(f), expected)

    def test_builtins_as_dict(self):
        f, ns = self._private_globals()
        ns["__builtins__"] = {"path":1}
        expected = inspect.ClosureVars({}, {}, {"path":1}, {"print"})
        self.assertEqual(inspect.getclosurevars(f), expected)

    def test_builtins_as_module(self):
        f, ns = self._private_globals()
        ns["__builtins__"] = os
        expected = inspect.ClosureVars({}, {}, {"path":os.path}, {"print"})
        self.assertEqual(inspect.getclosurevars(f), expected)


class TestGetcallargsFunctions(unittest.TestCase):

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
        except Exception as e:
            ex1 = e
        else:
            self.fail('Exception not raised')
        try:
            eval('inspect.getcallargs(func, %s)' % call_param_string, None,
                 locs)
        except Exception as e:
            ex2 = e
        else:
            self.fail('Exception not raised')
        self.assertIs(type(ex1), type(ex2))
        self.assertEqual(str(ex1), str(ex2))
        del ex1, ex2

    def makeCallable(self, signature):
        """Create a function that returns its locals()"""
        code = "lambda %s: locals()"
        return eval(code % signature)

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
        self.assertEqualCallArgs(f, '*collections.UserList([2])')
        self.assertEqualCallArgs(f, '*collections.UserList([2, 3])')
        self.assertEqualCallArgs(f, '**collections.UserDict(a=2)')
        self.assertEqualCallArgs(f, '2, **collections.UserDict(b=3)')
        self.assertEqualCallArgs(f, 'b=2, **collections.UserDict(a=3)')

    def test_varargs(self):
        f = self.makeCallable('a, b=1, *c')
        self.assertEqualCallArgs(f, '2')
        self.assertEqualCallArgs(f, '2, 3')
        self.assertEqualCallArgs(f, '2, 3, 4')
        self.assertEqualCallArgs(f, '*(2,3,4)')
        self.assertEqualCallArgs(f, '2, *[3,4]')
        self.assertEqualCallArgs(f, '2, 3, *collections.UserList([4])')

    def test_varkw(self):
        f = self.makeCallable('a, b=1, **c')
        self.assertEqualCallArgs(f, 'a=2')
        self.assertEqualCallArgs(f, '2, b=3, c=4')
        self.assertEqualCallArgs(f, 'b=3, a=2, c=4')
        self.assertEqualCallArgs(f, 'c=4, **{"a":2, "b":3}')
        self.assertEqualCallArgs(f, '2, c=4, **{"b":3}')
        self.assertEqualCallArgs(f, 'b=2, **{"a":3, "c":4}')
        self.assertEqualCallArgs(f, '**collections.UserDict(a=2, b=3, c=4)')
        self.assertEqualCallArgs(f, '2, c=4, **collections.UserDict(b=3)')
        self.assertEqualCallArgs(f, 'b=2, **collections.UserDict(a=3, c=4)')

    def test_varkw_only(self):
        # issue11256:
        f = self.makeCallable('**c')
        self.assertEqualCallArgs(f, '')
        self.assertEqualCallArgs(f, 'a=1')
        self.assertEqualCallArgs(f, 'a=1, b=2')
        self.assertEqualCallArgs(f, 'c=3, **{"a": 1, "b": 2}')
        self.assertEqualCallArgs(f, '**collections.UserDict(a=1, b=2)')
        self.assertEqualCallArgs(f, 'c=3, **collections.UserDict(a=1, b=2)')

    def test_keyword_only(self):
        f = self.makeCallable('a=3, *, c, d=2')
        self.assertEqualCallArgs(f, 'c=3')
        self.assertEqualCallArgs(f, 'c=3, a=3')
        self.assertEqualCallArgs(f, 'a=2, c=4')
        self.assertEqualCallArgs(f, '4, c=4')
        self.assertEqualException(f, '')
        self.assertEqualException(f, '3')
        self.assertEqualException(f, 'a=3')
        self.assertEqualException(f, 'd=4')

        f = self.makeCallable('*, c, d=2')
        self.assertEqualCallArgs(f, 'c=3')
        self.assertEqualCallArgs(f, 'c=3, d=4')
        self.assertEqualCallArgs(f, 'd=4, c=3')

    def test_multiple_features(self):
        f = self.makeCallable('a, b=2, *f, **g')
        self.assertEqualCallArgs(f, '2, 3, 7')
        self.assertEqualCallArgs(f, '2, 3, x=8')
        self.assertEqualCallArgs(f, '2, 3, x=8, *[(4,[5,6]), 7]')
        self.assertEqualCallArgs(f, '2, x=8, *[3, (4,[5,6]), 7], y=9')
        self.assertEqualCallArgs(f, 'x=8, *[2, 3, (4,[5,6])], y=9')
        self.assertEqualCallArgs(f, 'x=8, *collections.UserList('
                                 '[2, 3, (4,[5,6])]), **{"y":9, "z":10}')
        self.assertEqualCallArgs(f, '2, x=8, *collections.UserList([3, '
                                 '(4,[5,6])]), **collections.UserDict('
                                 'y=9, z=10)')

        f = self.makeCallable('a, b=2, *f, x, y=99, **g')
        self.assertEqualCallArgs(f, '2, 3, x=8')
        self.assertEqualCallArgs(f, '2, 3, x=8, *[(4,[5,6]), 7]')
        self.assertEqualCallArgs(f, '2, x=8, *[3, (4,[5,6]), 7], y=9, z=10')
        self.assertEqualCallArgs(f, 'x=8, *[2, 3, (4,[5,6])], y=9, z=10')
        self.assertEqualCallArgs(f, 'x=8, *collections.UserList('
                                 '[2, 3, (4,[5,6])]), q=0, **{"y":9, "z":10}')
        self.assertEqualCallArgs(f, '2, x=8, *collections.UserList([3, '
                                 '(4,[5,6])]), q=0, **collections.UserDict('
                                 'y=9, z=10)')

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
            # XXX: success of this one depends on dict order
            ## self.assertEqualException(f, '2, 3, 4, a=1, c=5')
            # f got an unexpected keyword argument
            self.assertEqualException(f, 'c=2')
            self.assertEqualException(f, '2, c=3')
            self.assertEqualException(f, '2, 3, c=4')
            self.assertEqualException(f, '2, c=4, b=3')
            self.assertEqualException(f, '**{u"\u03c0\u03b9": 4}')
            # f got multiple values for keyword argument
            self.assertEqualException(f, '1, a=2')
            self.assertEqualException(f, '1, **{"a":2}')
            self.assertEqualException(f, '1, 2, b=3')
            # XXX: Python inconsistency
            # - for functions and bound methods: unexpected keyword 'c'
            # - for unbound methods: multiple values for keyword 'a'
            #self.assertEqualException(f, '1, c=3, a=2')
        # issue11256:
        f3 = self.makeCallable('**c')
        self.assertEqualException(f3, '1, 2')
        self.assertEqualException(f3, '1, 2, a=1, b=2')
        f4 = self.makeCallable('*, a, b=0')
        self.assertEqualException(f3, '1, 2')
        self.assertEqualException(f3, '1, 2, a=1, b=2')

        # issue #20816: getcallargs() fails to iterate over non-existent
        # kwonlydefaults and raises a wrong TypeError
        def f5(*, a): pass
        with self.assertRaisesRegex(TypeError,
                                    'missing 1 required keyword-only'):
            inspect.getcallargs(f5)


        # issue20817:
        def f6(a, b, c):
            pass
        with self.assertRaisesRegex(TypeError, "'a', 'b' and 'c'"):
            inspect.getcallargs(f6)

        # bpo-33197
        with self.assertRaisesRegex(ValueError,
                                    'variadic keyword parameters cannot'
                                    ' have default values'):
            inspect.Parameter("foo", kind=inspect.Parameter.VAR_KEYWORD,
                              default=42)
        with self.assertRaisesRegex(ValueError,
                                    "value 5 is not a valid Parameter.kind"):
            inspect.Parameter("bar", kind=5, default=42)

        with self.assertRaisesRegex(TypeError,
                                   'name must be a str, not a int'):
            inspect.Parameter(123, kind=4)

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


class TestGetattrStatic(unittest.TestCase):

    def test_basic(self):
        class Thing(object):
            x = object()

        thing = Thing()
        self.assertEqual(inspect.getattr_static(thing, 'x'), Thing.x)
        self.assertEqual(inspect.getattr_static(thing, 'x', None), Thing.x)
        with self.assertRaises(AttributeError):
            inspect.getattr_static(thing, 'y')

        self.assertEqual(inspect.getattr_static(thing, 'y', 3), 3)

    def test_inherited(self):
        class Thing(object):
            x = object()
        class OtherThing(Thing):
            pass

        something = OtherThing()
        self.assertEqual(inspect.getattr_static(something, 'x'), Thing.x)

    def test_instance_attr(self):
        class Thing(object):
            x = 2
            def __init__(self, x):
                self.x = x
        thing = Thing(3)
        self.assertEqual(inspect.getattr_static(thing, 'x'), 3)
        del thing.x
        self.assertEqual(inspect.getattr_static(thing, 'x'), 2)

    def test_property(self):
        class Thing(object):
            @property
            def x(self):
                raise AttributeError("I'm pretending not to exist")
        thing = Thing()
        self.assertEqual(inspect.getattr_static(thing, 'x'), Thing.x)

    def test_descriptor_raises_AttributeError(self):
        class descriptor(object):
            def __get__(*_):
                raise AttributeError("I'm pretending not to exist")
        desc = descriptor()
        class Thing(object):
            x = desc
        thing = Thing()
        self.assertEqual(inspect.getattr_static(thing, 'x'), desc)

    def test_classAttribute(self):
        class Thing(object):
            x = object()

        self.assertEqual(inspect.getattr_static(Thing, 'x'), Thing.x)

    def test_classVirtualAttribute(self):
        class Thing(object):
            @types.DynamicClassAttribute
            def x(self):
                return self._x
            _x = object()

        self.assertEqual(inspect.getattr_static(Thing, 'x'), Thing.__dict__['x'])

    def test_inherited_classattribute(self):
        class Thing(object):
            x = object()
        class OtherThing(Thing):
            pass

        self.assertEqual(inspect.getattr_static(OtherThing, 'x'), Thing.x)

    def test_slots(self):
        class Thing(object):
            y = 'bar'
            __slots__ = ['x']
            def __init__(self):
                self.x = 'foo'
        thing = Thing()
        self.assertEqual(inspect.getattr_static(thing, 'x'), Thing.x)
        self.assertEqual(inspect.getattr_static(thing, 'y'), 'bar')

        del thing.x
        self.assertEqual(inspect.getattr_static(thing, 'x'), Thing.x)

    def test_metaclass(self):
        class meta(type):
            attr = 'foo'
        class Thing(object, metaclass=meta):
            pass
        self.assertEqual(inspect.getattr_static(Thing, 'attr'), 'foo')

        class sub(meta):
            pass
        class OtherThing(object, metaclass=sub):
            x = 3
        self.assertEqual(inspect.getattr_static(OtherThing, 'attr'), 'foo')

        class OtherOtherThing(OtherThing):
            pass
        # this test is odd, but it was added as it exposed a bug
        self.assertEqual(inspect.getattr_static(OtherOtherThing, 'x'), 3)

    def test_no_dict_no_slots(self):
        self.assertEqual(inspect.getattr_static(1, 'foo', None), None)
        self.assertNotEqual(inspect.getattr_static('foo', 'lower'), None)

    def test_no_dict_no_slots_instance_member(self):
        # returns descriptor
        with open(__file__, encoding='utf-8') as handle:
            self.assertEqual(inspect.getattr_static(handle, 'name'), type(handle).name)

    def test_inherited_slots(self):
        # returns descriptor
        class Thing(object):
            __slots__ = ['x']
            def __init__(self):
                self.x = 'foo'

        class OtherThing(Thing):
            pass
        # it would be nice if this worked...
        # we get the descriptor instead of the instance attribute
        self.assertEqual(inspect.getattr_static(OtherThing(), 'x'), Thing.x)

    def test_descriptor(self):
        class descriptor(object):
            def __get__(self, instance, owner):
                return 3
        class Foo(object):
            d = descriptor()

        foo = Foo()

        # for a non data descriptor we return the instance attribute
        foo.__dict__['d'] = 1
        self.assertEqual(inspect.getattr_static(foo, 'd'), 1)

        # if the descriptor is a data-descriptor we should return the
        # descriptor
        descriptor.__set__ = lambda s, i, v: None
        self.assertEqual(inspect.getattr_static(foo, 'd'), Foo.__dict__['d'])


    def test_metaclass_with_descriptor(self):
        class descriptor(object):
            def __get__(self, instance, owner):
                return 3
        class meta(type):
            d = descriptor()
        class Thing(object, metaclass=meta):
            pass
        self.assertEqual(inspect.getattr_static(Thing, 'd'), meta.__dict__['d'])


    def test_class_as_property(self):
        class Base(object):
            foo = 3

        class Something(Base):
            executed = False
            @property
            def __class__(self):
                self.executed = True
                return object

        instance = Something()
        self.assertEqual(inspect.getattr_static(instance, 'foo'), 3)
        self.assertFalse(instance.executed)
        self.assertEqual(inspect.getattr_static(Something, 'foo'), 3)

    def test_mro_as_property(self):
        class Meta(type):
            @property
            def __mro__(self):
                return (object,)

        class Base(object):
            foo = 3

        class Something(Base, metaclass=Meta):
            pass

        self.assertEqual(inspect.getattr_static(Something(), 'foo'), 3)
        self.assertEqual(inspect.getattr_static(Something, 'foo'), 3)

    def test_dict_as_property(self):
        test = self
        test.called = False

        class Foo(dict):
            a = 3
            @property
            def __dict__(self):
                test.called = True
                return {}

        foo = Foo()
        foo.a = 4
        self.assertEqual(inspect.getattr_static(foo, 'a'), 3)
        self.assertFalse(test.called)

    def test_custom_object_dict(self):
        test = self
        test.called = False

        class Custom(dict):
            def get(self, key, default=None):
                test.called = True
                super().get(key, default)

        class Foo(object):
            a = 3
        foo = Foo()
        foo.__dict__ = Custom()
        self.assertEqual(inspect.getattr_static(foo, 'a'), 3)
        self.assertFalse(test.called)

    def test_metaclass_dict_as_property(self):
        class Meta(type):
            @property
            def __dict__(self):
                self.executed = True

        class Thing(metaclass=Meta):
            executed = False

            def __init__(self):
                self.spam = 42

        instance = Thing()
        self.assertEqual(inspect.getattr_static(instance, "spam"), 42)
        self.assertFalse(Thing.executed)

    def test_module(self):
        sentinel = object()
        self.assertIsNot(inspect.getattr_static(sys, "version", sentinel),
                         sentinel)

    def test_metaclass_with_metaclass_with_dict_as_property(self):
        class MetaMeta(type):
            @property
            def __dict__(self):
                self.executed = True
                return dict(spam=42)

        class Meta(type, metaclass=MetaMeta):
            executed = False

        class Thing(metaclass=Meta):
            pass

        with self.assertRaises(AttributeError):
            inspect.getattr_static(Thing, "spam")
        self.assertFalse(Thing.executed)

class TestGetGeneratorState(unittest.TestCase):

    def setUp(self):
        def number_generator():
            for number in range(5):
                yield number
        self.generator = number_generator()

    def _generatorstate(self):
        return inspect.getgeneratorstate(self.generator)

    def test_created(self):
        self.assertEqual(self._generatorstate(), inspect.GEN_CREATED)

    def test_suspended(self):
        next(self.generator)
        self.assertEqual(self._generatorstate(), inspect.GEN_SUSPENDED)

    def test_closed_after_exhaustion(self):
        for i in self.generator:
            pass
        self.assertEqual(self._generatorstate(), inspect.GEN_CLOSED)

    def test_closed_after_immediate_exception(self):
        with self.assertRaises(RuntimeError):
            self.generator.throw(RuntimeError)
        self.assertEqual(self._generatorstate(), inspect.GEN_CLOSED)

    def test_running(self):
        # As mentioned on issue #10220, checking for the RUNNING state only
        # makes sense inside the generator itself.
        # The following generator checks for this by using the closure's
        # reference to self and the generator state checking helper method
        def running_check_generator():
            for number in range(5):
                self.assertEqual(self._generatorstate(), inspect.GEN_RUNNING)
                yield number
                self.assertEqual(self._generatorstate(), inspect.GEN_RUNNING)
        self.generator = running_check_generator()
        # Running up to the first yield
        next(self.generator)
        # Running after the first yield
        next(self.generator)

    def test_easy_debugging(self):
        # repr() and str() of a generator state should contain the state name
        names = 'GEN_CREATED GEN_RUNNING GEN_SUSPENDED GEN_CLOSED'.split()
        for name in names:
            state = getattr(inspect, name)
            self.assertIn(name, repr(state))
            self.assertIn(name, str(state))

    def test_getgeneratorlocals(self):
        def each(lst, a=None):
            b=(1, 2, 3)
            for v in lst:
                if v == 3:
                    c = 12
                yield v

        numbers = each([1, 2, 3])
        self.assertEqual(inspect.getgeneratorlocals(numbers),
                         {'a': None, 'lst': [1, 2, 3]})
        next(numbers)
        self.assertEqual(inspect.getgeneratorlocals(numbers),
                         {'a': None, 'lst': [1, 2, 3], 'v': 1,
                          'b': (1, 2, 3)})
        next(numbers)
        self.assertEqual(inspect.getgeneratorlocals(numbers),
                         {'a': None, 'lst': [1, 2, 3], 'v': 2,
                          'b': (1, 2, 3)})
        next(numbers)
        self.assertEqual(inspect.getgeneratorlocals(numbers),
                         {'a': None, 'lst': [1, 2, 3], 'v': 3,
                          'b': (1, 2, 3), 'c': 12})
        try:
            next(numbers)
        except StopIteration:
            pass
        self.assertEqual(inspect.getgeneratorlocals(numbers), {})

    def test_getgeneratorlocals_empty(self):
        def yield_one():
            yield 1
        one = yield_one()
        self.assertEqual(inspect.getgeneratorlocals(one), {})
        try:
            next(one)
        except StopIteration:
            pass
        self.assertEqual(inspect.getgeneratorlocals(one), {})

    def test_getgeneratorlocals_error(self):
        self.assertRaises(TypeError, inspect.getgeneratorlocals, 1)
        self.assertRaises(TypeError, inspect.getgeneratorlocals, lambda x: True)
        self.assertRaises(TypeError, inspect.getgeneratorlocals, set)
        self.assertRaises(TypeError, inspect.getgeneratorlocals, (2,3))


class TestGetCoroutineState(unittest.TestCase):

    def setUp(self):
        @types.coroutine
        def number_coroutine():
            for number in range(5):
                yield number
        async def coroutine():
            await number_coroutine()
        self.coroutine = coroutine()

    def tearDown(self):
        self.coroutine.close()

    def _coroutinestate(self):
        return inspect.getcoroutinestate(self.coroutine)

    def test_created(self):
        self.assertEqual(self._coroutinestate(), inspect.CORO_CREATED)

    def test_suspended(self):
        self.coroutine.send(None)
        self.assertEqual(self._coroutinestate(), inspect.CORO_SUSPENDED)

    def test_closed_after_exhaustion(self):
        while True:
            try:
                self.coroutine.send(None)
            except StopIteration:
                break

        self.assertEqual(self._coroutinestate(), inspect.CORO_CLOSED)

    def test_closed_after_immediate_exception(self):
        with self.assertRaises(RuntimeError):
            self.coroutine.throw(RuntimeError)
        self.assertEqual(self._coroutinestate(), inspect.CORO_CLOSED)

    def test_easy_debugging(self):
        # repr() and str() of a coroutine state should contain the state name
        names = 'CORO_CREATED CORO_RUNNING CORO_SUSPENDED CORO_CLOSED'.split()
        for name in names:
            state = getattr(inspect, name)
            self.assertIn(name, repr(state))
            self.assertIn(name, str(state))

    def test_getcoroutinelocals(self):
        @types.coroutine
        def gencoro():
            yield

        gencoro = gencoro()
        async def func(a=None):
            b = 'spam'
            await gencoro

        coro = func()
        self.assertEqual(inspect.getcoroutinelocals(coro),
                         {'a': None, 'gencoro': gencoro})
        coro.send(None)
        self.assertEqual(inspect.getcoroutinelocals(coro),
                         {'a': None, 'gencoro': gencoro, 'b': 'spam'})


class MySignature(inspect.Signature):
    # Top-level to make it picklable;
    # used in test_signature_object_pickle
    pass

class MyParameter(inspect.Parameter):
    # Top-level to make it picklable;
    # used in test_signature_object_pickle
    pass



class TestSignatureObject(unittest.TestCase):
    @staticmethod
    def signature(func, **kw):
        sig = inspect.signature(func, **kw)
        return (tuple((param.name,
                       (... if param.default is param.empty else param.default),
                       (... if param.annotation is param.empty
                                                        else param.annotation),
                       str(param.kind).lower())
                                    for param in sig.parameters.values()),
                (... if sig.return_annotation is sig.empty
                                            else sig.return_annotation))

    def test_signature_object(self):
        S = inspect.Signature
        P = inspect.Parameter

        self.assertEqual(str(S()), '()')
        self.assertEqual(repr(S().parameters), 'mappingproxy(OrderedDict())')

        def test(po, pk, pod=42, pkd=100, *args, ko, **kwargs):
            pass
        sig = inspect.signature(test)
        po = sig.parameters['po'].replace(kind=P.POSITIONAL_ONLY)
        pod = sig.parameters['pod'].replace(kind=P.POSITIONAL_ONLY)
        pk = sig.parameters['pk']
        pkd = sig.parameters['pkd']
        args = sig.parameters['args']
        ko = sig.parameters['ko']
        kwargs = sig.parameters['kwargs']

        S((po, pk, args, ko, kwargs))

        with self.assertRaisesRegex(ValueError, 'wrong parameter order'):
            S((pk, po, args, ko, kwargs))

        with self.assertRaisesRegex(ValueError, 'wrong parameter order'):
            S((po, args, pk, ko, kwargs))

        with self.assertRaisesRegex(ValueError, 'wrong parameter order'):
            S((args, po, pk, ko, kwargs))

        with self.assertRaisesRegex(ValueError, 'wrong parameter order'):
            S((po, pk, args, kwargs, ko))

        kwargs2 = kwargs.replace(name='args')
        with self.assertRaisesRegex(ValueError, 'duplicate parameter name'):
            S((po, pk, args, kwargs2, ko))

        with self.assertRaisesRegex(ValueError, 'follows default argument'):
            S((pod, po))

        with self.assertRaisesRegex(ValueError, 'follows default argument'):
            S((po, pkd, pk))

        with self.assertRaisesRegex(ValueError, 'follows default argument'):
            S((pkd, pk))

        self.assertTrue(repr(sig).startswith('<Signature'))
        self.assertTrue('(po, pk' in repr(sig))

    def test_signature_object_pickle(self):
        def foo(a, b, *, c:1={}, **kw) -> {42:'ham'}: pass
        foo_partial = functools.partial(foo, a=1)

        sig = inspect.signature(foo_partial)

        for ver in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.subTest(pickle_ver=ver, subclass=False):
                sig_pickled = pickle.loads(pickle.dumps(sig, ver))
                self.assertEqual(sig, sig_pickled)

        # Test that basic sub-classing works
        sig = inspect.signature(foo)
        myparam = MyParameter(name='z', kind=inspect.Parameter.POSITIONAL_ONLY)
        myparams = collections.OrderedDict(sig.parameters, a=myparam)
        mysig = MySignature().replace(parameters=myparams.values(),
                                      return_annotation=sig.return_annotation)
        self.assertTrue(isinstance(mysig, MySignature))
        self.assertTrue(isinstance(mysig.parameters['z'], MyParameter))

        for ver in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.subTest(pickle_ver=ver, subclass=True):
                sig_pickled = pickle.loads(pickle.dumps(mysig, ver))
                self.assertEqual(mysig, sig_pickled)
                self.assertTrue(isinstance(sig_pickled, MySignature))
                self.assertTrue(isinstance(sig_pickled.parameters['z'],
                                           MyParameter))

    def test_signature_immutability(self):
        def test(a):
            pass
        sig = inspect.signature(test)

        with self.assertRaises(AttributeError):
            sig.foo = 'bar'

        with self.assertRaises(TypeError):
            sig.parameters['a'] = None

    def test_signature_on_noarg(self):
        def test():
            pass
        self.assertEqual(self.signature(test), ((), ...))

    def test_signature_on_wargs(self):
        def test(a, b:'foo') -> 123:
            pass
        self.assertEqual(self.signature(test),
                         ((('a', ..., ..., "positional_or_keyword"),
                           ('b', ..., 'foo', "positional_or_keyword")),
                          123))

    def test_signature_on_wkwonly(self):
        def test(*, a:float, b:str) -> int:
            pass
        self.assertEqual(self.signature(test),
                         ((('a', ..., float, "keyword_only"),
                           ('b', ..., str, "keyword_only")),
                           int))

    def test_signature_on_complex_args(self):
        def test(a, b:'foo'=10, *args:'bar', spam:'baz', ham=123, **kwargs:int):
            pass
        self.assertEqual(self.signature(test),
                         ((('a', ..., ..., "positional_or_keyword"),
                           ('b', 10, 'foo', "positional_or_keyword"),
                           ('args', ..., 'bar', "var_positional"),
                           ('spam', ..., 'baz', "keyword_only"),
                           ('ham', 123, ..., "keyword_only"),
                           ('kwargs', ..., int, "var_keyword")),
                          ...))

    def test_signature_without_self(self):
        def test_args_only(*args):  # NOQA
            pass

        def test_args_kwargs_only(*args, **kwargs):  # NOQA
            pass

        class A:
            @classmethod
            def test_classmethod(*args):  # NOQA
                pass

            @staticmethod
            def test_staticmethod(*args):  # NOQA
                pass

            f1 = functools.partialmethod((test_classmethod), 1)
            f2 = functools.partialmethod((test_args_only), 1)
            f3 = functools.partialmethod((test_staticmethod), 1)
            f4 = functools.partialmethod((test_args_kwargs_only),1)

        self.assertEqual(self.signature(test_args_only),
                         ((('args', ..., ..., 'var_positional'),), ...))
        self.assertEqual(self.signature(test_args_kwargs_only),
                         ((('args', ..., ..., 'var_positional'),
                           ('kwargs', ..., ..., 'var_keyword')), ...))
        self.assertEqual(self.signature(A.f1),
                         ((('args', ..., ..., 'var_positional'),), ...))
        self.assertEqual(self.signature(A.f2),
                         ((('args', ..., ..., 'var_positional'),), ...))
        self.assertEqual(self.signature(A.f3),
                         ((('args', ..., ..., 'var_positional'),), ...))
        self.assertEqual(self.signature(A.f4),
                         ((('args', ..., ..., 'var_positional'),
                            ('kwargs', ..., ..., 'var_keyword')), ...))
    @cpython_only
    @unittest.skipIf(MISSING_C_DOCSTRINGS,
                     "Signature information for builtins requires docstrings")
    def test_signature_on_builtins(self):
        import _testcapi

        def test_unbound_method(o):
            """Use this to test unbound methods (things that should have a self)"""
            signature = inspect.signature(o)
            self.assertTrue(isinstance(signature, inspect.Signature))
            self.assertEqual(list(signature.parameters.values())[0].name, 'self')
            return signature

        def test_callable(o):
            """Use this to test bound methods or normal callables (things that don't expect self)"""
            signature = inspect.signature(o)
            self.assertTrue(isinstance(signature, inspect.Signature))
            if signature.parameters:
                self.assertNotEqual(list(signature.parameters.values())[0].name, 'self')
            return signature

        signature = test_callable(_testcapi.docstring_with_signature_with_defaults)
        def p(name): return signature.parameters[name].default
        self.assertEqual(p('s'), 'avocado')
        self.assertEqual(p('b'), b'bytes')
        self.assertEqual(p('d'), 3.14)
        self.assertEqual(p('i'), 35)
        self.assertEqual(p('n'), None)
        self.assertEqual(p('t'), True)
        self.assertEqual(p('f'), False)
        self.assertEqual(p('local'), 3)
        self.assertEqual(p('sys'), sys.maxsize)
        self.assertNotIn('exp', signature.parameters)

        test_callable(object)

        # normal method
        # (PyMethodDescr_Type, "method_descriptor")
        test_unbound_method(_pickle.Pickler.dump)
        d = _pickle.Pickler(io.StringIO())
        test_callable(d.dump)

        # static method
        test_callable(bytes.maketrans)
        test_callable(b'abc'.maketrans)

        # class method
        test_callable(dict.fromkeys)
        test_callable({}.fromkeys)

        # wrapper around slot (PyWrapperDescr_Type, "wrapper_descriptor")
        test_unbound_method(type.__call__)
        test_unbound_method(int.__add__)
        test_callable((3).__add__)

        # _PyMethodWrapper_Type
        # support for 'method-wrapper'
        test_callable(min.__call__)

        # This doesn't work now.
        # (We don't have a valid signature for "type" in 3.4)
        with self.assertRaisesRegex(ValueError, "no signature found"):
            class ThisWorksNow:
                __call__ = type
            test_callable(ThisWorksNow())

        # Regression test for issue #20786
        test_unbound_method(dict.__delitem__)
        test_unbound_method(property.__delete__)

        # Regression test for issue #20586
        test_callable(_testcapi.docstring_with_signature_but_no_doc)

    @cpython_only
    @unittest.skipIf(MISSING_C_DOCSTRINGS,
                     "Signature information for builtins requires docstrings")
    def test_signature_on_decorated_builtins(self):
        import _testcapi
        func = _testcapi.docstring_with_signature_with_defaults

        def decorator(func):
            @functools.wraps(func)
            def wrapper(*args, **kwargs) -> int:
                return func(*args, **kwargs)
            return wrapper

        decorated_func = decorator(func)

        self.assertEqual(inspect.signature(func),
                         inspect.signature(decorated_func))

        def wrapper_like(*args, **kwargs) -> int: pass
        self.assertEqual(inspect.signature(decorated_func,
                                           follow_wrapped=False),
                         inspect.signature(wrapper_like))

    @cpython_only
    def test_signature_on_builtins_no_signature(self):
        import _testcapi
        with self.assertRaisesRegex(ValueError,
                                    'no signature found for builtin'):
            inspect.signature(_testcapi.docstring_no_signature)

        with self.assertRaisesRegex(ValueError,
                                    'no signature found for builtin'):
            inspect.signature(str)

    def test_signature_on_non_function(self):
        with self.assertRaisesRegex(TypeError, 'is not a callable object'):
            inspect.signature(42)

    def test_signature_from_functionlike_object(self):
        def func(a,b, *args, kwonly=True, kwonlyreq, **kwargs):
            pass

        class funclike:
            # Has to be callable, and have correct
            # __code__, __annotations__, __defaults__, __name__,
            # and __kwdefaults__ attributes

            def __init__(self, func):
                self.__name__ = func.__name__
                self.__code__ = func.__code__
                self.__annotations__ = func.__annotations__
                self.__defaults__ = func.__defaults__
                self.__kwdefaults__ = func.__kwdefaults__
                self.func = func

            def __call__(self, *args, **kwargs):
                return self.func(*args, **kwargs)

        sig_func = inspect.Signature.from_callable(func)

        sig_funclike = inspect.Signature.from_callable(funclike(func))
        self.assertEqual(sig_funclike, sig_func)

        sig_funclike = inspect.signature(funclike(func))
        self.assertEqual(sig_funclike, sig_func)

        # If object is not a duck type of function, then
        # signature will try to get a signature for its '__call__'
        # method
        fl = funclike(func)
        del fl.__defaults__
        self.assertEqual(self.signature(fl),
                         ((('args', ..., ..., "var_positional"),
                           ('kwargs', ..., ..., "var_keyword")),
                           ...))

        # Test with cython-like builtins:
        _orig_isdesc = inspect.ismethoddescriptor
        def _isdesc(obj):
            if hasattr(obj, '_builtinmock'):
                return True
            return _orig_isdesc(obj)

        with unittest.mock.patch('inspect.ismethoddescriptor', _isdesc):
            builtin_func = funclike(func)
            # Make sure that our mock setup is working
            self.assertFalse(inspect.ismethoddescriptor(builtin_func))
            builtin_func._builtinmock = True
            self.assertTrue(inspect.ismethoddescriptor(builtin_func))
            self.assertEqual(inspect.signature(builtin_func), sig_func)

    def test_signature_functionlike_class(self):
        # We only want to duck type function-like objects,
        # not classes.

        def func(a,b, *args, kwonly=True, kwonlyreq, **kwargs):
            pass

        class funclike:
            def __init__(self, marker):
                pass

            __name__ = func.__name__
            __code__ = func.__code__
            __annotations__ = func.__annotations__
            __defaults__ = func.__defaults__
            __kwdefaults__ = func.__kwdefaults__

        self.assertEqual(str(inspect.signature(funclike)), '(marker)')

    def test_signature_on_method(self):
        class Test:
            def __init__(*args):
                pass
            def m1(self, arg1, arg2=1) -> int:
                pass
            def m2(*args):
                pass
            def __call__(*, a):
                pass

        self.assertEqual(self.signature(Test().m1),
                         ((('arg1', ..., ..., "positional_or_keyword"),
                           ('arg2', 1, ..., "positional_or_keyword")),
                          int))

        self.assertEqual(self.signature(Test().m2),
                         ((('args', ..., ..., "var_positional"),),
                          ...))

        self.assertEqual(self.signature(Test),
                         ((('args', ..., ..., "var_positional"),),
                          ...))

        with self.assertRaisesRegex(ValueError, 'invalid method signature'):
            self.signature(Test())

    def test_signature_wrapped_bound_method(self):
        # Issue 24298
        class Test:
            def m1(self, arg1, arg2=1) -> int:
                pass
        @functools.wraps(Test().m1)
        def m1d(*args, **kwargs):
            pass
        self.assertEqual(self.signature(m1d),
                         ((('arg1', ..., ..., "positional_or_keyword"),
                           ('arg2', 1, ..., "positional_or_keyword")),
                          int))

    def test_signature_on_classmethod(self):
        class Test:
            @classmethod
            def foo(cls, arg1, *, arg2=1):
                pass

        meth = Test().foo
        self.assertEqual(self.signature(meth),
                         ((('arg1', ..., ..., "positional_or_keyword"),
                           ('arg2', 1, ..., "keyword_only")),
                          ...))

        meth = Test.foo
        self.assertEqual(self.signature(meth),
                         ((('arg1', ..., ..., "positional_or_keyword"),
                           ('arg2', 1, ..., "keyword_only")),
                          ...))

    def test_signature_on_staticmethod(self):
        class Test:
            @staticmethod
            def foo(cls, *, arg):
                pass

        meth = Test().foo
        self.assertEqual(self.signature(meth),
                         ((('cls', ..., ..., "positional_or_keyword"),
                           ('arg', ..., ..., "keyword_only")),
                          ...))

        meth = Test.foo
        self.assertEqual(self.signature(meth),
                         ((('cls', ..., ..., "positional_or_keyword"),
                           ('arg', ..., ..., "keyword_only")),
                          ...))

    def test_signature_on_partial(self):
        from functools import partial

        Parameter = inspect.Parameter

        def test():
            pass

        self.assertEqual(self.signature(partial(test)), ((), ...))

        with self.assertRaisesRegex(ValueError, "has incorrect arguments"):
            inspect.signature(partial(test, 1))

        with self.assertRaisesRegex(ValueError, "has incorrect arguments"):
            inspect.signature(partial(test, a=1))

        def test(a, b, *, c, d):
            pass

        self.assertEqual(self.signature(partial(test)),
                         ((('a', ..., ..., "positional_or_keyword"),
                           ('b', ..., ..., "positional_or_keyword"),
                           ('c', ..., ..., "keyword_only"),
                           ('d', ..., ..., "keyword_only")),
                          ...))

        self.assertEqual(self.signature(partial(test, 1)),
                         ((('b', ..., ..., "positional_or_keyword"),
                           ('c', ..., ..., "keyword_only"),
                           ('d', ..., ..., "keyword_only")),
                          ...))

        self.assertEqual(self.signature(partial(test, 1, c=2)),
                         ((('b', ..., ..., "positional_or_keyword"),
                           ('c', 2, ..., "keyword_only"),
                           ('d', ..., ..., "keyword_only")),
                          ...))

        self.assertEqual(self.signature(partial(test, b=1, c=2)),
                         ((('a', ..., ..., "positional_or_keyword"),
                           ('b', 1, ..., "keyword_only"),
                           ('c', 2, ..., "keyword_only"),
                           ('d', ..., ..., "keyword_only")),
                          ...))

        self.assertEqual(self.signature(partial(test, 0, b=1, c=2)),
                         ((('b', 1, ..., "keyword_only"),
                           ('c', 2, ..., "keyword_only"),
                           ('d', ..., ..., "keyword_only")),
                          ...))

        self.assertEqual(self.signature(partial(test, a=1)),
                         ((('a', 1, ..., "keyword_only"),
                           ('b', ..., ..., "keyword_only"),
                           ('c', ..., ..., "keyword_only"),
                           ('d', ..., ..., "keyword_only")),
                          ...))

        def test(a, *args, b, **kwargs):
            pass

        self.assertEqual(self.signature(partial(test, 1)),
                         ((('args', ..., ..., "var_positional"),
                           ('b', ..., ..., "keyword_only"),
                           ('kwargs', ..., ..., "var_keyword")),
                          ...))

        self.assertEqual(self.signature(partial(test, a=1)),
                         ((('a', 1, ..., "keyword_only"),
                           ('b', ..., ..., "keyword_only"),
                           ('kwargs', ..., ..., "var_keyword")),
                          ...))

        self.assertEqual(self.signature(partial(test, 1, 2, 3)),
                         ((('args', ..., ..., "var_positional"),
                           ('b', ..., ..., "keyword_only"),
                           ('kwargs', ..., ..., "var_keyword")),
                          ...))

        self.assertEqual(self.signature(partial(test, 1, 2, 3, test=True)),
                         ((('args', ..., ..., "var_positional"),
                           ('b', ..., ..., "keyword_only"),
                           ('kwargs', ..., ..., "var_keyword")),
                          ...))

        self.assertEqual(self.signature(partial(test, 1, 2, 3, test=1, b=0)),
                         ((('args', ..., ..., "var_positional"),
                           ('b', 0, ..., "keyword_only"),
                           ('kwargs', ..., ..., "var_keyword")),
                          ...))

        self.assertEqual(self.signature(partial(test, b=0)),
                         ((('a', ..., ..., "positional_or_keyword"),
                           ('args', ..., ..., "var_positional"),
                           ('b', 0, ..., "keyword_only"),
                           ('kwargs', ..., ..., "var_keyword")),
                          ...))

        self.assertEqual(self.signature(partial(test, b=0, test=1)),
                         ((('a', ..., ..., "positional_or_keyword"),
                           ('args', ..., ..., "var_positional"),
                           ('b', 0, ..., "keyword_only"),
                           ('kwargs', ..., ..., "var_keyword")),
                          ...))

        def test(a, b, c:int) -> 42:
            pass

        sig = test.__signature__ = inspect.signature(test)

        self.assertEqual(self.signature(partial(partial(test, 1))),
                         ((('b', ..., ..., "positional_or_keyword"),
                           ('c', ..., int, "positional_or_keyword")),
                          42))

        self.assertEqual(self.signature(partial(partial(test, 1), 2)),
                         ((('c', ..., int, "positional_or_keyword"),),
                          42))

        psig = inspect.signature(partial(partial(test, 1), 2))

        def foo(a):
            return a
        _foo = partial(partial(foo, a=10), a=20)
        self.assertEqual(self.signature(_foo),
                         ((('a', 20, ..., "keyword_only"),),
                          ...))
        # check that we don't have any side-effects in signature(),
        # and the partial object is still functioning
        self.assertEqual(_foo(), 20)

        def foo(a, b, c):
            return a, b, c
        _foo = partial(partial(foo, 1, b=20), b=30)

        self.assertEqual(self.signature(_foo),
                         ((('b', 30, ..., "keyword_only"),
                           ('c', ..., ..., "keyword_only")),
                          ...))
        self.assertEqual(_foo(c=10), (1, 30, 10))

        def foo(a, b, c, *, d):
            return a, b, c, d
        _foo = partial(partial(foo, d=20, c=20), b=10, d=30)
        self.assertEqual(self.signature(_foo),
                         ((('a', ..., ..., "positional_or_keyword"),
                           ('b', 10, ..., "keyword_only"),
                           ('c', 20, ..., "keyword_only"),
                           ('d', 30, ..., "keyword_only"),
                           ),
                          ...))
        ba = inspect.signature(_foo).bind(a=200, b=11)
        self.assertEqual(_foo(*ba.args, **ba.kwargs), (200, 11, 20, 30))

        def foo(a=1, b=2, c=3):
            return a, b, c
        _foo = partial(foo, c=13) # (a=1, b=2, *, c=13)

        ba = inspect.signature(_foo).bind(a=11)
        self.assertEqual(_foo(*ba.args, **ba.kwargs), (11, 2, 13))

        ba = inspect.signature(_foo).bind(11, 12)
        self.assertEqual(_foo(*ba.args, **ba.kwargs), (11, 12, 13))

        ba = inspect.signature(_foo).bind(11, b=12)
        self.assertEqual(_foo(*ba.args, **ba.kwargs), (11, 12, 13))

        ba = inspect.signature(_foo).bind(b=12)
        self.assertEqual(_foo(*ba.args, **ba.kwargs), (1, 12, 13))

        _foo = partial(_foo, b=10, c=20)
        ba = inspect.signature(_foo).bind(12)
        self.assertEqual(_foo(*ba.args, **ba.kwargs), (12, 10, 20))


        def foo(a, b, c, d, **kwargs):
            pass
        sig = inspect.signature(foo)
        params = sig.parameters.copy()
        params['a'] = params['a'].replace(kind=Parameter.POSITIONAL_ONLY)
        params['b'] = params['b'].replace(kind=Parameter.POSITIONAL_ONLY)
        foo.__signature__ = inspect.Signature(params.values())
        sig = inspect.signature(foo)
        self.assertEqual(str(sig), '(a, b, /, c, d, **kwargs)')

        self.assertEqual(self.signature(partial(foo, 1)),
                         ((('b', ..., ..., 'positional_only'),
                           ('c', ..., ..., 'positional_or_keyword'),
                           ('d', ..., ..., 'positional_or_keyword'),
                           ('kwargs', ..., ..., 'var_keyword')),
                         ...))

        self.assertEqual(self.signature(partial(foo, 1, 2)),
                         ((('c', ..., ..., 'positional_or_keyword'),
                           ('d', ..., ..., 'positional_or_keyword'),
                           ('kwargs', ..., ..., 'var_keyword')),
                         ...))

        self.assertEqual(self.signature(partial(foo, 1, 2, 3)),
                         ((('d', ..., ..., 'positional_or_keyword'),
                           ('kwargs', ..., ..., 'var_keyword')),
                         ...))

        self.assertEqual(self.signature(partial(foo, 1, 2, c=3)),
                         ((('c', 3, ..., 'keyword_only'),
                           ('d', ..., ..., 'keyword_only'),
                           ('kwargs', ..., ..., 'var_keyword')),
                         ...))

        self.assertEqual(self.signature(partial(foo, 1, c=3)),
                         ((('b', ..., ..., 'positional_only'),
                           ('c', 3, ..., 'keyword_only'),
                           ('d', ..., ..., 'keyword_only'),
                           ('kwargs', ..., ..., 'var_keyword')),
                         ...))

    def test_signature_on_partialmethod(self):
        from functools import partialmethod

        class Spam:
            def test():
                pass
            ham = partialmethod(test)

        with self.assertRaisesRegex(ValueError, "has incorrect arguments"):
            inspect.signature(Spam.ham)

        class Spam:
            def test(it, a, *, c) -> 'spam':
                pass
            ham = partialmethod(test, c=1)

        self.assertEqual(self.signature(Spam.ham, eval_str=False),
                         ((('it', ..., ..., 'positional_or_keyword'),
                           ('a', ..., ..., 'positional_or_keyword'),
                           ('c', 1, ..., 'keyword_only')),
                          'spam'))

        self.assertEqual(self.signature(Spam().ham, eval_str=False),
                         ((('a', ..., ..., 'positional_or_keyword'),
                           ('c', 1, ..., 'keyword_only')),
                          'spam'))

        class Spam:
            def test(self: 'anno', x):
                pass

            g = partialmethod(test, 1)

        self.assertEqual(self.signature(Spam.g, eval_str=False),
                         ((('self', ..., 'anno', 'positional_or_keyword'),),
                          ...))

    def test_signature_on_fake_partialmethod(self):
        def foo(a): pass
        foo._partialmethod = 'spam'
        self.assertEqual(str(inspect.signature(foo)), '(a)')

    def test_signature_on_decorated(self):
        import functools

        def decorator(func):
            @functools.wraps(func)
            def wrapper(*args, **kwargs) -> int:
                return func(*args, **kwargs)
            return wrapper

        class Foo:
            @decorator
            def bar(self, a, b):
                pass

        self.assertEqual(self.signature(Foo.bar),
                         ((('self', ..., ..., "positional_or_keyword"),
                           ('a', ..., ..., "positional_or_keyword"),
                           ('b', ..., ..., "positional_or_keyword")),
                          ...))

        self.assertEqual(self.signature(Foo().bar),
                         ((('a', ..., ..., "positional_or_keyword"),
                           ('b', ..., ..., "positional_or_keyword")),
                          ...))

        self.assertEqual(self.signature(Foo.bar, follow_wrapped=False),
                         ((('args', ..., ..., "var_positional"),
                           ('kwargs', ..., ..., "var_keyword")),
                          ...)) # functools.wraps will copy __annotations__
                                # from "func" to "wrapper", hence no
                                # return_annotation

        # Test that we handle method wrappers correctly
        def decorator(func):
            @functools.wraps(func)
            def wrapper(*args, **kwargs) -> int:
                return func(42, *args, **kwargs)
            sig = inspect.signature(func)
            new_params = tuple(sig.parameters.values())[1:]
            wrapper.__signature__ = sig.replace(parameters=new_params)
            return wrapper

        class Foo:
            @decorator
            def __call__(self, a, b):
                pass

        self.assertEqual(self.signature(Foo.__call__),
                         ((('a', ..., ..., "positional_or_keyword"),
                           ('b', ..., ..., "positional_or_keyword")),
                          ...))

        self.assertEqual(self.signature(Foo().__call__),
                         ((('b', ..., ..., "positional_or_keyword"),),
                          ...))

        # Test we handle __signature__ partway down the wrapper stack
        def wrapped_foo_call():
            pass
        wrapped_foo_call.__wrapped__ = Foo.__call__

        self.assertEqual(self.signature(wrapped_foo_call),
                         ((('a', ..., ..., "positional_or_keyword"),
                           ('b', ..., ..., "positional_or_keyword")),
                          ...))


    def test_signature_on_class(self):
        class C:
            def __init__(self, a):
                pass

        self.assertEqual(self.signature(C),
                         ((('a', ..., ..., "positional_or_keyword"),),
                          ...))

        class CM(type):
            def __call__(cls, a):
                pass
        class C(metaclass=CM):
            def __init__(self, b):
                pass

        self.assertEqual(self.signature(C),
                         ((('a', ..., ..., "positional_or_keyword"),),
                          ...))

        class CM(type):
            def __new__(mcls, name, bases, dct, *, foo=1):
                return super().__new__(mcls, name, bases, dct)
        class C(metaclass=CM):
            def __init__(self, b):
                pass

        self.assertEqual(self.signature(C),
                         ((('b', ..., ..., "positional_or_keyword"),),
                          ...))

        self.assertEqual(self.signature(CM),
                         ((('name', ..., ..., "positional_or_keyword"),
                           ('bases', ..., ..., "positional_or_keyword"),
                           ('dct', ..., ..., "positional_or_keyword"),
                           ('foo', 1, ..., "keyword_only")),
                          ...))

        class CMM(type):
            def __new__(mcls, name, bases, dct, *, foo=1):
                return super().__new__(mcls, name, bases, dct)
            def __call__(cls, nm, bs, dt):
                return type(nm, bs, dt)
        class CM(type, metaclass=CMM):
            def __new__(mcls, name, bases, dct, *, bar=2):
                return super().__new__(mcls, name, bases, dct)
        class C(metaclass=CM):
            def __init__(self, b):
                pass

        self.assertEqual(self.signature(CMM),
                         ((('name', ..., ..., "positional_or_keyword"),
                           ('bases', ..., ..., "positional_or_keyword"),
                           ('dct', ..., ..., "positional_or_keyword"),
                           ('foo', 1, ..., "keyword_only")),
                          ...))

        self.assertEqual(self.signature(CM),
                         ((('nm', ..., ..., "positional_or_keyword"),
                           ('bs', ..., ..., "positional_or_keyword"),
                           ('dt', ..., ..., "positional_or_keyword")),
                          ...))

        self.assertEqual(self.signature(C),
                         ((('b', ..., ..., "positional_or_keyword"),),
                          ...))

        class CM(type):
            def __init__(cls, name, bases, dct, *, bar=2):
                return super().__init__(name, bases, dct)
        class C(metaclass=CM):
            def __init__(self, b):
                pass

        self.assertEqual(self.signature(CM),
                         ((('name', ..., ..., "positional_or_keyword"),
                           ('bases', ..., ..., "positional_or_keyword"),
                           ('dct', ..., ..., "positional_or_keyword"),
                           ('bar', 2, ..., "keyword_only")),
                          ...))

    def test_signature_on_subclass(self):
        class A:
            def __new__(cls, a=1, *args, **kwargs):
                return object.__new__(cls)
        class B(A):
            def __init__(self, b):
                pass
        class C(A):
            def __new__(cls, a=1, b=2, *args, **kwargs):
                return object.__new__(cls)
        class D(A):
            pass

        self.assertEqual(self.signature(B),
                         ((('b', ..., ..., "positional_or_keyword"),),
                          ...))
        self.assertEqual(self.signature(C),
                         ((('a', 1, ..., 'positional_or_keyword'),
                           ('b', 2, ..., 'positional_or_keyword'),
                           ('args', ..., ..., 'var_positional'),
                           ('kwargs', ..., ..., 'var_keyword')),
                          ...))
        self.assertEqual(self.signature(D),
                         ((('a', 1, ..., 'positional_or_keyword'),
                           ('args', ..., ..., 'var_positional'),
                           ('kwargs', ..., ..., 'var_keyword')),
                          ...))

    def test_signature_on_generic_subclass(self):
        from typing import Generic, TypeVar

        T = TypeVar('T')

        class A(Generic[T]):
            def __init__(self, *, a: int) -> None:
                pass

        self.assertEqual(self.signature(A),
                         ((('a', ..., int, 'keyword_only'),),
                          None))

    @unittest.skipIf(MISSING_C_DOCSTRINGS,
                     "Signature information for builtins requires docstrings")
    def test_signature_on_class_without_init(self):
        # Test classes without user-defined __init__ or __new__
        class C: pass
        self.assertEqual(str(inspect.signature(C)), '()')
        class D(C): pass
        self.assertEqual(str(inspect.signature(D)), '()')

        # Test meta-classes without user-defined __init__ or __new__
        class C(type): pass
        class D(C): pass
        with self.assertRaisesRegex(ValueError, "callable.*is not supported"):
            self.assertEqual(inspect.signature(C), None)
        with self.assertRaisesRegex(ValueError, "callable.*is not supported"):
            self.assertEqual(inspect.signature(D), None)

    @unittest.skipIf(MISSING_C_DOCSTRINGS,
                     "Signature information for builtins requires docstrings")
    def test_signature_on_builtin_class(self):
        expected = ('(file, protocol=None, fix_imports=True, '
                    'buffer_callback=None)')
        self.assertEqual(str(inspect.signature(_pickle.Pickler)), expected)

        class P(_pickle.Pickler): pass
        class EmptyTrait: pass
        class P2(EmptyTrait, P): pass
        self.assertEqual(str(inspect.signature(P)), expected)
        self.assertEqual(str(inspect.signature(P2)), expected)

        class P3(P2):
            def __init__(self, spam):
                pass
        self.assertEqual(str(inspect.signature(P3)), '(spam)')

        class MetaP(type):
            def __call__(cls, foo, bar):
                pass
        class P4(P2, metaclass=MetaP):
            pass
        self.assertEqual(str(inspect.signature(P4)), '(foo, bar)')

    def test_signature_on_callable_objects(self):
        class Foo:
            def __call__(self, a):
                pass

        self.assertEqual(self.signature(Foo()),
                         ((('a', ..., ..., "positional_or_keyword"),),
                          ...))

        class Spam:
            pass
        with self.assertRaisesRegex(TypeError, "is not a callable object"):
            inspect.signature(Spam())

        class Bar(Spam, Foo):
            pass

        self.assertEqual(self.signature(Bar()),
                         ((('a', ..., ..., "positional_or_keyword"),),
                          ...))

        class Wrapped:
            pass
        Wrapped.__wrapped__ = lambda a: None
        self.assertEqual(self.signature(Wrapped),
                         ((('a', ..., ..., "positional_or_keyword"),),
                          ...))
        # wrapper loop:
        Wrapped.__wrapped__ = Wrapped
        with self.assertRaisesRegex(ValueError, 'wrapper loop'):
            self.signature(Wrapped)

    def test_signature_on_lambdas(self):
        self.assertEqual(self.signature((lambda a=10: a)),
                         ((('a', 10, ..., "positional_or_keyword"),),
                          ...))

    def test_signature_equality(self):
        def foo(a, *, b:int) -> float: pass
        self.assertFalse(inspect.signature(foo) == 42)
        self.assertTrue(inspect.signature(foo) != 42)
        self.assertTrue(inspect.signature(foo) == ALWAYS_EQ)
        self.assertFalse(inspect.signature(foo) != ALWAYS_EQ)

        def bar(a, *, b:int) -> float: pass
        self.assertTrue(inspect.signature(foo) == inspect.signature(bar))
        self.assertFalse(inspect.signature(foo) != inspect.signature(bar))
        self.assertEqual(
            hash(inspect.signature(foo)), hash(inspect.signature(bar)))

        def bar(a, *, b:int) -> int: pass
        self.assertFalse(inspect.signature(foo) == inspect.signature(bar))
        self.assertTrue(inspect.signature(foo) != inspect.signature(bar))
        self.assertNotEqual(
            hash(inspect.signature(foo)), hash(inspect.signature(bar)))

        def bar(a, *, b:int): pass
        self.assertFalse(inspect.signature(foo) == inspect.signature(bar))
        self.assertTrue(inspect.signature(foo) != inspect.signature(bar))
        self.assertNotEqual(
            hash(inspect.signature(foo)), hash(inspect.signature(bar)))

        def bar(a, *, b:int=42) -> float: pass
        self.assertFalse(inspect.signature(foo) == inspect.signature(bar))
        self.assertTrue(inspect.signature(foo) != inspect.signature(bar))
        self.assertNotEqual(
            hash(inspect.signature(foo)), hash(inspect.signature(bar)))

        def bar(a, *, c) -> float: pass
        self.assertFalse(inspect.signature(foo) == inspect.signature(bar))
        self.assertTrue(inspect.signature(foo) != inspect.signature(bar))
        self.assertNotEqual(
            hash(inspect.signature(foo)), hash(inspect.signature(bar)))

        def bar(a, b:int) -> float: pass
        self.assertFalse(inspect.signature(foo) == inspect.signature(bar))
        self.assertTrue(inspect.signature(foo) != inspect.signature(bar))
        self.assertNotEqual(
            hash(inspect.signature(foo)), hash(inspect.signature(bar)))
        def spam(b:int, a) -> float: pass
        self.assertFalse(inspect.signature(spam) == inspect.signature(bar))
        self.assertTrue(inspect.signature(spam) != inspect.signature(bar))
        self.assertNotEqual(
            hash(inspect.signature(spam)), hash(inspect.signature(bar)))

        def foo(*, a, b, c): pass
        def bar(*, c, b, a): pass
        self.assertTrue(inspect.signature(foo) == inspect.signature(bar))
        self.assertFalse(inspect.signature(foo) != inspect.signature(bar))
        self.assertEqual(
            hash(inspect.signature(foo)), hash(inspect.signature(bar)))

        def foo(*, a=1, b, c): pass
        def bar(*, c, b, a=1): pass
        self.assertTrue(inspect.signature(foo) == inspect.signature(bar))
        self.assertFalse(inspect.signature(foo) != inspect.signature(bar))
        self.assertEqual(
            hash(inspect.signature(foo)), hash(inspect.signature(bar)))

        def foo(pos, *, a=1, b, c): pass
        def bar(pos, *, c, b, a=1): pass
        self.assertTrue(inspect.signature(foo) == inspect.signature(bar))
        self.assertFalse(inspect.signature(foo) != inspect.signature(bar))
        self.assertEqual(
            hash(inspect.signature(foo)), hash(inspect.signature(bar)))

        def foo(pos, *, a, b, c): pass
        def bar(pos, *, c, b, a=1): pass
        self.assertFalse(inspect.signature(foo) == inspect.signature(bar))
        self.assertTrue(inspect.signature(foo) != inspect.signature(bar))
        self.assertNotEqual(
            hash(inspect.signature(foo)), hash(inspect.signature(bar)))

        def foo(pos, *args, a=42, b, c, **kwargs:int): pass
        def bar(pos, *args, c, b, a=42, **kwargs:int): pass
        self.assertTrue(inspect.signature(foo) == inspect.signature(bar))
        self.assertFalse(inspect.signature(foo) != inspect.signature(bar))
        self.assertEqual(
            hash(inspect.signature(foo)), hash(inspect.signature(bar)))

    def test_signature_hashable(self):
        S = inspect.Signature
        P = inspect.Parameter

        def foo(a): pass
        foo_sig = inspect.signature(foo)

        manual_sig = S(parameters=[P('a', P.POSITIONAL_OR_KEYWORD)])

        self.assertEqual(hash(foo_sig), hash(manual_sig))
        self.assertNotEqual(hash(foo_sig),
                            hash(manual_sig.replace(return_annotation='spam')))

        def bar(a) -> 1: pass
        self.assertNotEqual(hash(foo_sig), hash(inspect.signature(bar)))

        def foo(a={}): pass
        with self.assertRaisesRegex(TypeError, 'unhashable type'):
            hash(inspect.signature(foo))

        def foo(a) -> {}: pass
        with self.assertRaisesRegex(TypeError, 'unhashable type'):
            hash(inspect.signature(foo))

    def test_signature_str(self):
        def foo(a:int=1, *, b, c=None, **kwargs) -> 42:
            pass
        self.assertEqual(str(inspect.signature(foo)),
                         '(a: int = 1, *, b, c=None, **kwargs) -> 42')

        def foo(a:int=1, *args, b, c=None, **kwargs) -> 42:
            pass
        self.assertEqual(str(inspect.signature(foo)),
                         '(a: int = 1, *args, b, c=None, **kwargs) -> 42')

        def foo():
            pass
        self.assertEqual(str(inspect.signature(foo)), '()')

    def test_signature_str_positional_only(self):
        P = inspect.Parameter
        S = inspect.Signature

        def test(a_po, *, b, **kwargs):
            return a_po, kwargs

        sig = inspect.signature(test)
        new_params = list(sig.parameters.values())
        new_params[0] = new_params[0].replace(kind=P.POSITIONAL_ONLY)
        test.__signature__ = sig.replace(parameters=new_params)

        self.assertEqual(str(inspect.signature(test)),
                         '(a_po, /, *, b, **kwargs)')

        self.assertEqual(str(S(parameters=[P('foo', P.POSITIONAL_ONLY)])),
                         '(foo, /)')

        self.assertEqual(str(S(parameters=[
                                P('foo', P.POSITIONAL_ONLY),
                                P('bar', P.VAR_KEYWORD)])),
                         '(foo, /, **bar)')

        self.assertEqual(str(S(parameters=[
                                P('foo', P.POSITIONAL_ONLY),
                                P('bar', P.VAR_POSITIONAL)])),
                         '(foo, /, *bar)')

    def test_signature_replace_anno(self):
        def test() -> 42:
            pass

        sig = inspect.signature(test)
        sig = sig.replace(return_annotation=None)
        self.assertIs(sig.return_annotation, None)
        sig = sig.replace(return_annotation=sig.empty)
        self.assertIs(sig.return_annotation, sig.empty)
        sig = sig.replace(return_annotation=42)
        self.assertEqual(sig.return_annotation, 42)
        self.assertEqual(sig, inspect.signature(test))

    def test_signature_on_mangled_parameters(self):
        class Spam:
            def foo(self, __p1:1=2, *, __p2:2=3):
                pass
        class Ham(Spam):
            pass

        self.assertEqual(self.signature(Spam.foo),
                         ((('self', ..., ..., "positional_or_keyword"),
                           ('_Spam__p1', 2, 1, "positional_or_keyword"),
                           ('_Spam__p2', 3, 2, "keyword_only")),
                          ...))

        self.assertEqual(self.signature(Spam.foo),
                         self.signature(Ham.foo))

    def test_signature_from_callable_python_obj(self):
        class MySignature(inspect.Signature): pass
        def foo(a, *, b:1): pass
        foo_sig = MySignature.from_callable(foo)
        self.assertIsInstance(foo_sig, MySignature)

    def test_signature_from_callable_class(self):
        # A regression test for a class inheriting its signature from `object`.
        class MySignature(inspect.Signature): pass
        class foo: pass
        foo_sig = MySignature.from_callable(foo)
        self.assertIsInstance(foo_sig, MySignature)

    @unittest.skipIf(MISSING_C_DOCSTRINGS,
                     "Signature information for builtins requires docstrings")
    def test_signature_from_callable_builtin_obj(self):
        class MySignature(inspect.Signature): pass
        sig = MySignature.from_callable(_pickle.Pickler)
        self.assertIsInstance(sig, MySignature)

    def test_signature_definition_order_preserved_on_kwonly(self):
        for fn in signatures_with_lexicographic_keyword_only_parameters():
            signature = inspect.signature(fn)
            l = list(signature.parameters)
            sorted_l = sorted(l)
            self.assertTrue(l)
            self.assertEqual(l, sorted_l)
        signature = inspect.signature(unsorted_keyword_only_parameters_fn)
        l = list(signature.parameters)
        self.assertEqual(l, unsorted_keyword_only_parameters)

    def test_signater_parameters_is_ordered(self):
        p1 = inspect.signature(lambda x, y: None).parameters
        p2 = inspect.signature(lambda y, x: None).parameters
        self.assertNotEqual(p1, p2)

    def test_signature_annotations_with_local_namespaces(self):
        class Foo: ...
        def func(foo: Foo) -> int: pass
        def func2(foo: Foo, bar: 'Bar') -> int: pass

        for signature_func in (inspect.signature, inspect.Signature.from_callable):
            with self.subTest(signature_func = signature_func):
                sig1 = signature_func(func)
                self.assertEqual(sig1.return_annotation, int)
                self.assertEqual(sig1.parameters['foo'].annotation, Foo)

                sig2 = signature_func(func, locals=locals())
                self.assertEqual(sig2.return_annotation, int)
                self.assertEqual(sig2.parameters['foo'].annotation, Foo)

                sig3 = signature_func(func2, globals={'Bar': int}, locals=locals())
                self.assertEqual(sig3.return_annotation, int)
                self.assertEqual(sig3.parameters['foo'].annotation, Foo)
                self.assertEqual(sig3.parameters['bar'].annotation, 'Bar')

    def test_signature_eval_str(self):
        isa = inspect_stringized_annotations
        sig = inspect.Signature
        par = inspect.Parameter
        PORK = inspect.Parameter.POSITIONAL_OR_KEYWORD
        for signature_func in (inspect.signature, inspect.Signature.from_callable):
            with self.subTest(signature_func = signature_func):
                self.assertEqual(
                    signature_func(isa.MyClass),
                    sig(
                        parameters=(
                            par('a', PORK),
                            par('b', PORK),
                        )))
                self.assertEqual(
                    signature_func(isa.function),
                    sig(
                        return_annotation='MyClass',
                        parameters=(
                            par('a', PORK, annotation='int'),
                            par('b', PORK, annotation='str'),
                        )))
                self.assertEqual(
                    signature_func(isa.function2),
                    sig(
                        return_annotation='MyClass',
                        parameters=(
                            par('a', PORK, annotation='int'),
                            par('b', PORK, annotation="'str'"),
                            par('c', PORK, annotation="MyClass"),
                        )))
                self.assertEqual(
                    signature_func(isa.function3),
                    sig(
                        parameters=(
                            par('a', PORK, annotation="'int'"),
                            par('b', PORK, annotation="'str'"),
                            par('c', PORK, annotation="'MyClass'"),
                        )))

                self.assertEqual(signature_func(isa.UnannotatedClass), sig())
                self.assertEqual(signature_func(isa.unannotated_function),
                    sig(
                        parameters=(
                            par('a', PORK),
                            par('b', PORK),
                            par('c', PORK),
                        )))

                self.assertEqual(
                    signature_func(isa.MyClass, eval_str=True),
                    sig(
                        parameters=(
                            par('a', PORK),
                            par('b', PORK),
                        )))
                self.assertEqual(
                    signature_func(isa.function, eval_str=True),
                    sig(
                        return_annotation=isa.MyClass,
                        parameters=(
                            par('a', PORK, annotation=int),
                            par('b', PORK, annotation=str),
                        )))
                self.assertEqual(
                    signature_func(isa.function2, eval_str=True),
                    sig(
                        return_annotation=isa.MyClass,
                        parameters=(
                            par('a', PORK, annotation=int),
                            par('b', PORK, annotation='str'),
                            par('c', PORK, annotation=isa.MyClass),
                        )))
                self.assertEqual(
                    signature_func(isa.function3, eval_str=True),
                    sig(
                        parameters=(
                            par('a', PORK, annotation='int'),
                            par('b', PORK, annotation='str'),
                            par('c', PORK, annotation='MyClass'),
                        )))

                globalns = {'int': float, 'str': complex}
                localns = {'str': tuple, 'MyClass': dict}
                with self.assertRaises(NameError):
                    signature_func(isa.function, eval_str=True, globals=globalns)

                self.assertEqual(
                    signature_func(isa.function, eval_str=True, locals=localns),
                    sig(
                        return_annotation=dict,
                        parameters=(
                            par('a', PORK, annotation=int),
                            par('b', PORK, annotation=tuple),
                        )))

                self.assertEqual(
                    signature_func(isa.function, eval_str=True, globals=globalns, locals=localns),
                    sig(
                        return_annotation=dict,
                        parameters=(
                            par('a', PORK, annotation=float),
                            par('b', PORK, annotation=tuple),
                        )))

    def test_signature_none_annotation(self):
        class funclike:
            # Has to be callable, and have correct
            # __code__, __annotations__, __defaults__, __name__,
            # and __kwdefaults__ attributes

            def __init__(self, func):
                self.__name__ = func.__name__
                self.__code__ = func.__code__
                self.__annotations__ = func.__annotations__
                self.__defaults__ = func.__defaults__
                self.__kwdefaults__ = func.__kwdefaults__
                self.func = func

            def __call__(self, *args, **kwargs):
                return self.func(*args, **kwargs)

        def foo(): pass
        foo = funclike(foo)
        foo.__annotations__ = None
        for signature_func in (inspect.signature, inspect.Signature.from_callable):
            with self.subTest(signature_func = signature_func):
                self.assertEqual(signature_func(foo), inspect.Signature())
        self.assertEqual(inspect.get_annotations(foo), {})


class TestParameterObject(unittest.TestCase):
    def test_signature_parameter_kinds(self):
        P = inspect.Parameter
        self.assertTrue(P.POSITIONAL_ONLY < P.POSITIONAL_OR_KEYWORD < \
                        P.VAR_POSITIONAL < P.KEYWORD_ONLY < P.VAR_KEYWORD)

        self.assertEqual(str(P.POSITIONAL_ONLY), 'POSITIONAL_ONLY')
        self.assertTrue('POSITIONAL_ONLY' in repr(P.POSITIONAL_ONLY))

    def test_signature_parameter_object(self):
        p = inspect.Parameter('foo', default=10,
                              kind=inspect.Parameter.POSITIONAL_ONLY)
        self.assertEqual(p.name, 'foo')
        self.assertEqual(p.default, 10)
        self.assertIs(p.annotation, p.empty)
        self.assertEqual(p.kind, inspect.Parameter.POSITIONAL_ONLY)

        with self.assertRaisesRegex(ValueError, "value '123' is "
                                    "not a valid Parameter.kind"):
            inspect.Parameter('foo', default=10, kind='123')

        with self.assertRaisesRegex(ValueError, 'not a valid parameter name'):
            inspect.Parameter('1', kind=inspect.Parameter.VAR_KEYWORD)

        with self.assertRaisesRegex(TypeError, 'name must be a str'):
            inspect.Parameter(None, kind=inspect.Parameter.VAR_KEYWORD)

        with self.assertRaisesRegex(ValueError,
                                    'is not a valid parameter name'):
            inspect.Parameter('$', kind=inspect.Parameter.VAR_KEYWORD)

        with self.assertRaisesRegex(ValueError,
                                    'is not a valid parameter name'):
            inspect.Parameter('.a', kind=inspect.Parameter.VAR_KEYWORD)

        with self.assertRaisesRegex(ValueError, 'cannot have default values'):
            inspect.Parameter('a', default=42,
                              kind=inspect.Parameter.VAR_KEYWORD)

        with self.assertRaisesRegex(ValueError, 'cannot have default values'):
            inspect.Parameter('a', default=42,
                              kind=inspect.Parameter.VAR_POSITIONAL)

        p = inspect.Parameter('a', default=42,
                              kind=inspect.Parameter.POSITIONAL_OR_KEYWORD)
        with self.assertRaisesRegex(ValueError, 'cannot have default values'):
            p.replace(kind=inspect.Parameter.VAR_POSITIONAL)

        self.assertTrue(repr(p).startswith('<Parameter'))
        self.assertTrue('"a=42"' in repr(p))

    def test_signature_parameter_hashable(self):
        P = inspect.Parameter
        foo = P('foo', kind=P.POSITIONAL_ONLY)
        self.assertEqual(hash(foo), hash(P('foo', kind=P.POSITIONAL_ONLY)))
        self.assertNotEqual(hash(foo), hash(P('foo', kind=P.POSITIONAL_ONLY,
                                              default=42)))
        self.assertNotEqual(hash(foo),
                            hash(foo.replace(kind=P.VAR_POSITIONAL)))

    def test_signature_parameter_equality(self):
        P = inspect.Parameter
        p = P('foo', default=42, kind=inspect.Parameter.KEYWORD_ONLY)

        self.assertTrue(p == p)
        self.assertFalse(p != p)
        self.assertFalse(p == 42)
        self.assertTrue(p != 42)
        self.assertTrue(p == ALWAYS_EQ)
        self.assertFalse(p != ALWAYS_EQ)

        self.assertTrue(p == P('foo', default=42,
                               kind=inspect.Parameter.KEYWORD_ONLY))
        self.assertFalse(p != P('foo', default=42,
                                kind=inspect.Parameter.KEYWORD_ONLY))

    def test_signature_parameter_replace(self):
        p = inspect.Parameter('foo', default=42,
                              kind=inspect.Parameter.KEYWORD_ONLY)

        self.assertIsNot(p, p.replace())
        self.assertEqual(p, p.replace())

        p2 = p.replace(annotation=1)
        self.assertEqual(p2.annotation, 1)
        p2 = p2.replace(annotation=p2.empty)
        self.assertEqual(p, p2)

        p2 = p2.replace(name='bar')
        self.assertEqual(p2.name, 'bar')
        self.assertNotEqual(p2, p)

        with self.assertRaisesRegex(ValueError,
                                    'name is a required attribute'):
            p2 = p2.replace(name=p2.empty)

        p2 = p2.replace(name='foo', default=None)
        self.assertIs(p2.default, None)
        self.assertNotEqual(p2, p)

        p2 = p2.replace(name='foo', default=p2.empty)
        self.assertIs(p2.default, p2.empty)


        p2 = p2.replace(default=42, kind=p2.POSITIONAL_OR_KEYWORD)
        self.assertEqual(p2.kind, p2.POSITIONAL_OR_KEYWORD)
        self.assertNotEqual(p2, p)

        with self.assertRaisesRegex(ValueError,
                                    "value <class 'inspect._empty'> "
                                    "is not a valid Parameter.kind"):
            p2 = p2.replace(kind=p2.empty)

        p2 = p2.replace(kind=p2.KEYWORD_ONLY)
        self.assertEqual(p2, p)

    def test_signature_parameter_positional_only(self):
        with self.assertRaisesRegex(TypeError, 'name must be a str'):
            inspect.Parameter(None, kind=inspect.Parameter.POSITIONAL_ONLY)

    @cpython_only
    def test_signature_parameter_implicit(self):
        with self.assertRaisesRegex(ValueError,
                                    'implicit arguments must be passed as '
                                    'positional or keyword arguments, '
                                    'not positional-only'):
            inspect.Parameter('.0', kind=inspect.Parameter.POSITIONAL_ONLY)

        param = inspect.Parameter(
            '.0', kind=inspect.Parameter.POSITIONAL_OR_KEYWORD)
        self.assertEqual(param.kind, inspect.Parameter.POSITIONAL_ONLY)
        self.assertEqual(param.name, 'implicit0')

    def test_signature_parameter_immutability(self):
        p = inspect.Parameter('spam', kind=inspect.Parameter.KEYWORD_ONLY)

        with self.assertRaises(AttributeError):
            p.foo = 'bar'

        with self.assertRaises(AttributeError):
            p.kind = 123


class TestSignatureBind(unittest.TestCase):
    @staticmethod
    def call(func, *args, **kwargs):
        sig = inspect.signature(func)
        ba = sig.bind(*args, **kwargs)
        return func(*ba.args, **ba.kwargs)

    def test_signature_bind_empty(self):
        def test():
            return 42

        self.assertEqual(self.call(test), 42)
        with self.assertRaisesRegex(TypeError, 'too many positional arguments'):
            self.call(test, 1)
        with self.assertRaisesRegex(TypeError, 'too many positional arguments'):
            self.call(test, 1, spam=10)
        with self.assertRaisesRegex(
            TypeError, "got an unexpected keyword argument 'spam'"):

            self.call(test, spam=1)

    def test_signature_bind_var(self):
        def test(*args, **kwargs):
            return args, kwargs

        self.assertEqual(self.call(test), ((), {}))
        self.assertEqual(self.call(test, 1), ((1,), {}))
        self.assertEqual(self.call(test, 1, 2), ((1, 2), {}))
        self.assertEqual(self.call(test, foo='bar'), ((), {'foo': 'bar'}))
        self.assertEqual(self.call(test, 1, foo='bar'), ((1,), {'foo': 'bar'}))
        self.assertEqual(self.call(test, args=10), ((), {'args': 10}))
        self.assertEqual(self.call(test, 1, 2, foo='bar'),
                         ((1, 2), {'foo': 'bar'}))

    def test_signature_bind_just_args(self):
        def test(a, b, c):
            return a, b, c

        self.assertEqual(self.call(test, 1, 2, 3), (1, 2, 3))

        with self.assertRaisesRegex(TypeError, 'too many positional arguments'):
            self.call(test, 1, 2, 3, 4)

        with self.assertRaisesRegex(TypeError,
                                    "missing a required argument: 'b'"):
            self.call(test, 1)

        with self.assertRaisesRegex(TypeError,
                                    "missing a required argument: 'a'"):
            self.call(test)

        def test(a, b, c=10):
            return a, b, c
        self.assertEqual(self.call(test, 1, 2, 3), (1, 2, 3))
        self.assertEqual(self.call(test, 1, 2), (1, 2, 10))

        def test(a=1, b=2, c=3):
            return a, b, c
        self.assertEqual(self.call(test, a=10, c=13), (10, 2, 13))
        self.assertEqual(self.call(test, a=10), (10, 2, 3))
        self.assertEqual(self.call(test, b=10), (1, 10, 3))

    def test_signature_bind_varargs_order(self):
        def test(*args):
            return args

        self.assertEqual(self.call(test), ())
        self.assertEqual(self.call(test, 1, 2, 3), (1, 2, 3))

    def test_signature_bind_args_and_varargs(self):
        def test(a, b, c=3, *args):
            return a, b, c, args

        self.assertEqual(self.call(test, 1, 2, 3, 4, 5), (1, 2, 3, (4, 5)))
        self.assertEqual(self.call(test, 1, 2), (1, 2, 3, ()))
        self.assertEqual(self.call(test, b=1, a=2), (2, 1, 3, ()))
        self.assertEqual(self.call(test, 1, b=2), (1, 2, 3, ()))

        with self.assertRaisesRegex(TypeError,
                                     "multiple values for argument 'c'"):
            self.call(test, 1, 2, 3, c=4)

    def test_signature_bind_just_kwargs(self):
        def test(**kwargs):
            return kwargs

        self.assertEqual(self.call(test), {})
        self.assertEqual(self.call(test, foo='bar', spam='ham'),
                         {'foo': 'bar', 'spam': 'ham'})

    def test_signature_bind_args_and_kwargs(self):
        def test(a, b, c=3, **kwargs):
            return a, b, c, kwargs

        self.assertEqual(self.call(test, 1, 2), (1, 2, 3, {}))
        self.assertEqual(self.call(test, 1, 2, foo='bar', spam='ham'),
                         (1, 2, 3, {'foo': 'bar', 'spam': 'ham'}))
        self.assertEqual(self.call(test, b=2, a=1, foo='bar', spam='ham'),
                         (1, 2, 3, {'foo': 'bar', 'spam': 'ham'}))
        self.assertEqual(self.call(test, a=1, b=2, foo='bar', spam='ham'),
                         (1, 2, 3, {'foo': 'bar', 'spam': 'ham'}))
        self.assertEqual(self.call(test, 1, b=2, foo='bar', spam='ham'),
                         (1, 2, 3, {'foo': 'bar', 'spam': 'ham'}))
        self.assertEqual(self.call(test, 1, b=2, c=4, foo='bar', spam='ham'),
                         (1, 2, 4, {'foo': 'bar', 'spam': 'ham'}))
        self.assertEqual(self.call(test, 1, 2, 4, foo='bar'),
                         (1, 2, 4, {'foo': 'bar'}))
        self.assertEqual(self.call(test, c=5, a=4, b=3),
                         (4, 3, 5, {}))

    def test_signature_bind_kwonly(self):
        def test(*, foo):
            return foo
        with self.assertRaisesRegex(TypeError,
                                     'too many positional arguments'):
            self.call(test, 1)
        self.assertEqual(self.call(test, foo=1), 1)

        def test(a, *, foo=1, bar):
            return foo
        with self.assertRaisesRegex(TypeError,
                                     "missing a required argument: 'bar'"):
            self.call(test, 1)

        def test(foo, *, bar):
            return foo, bar
        self.assertEqual(self.call(test, 1, bar=2), (1, 2))
        self.assertEqual(self.call(test, bar=2, foo=1), (1, 2))

        with self.assertRaisesRegex(
            TypeError, "got an unexpected keyword argument 'spam'"):

            self.call(test, bar=2, foo=1, spam=10)

        with self.assertRaisesRegex(TypeError,
                                     'too many positional arguments'):
            self.call(test, 1, 2)

        with self.assertRaisesRegex(TypeError,
                                     'too many positional arguments'):
            self.call(test, 1, 2, bar=2)

        with self.assertRaisesRegex(
            TypeError, "got an unexpected keyword argument 'spam'"):

            self.call(test, 1, bar=2, spam='ham')

        with self.assertRaisesRegex(TypeError,
                                     "missing a required argument: 'bar'"):
            self.call(test, 1)

        def test(foo, *, bar, **bin):
            return foo, bar, bin
        self.assertEqual(self.call(test, 1, bar=2), (1, 2, {}))
        self.assertEqual(self.call(test, foo=1, bar=2), (1, 2, {}))
        self.assertEqual(self.call(test, 1, bar=2, spam='ham'),
                         (1, 2, {'spam': 'ham'}))
        self.assertEqual(self.call(test, spam='ham', foo=1, bar=2),
                         (1, 2, {'spam': 'ham'}))
        with self.assertRaisesRegex(TypeError,
                                    "missing a required argument: 'foo'"):
            self.call(test, spam='ham', bar=2)
        self.assertEqual(self.call(test, 1, bar=2, bin=1, spam=10),
                         (1, 2, {'bin': 1, 'spam': 10}))

    def test_signature_bind_arguments(self):
        def test(a, *args, b, z=100, **kwargs):
            pass
        sig = inspect.signature(test)
        ba = sig.bind(10, 20, b=30, c=40, args=50, kwargs=60)
        # we won't have 'z' argument in the bound arguments object, as we didn't
        # pass it to the 'bind'
        self.assertEqual(tuple(ba.arguments.items()),
                         (('a', 10), ('args', (20,)), ('b', 30),
                          ('kwargs', {'c': 40, 'args': 50, 'kwargs': 60})))
        self.assertEqual(ba.kwargs,
                         {'b': 30, 'c': 40, 'args': 50, 'kwargs': 60})
        self.assertEqual(ba.args, (10, 20))

    def test_signature_bind_positional_only(self):
        P = inspect.Parameter

        def test(a_po, b_po, c_po=3, foo=42, *, bar=50, **kwargs):
            return a_po, b_po, c_po, foo, bar, kwargs

        sig = inspect.signature(test)
        new_params = collections.OrderedDict(tuple(sig.parameters.items()))
        for name in ('a_po', 'b_po', 'c_po'):
            new_params[name] = new_params[name].replace(kind=P.POSITIONAL_ONLY)
        new_sig = sig.replace(parameters=new_params.values())
        test.__signature__ = new_sig

        self.assertEqual(self.call(test, 1, 2, 4, 5, bar=6),
                         (1, 2, 4, 5, 6, {}))

        self.assertEqual(self.call(test, 1, 2),
                         (1, 2, 3, 42, 50, {}))

        self.assertEqual(self.call(test, 1, 2, foo=4, bar=5),
                         (1, 2, 3, 4, 5, {}))

        with self.assertRaisesRegex(TypeError, "but was passed as a keyword"):
            self.call(test, 1, 2, foo=4, bar=5, c_po=10)

        with self.assertRaisesRegex(TypeError, "parameter is positional only"):
            self.call(test, 1, 2, c_po=4)

        with self.assertRaisesRegex(TypeError, "parameter is positional only"):
            self.call(test, a_po=1, b_po=2)

    def test_signature_bind_with_self_arg(self):
        # Issue #17071: one of the parameters is named "self
        def test(a, self, b):
            pass
        sig = inspect.signature(test)
        ba = sig.bind(1, 2, 3)
        self.assertEqual(ba.args, (1, 2, 3))
        ba = sig.bind(1, self=2, b=3)
        self.assertEqual(ba.args, (1, 2, 3))

    def test_signature_bind_vararg_name(self):
        def test(a, *args):
            return a, args
        sig = inspect.signature(test)

        with self.assertRaisesRegex(
            TypeError, "got an unexpected keyword argument 'args'"):

            sig.bind(a=0, args=1)

        def test(*args, **kwargs):
            return args, kwargs
        self.assertEqual(self.call(test, args=1), ((), {'args': 1}))

        sig = inspect.signature(test)
        ba = sig.bind(args=1)
        self.assertEqual(ba.arguments, {'kwargs': {'args': 1}})

    @cpython_only
    def test_signature_bind_implicit_arg(self):
        # Issue #19611: getcallargs should work with set comprehensions
        def make_set():
            return {z * z for z in range(5)}
        setcomp_code = make_set.__code__.co_consts[1]
        setcomp_func = types.FunctionType(setcomp_code, {})

        iterator = iter(range(5))
        self.assertEqual(self.call(setcomp_func, iterator), {0, 1, 4, 9, 16})

    def test_signature_bind_posonly_kwargs(self):
        def foo(bar, /, **kwargs):
            return bar, kwargs.get(bar)

        sig = inspect.signature(foo)
        result = sig.bind("pos-only", bar="keyword")

        self.assertEqual(result.kwargs, {"bar": "keyword"})
        self.assertIn(("bar", "pos-only"), result.arguments.items())


class TestBoundArguments(unittest.TestCase):
    def test_signature_bound_arguments_unhashable(self):
        def foo(a): pass
        ba = inspect.signature(foo).bind(1)

        with self.assertRaisesRegex(TypeError, 'unhashable type'):
            hash(ba)

    def test_signature_bound_arguments_equality(self):
        def foo(a): pass
        ba = inspect.signature(foo).bind(1)
        self.assertTrue(ba == ba)
        self.assertFalse(ba != ba)
        self.assertTrue(ba == ALWAYS_EQ)
        self.assertFalse(ba != ALWAYS_EQ)

        ba2 = inspect.signature(foo).bind(1)
        self.assertTrue(ba == ba2)
        self.assertFalse(ba != ba2)

        ba3 = inspect.signature(foo).bind(2)
        self.assertFalse(ba == ba3)
        self.assertTrue(ba != ba3)
        ba3.arguments['a'] = 1
        self.assertTrue(ba == ba3)
        self.assertFalse(ba != ba3)

        def bar(b): pass
        ba4 = inspect.signature(bar).bind(1)
        self.assertFalse(ba == ba4)
        self.assertTrue(ba != ba4)

        def foo(*, a, b): pass
        sig = inspect.signature(foo)
        ba1 = sig.bind(a=1, b=2)
        ba2 = sig.bind(b=2, a=1)
        self.assertTrue(ba1 == ba2)
        self.assertFalse(ba1 != ba2)

    def test_signature_bound_arguments_pickle(self):
        def foo(a, b, *, c:1={}, **kw) -> {42:'ham'}: pass
        sig = inspect.signature(foo)
        ba = sig.bind(20, 30, z={})

        for ver in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.subTest(pickle_ver=ver):
                ba_pickled = pickle.loads(pickle.dumps(ba, ver))
                self.assertEqual(ba, ba_pickled)

    def test_signature_bound_arguments_repr(self):
        def foo(a, b, *, c:1={}, **kw) -> {42:'ham'}: pass
        sig = inspect.signature(foo)
        ba = sig.bind(20, 30, z={})
        self.assertRegex(repr(ba), r'<BoundArguments \(a=20,.*\}\}\)>')

    def test_signature_bound_arguments_apply_defaults(self):
        def foo(a, b=1, *args, c:1={}, **kw): pass
        sig = inspect.signature(foo)

        ba = sig.bind(20)
        ba.apply_defaults()
        self.assertEqual(
            list(ba.arguments.items()),
            [('a', 20), ('b', 1), ('args', ()), ('c', {}), ('kw', {})])

        # Make sure that we preserve the order:
        # i.e. 'c' should be *before* 'kw'.
        ba = sig.bind(10, 20, 30, d=1)
        ba.apply_defaults()
        self.assertEqual(
            list(ba.arguments.items()),
            [('a', 10), ('b', 20), ('args', (30,)), ('c', {}), ('kw', {'d':1})])

        # Make sure that BoundArguments produced by bind_partial()
        # are supported.
        def foo(a, b): pass
        sig = inspect.signature(foo)
        ba = sig.bind_partial(20)
        ba.apply_defaults()
        self.assertEqual(
            list(ba.arguments.items()),
            [('a', 20)])

        # Test no args
        def foo(): pass
        sig = inspect.signature(foo)
        ba = sig.bind()
        ba.apply_defaults()
        self.assertEqual(list(ba.arguments.items()), [])

        # Make sure a no-args binding still acquires proper defaults.
        def foo(a='spam'): pass
        sig = inspect.signature(foo)
        ba = sig.bind()
        ba.apply_defaults()
        self.assertEqual(list(ba.arguments.items()), [('a', 'spam')])

    def test_signature_bound_arguments_arguments_type(self):
        def foo(a): pass
        ba = inspect.signature(foo).bind(1)
        self.assertIs(type(ba.arguments), dict)

class TestSignaturePrivateHelpers(unittest.TestCase):
    def test_signature_get_bound_param(self):
        getter = inspect._signature_get_bound_param

        self.assertEqual(getter('($self)'), 'self')
        self.assertEqual(getter('($self, obj)'), 'self')
        self.assertEqual(getter('($cls, /, obj)'), 'cls')

    def _strip_non_python_syntax(self, input,
        clean_signature, self_parameter, last_positional_only):
        computed_clean_signature, \
            computed_self_parameter, \
            computed_last_positional_only = \
            inspect._signature_strip_non_python_syntax(input)
        self.assertEqual(computed_clean_signature, clean_signature)
        self.assertEqual(computed_self_parameter, self_parameter)
        self.assertEqual(computed_last_positional_only, last_positional_only)

    def test_signature_strip_non_python_syntax(self):
        self._strip_non_python_syntax(
            "($module, /, path, mode, *, dir_fd=None, " +
                "effective_ids=False,\n       follow_symlinks=True)",
            "(module, path, mode, *, dir_fd=None, " +
                "effective_ids=False, follow_symlinks=True)",
            0,
            0)

        self._strip_non_python_syntax(
            "($module, word, salt, /)",
            "(module, word, salt)",
            0,
            2)

        self._strip_non_python_syntax(
            "(x, y=None, z=None, /)",
            "(x, y=None, z=None)",
            None,
            2)

        self._strip_non_python_syntax(
            "(x, y=None, z=None)",
            "(x, y=None, z=None)",
            None,
            None)

        self._strip_non_python_syntax(
            "(x,\n    y=None,\n      z = None  )",
            "(x, y=None, z=None)",
            None,
            None)

        self._strip_non_python_syntax(
            "",
            "",
            None,
            None)

        self._strip_non_python_syntax(
            None,
            None,
            None,
            None)

class TestSignatureDefinitions(unittest.TestCase):
    # This test case provides a home for checking that particular APIs
    # have signatures available for introspection

    @cpython_only
    @unittest.skipIf(MISSING_C_DOCSTRINGS,
                     "Signature information for builtins requires docstrings")
    def test_builtins_have_signatures(self):
        # This checks all builtin callables in CPython have signatures
        # A few have signatures Signature can't yet handle, so we skip those
        # since they will have to wait until PEP 457 adds the required
        # introspection support to the inspect module
        # Some others also haven't been converted yet for various other
        # reasons, so we also skip those for the time being, but design
        # the test to fail in order to indicate when it needs to be
        # updated.
        no_signature = set()
        # These need PEP 457 groups
        needs_groups = {"range", "slice", "dir", "getattr",
                        "next", "iter", "vars"}
        no_signature |= needs_groups
        # These have unrepresentable parameter default values of NULL
        needs_null = {"anext"}
        no_signature |= needs_null
        # These need PEP 457 groups or a signature change to accept None
        needs_semantic_update = {"round"}
        no_signature |= needs_semantic_update
        # These need *args support in Argument Clinic
        needs_varargs = {"breakpoint", "min", "max", "print",
                         "__build_class__"}
        no_signature |= needs_varargs
        # These simply weren't covered in the initial AC conversion
        # for builtin callables
        not_converted_yet = {"open", "__import__"}
        no_signature |= not_converted_yet
        # These builtin types are expected to provide introspection info
        types_with_signatures = set()
        # Check the signatures we expect to be there
        ns = vars(builtins)
        for name, obj in sorted(ns.items()):
            if not callable(obj):
                continue
            # The builtin types haven't been converted to AC yet
            if isinstance(obj, type) and (name not in types_with_signatures):
                # Note that this also skips all the exception types
                no_signature.add(name)
            if (name in no_signature):
                # Not yet converted
                continue
            with self.subTest(builtin=name):
                self.assertIsNotNone(inspect.signature(obj))
        # Check callables that haven't been converted don't claim a signature
        # This ensures this test will start failing as more signatures are
        # added, so the affected items can be moved into the scope of the
        # regression test above
        for name in no_signature:
            with self.subTest(builtin=name):
                self.assertIsNone(obj.__text_signature__)

    def test_python_function_override_signature(self):
        def func(*args, **kwargs):
            pass
        func.__text_signature__ = '($self, a, b=1, *args, c, d=2, **kwargs)'
        sig = inspect.signature(func)
        self.assertIsNotNone(sig)
        self.assertEqual(str(sig), '(self, /, a, b=1, *args, c, d=2, **kwargs)')
        func.__text_signature__ = '($self, a, b=1, /, *args, c, d=2, **kwargs)'
        sig = inspect.signature(func)
        self.assertEqual(str(sig), '(self, a, b=1, /, *args, c, d=2, **kwargs)')


class NTimesUnwrappable:
    def __init__(self, n):
        self.n = n
        self._next = None

    @property
    def __wrapped__(self):
        if self.n <= 0:
            raise Exception("Unwrapped too many times")
        if self._next is None:
            self._next = NTimesUnwrappable(self.n - 1)
        return self._next

class TestUnwrap(unittest.TestCase):

    def test_unwrap_one(self):
        def func(a, b):
            return a + b
        wrapper = functools.lru_cache(maxsize=20)(func)
        self.assertIs(inspect.unwrap(wrapper), func)

    def test_unwrap_several(self):
        def func(a, b):
            return a + b
        wrapper = func
        for __ in range(10):
            @functools.wraps(wrapper)
            def wrapper():
                pass
        self.assertIsNot(wrapper.__wrapped__, func)
        self.assertIs(inspect.unwrap(wrapper), func)

    def test_stop(self):
        def func1(a, b):
            return a + b
        @functools.wraps(func1)
        def func2():
            pass
        @functools.wraps(func2)
        def wrapper():
            pass
        func2.stop_here = 1
        unwrapped = inspect.unwrap(wrapper,
                                   stop=(lambda f: hasattr(f, "stop_here")))
        self.assertIs(unwrapped, func2)

    def test_cycle(self):
        def func1(): pass
        func1.__wrapped__ = func1
        with self.assertRaisesRegex(ValueError, 'wrapper loop'):
            inspect.unwrap(func1)

        def func2(): pass
        func2.__wrapped__ = func1
        func1.__wrapped__ = func2
        with self.assertRaisesRegex(ValueError, 'wrapper loop'):
            inspect.unwrap(func1)
        with self.assertRaisesRegex(ValueError, 'wrapper loop'):
            inspect.unwrap(func2)

    def test_unhashable(self):
        def func(): pass
        func.__wrapped__ = None
        class C:
            __hash__ = None
            __wrapped__ = func
        self.assertIsNone(inspect.unwrap(C()))

    def test_recursion_limit(self):
        obj = NTimesUnwrappable(sys.getrecursionlimit() + 1)
        with self.assertRaisesRegex(ValueError, 'wrapper loop'):
            inspect.unwrap(obj)

class TestMain(unittest.TestCase):
    def test_only_source(self):
        module = importlib.import_module('unittest')
        rc, out, err = assert_python_ok('-m', 'inspect',
                                        'unittest')
        lines = out.decode().splitlines()
        # ignore the final newline
        self.assertEqual(lines[:-1], inspect.getsource(module).splitlines())
        self.assertEqual(err, b'')

    def test_custom_getattr(self):
        def foo():
            pass
        foo.__signature__ = 42
        with self.assertRaises(TypeError):
            inspect.signature(foo)

    @unittest.skipIf(ThreadPoolExecutor is None,
            'threads required to test __qualname__ for source files')
    def test_qualname_source(self):
        rc, out, err = assert_python_ok('-m', 'inspect',
                                     'concurrent.futures:ThreadPoolExecutor')
        lines = out.decode().splitlines()
        # ignore the final newline
        self.assertEqual(lines[:-1],
                         inspect.getsource(ThreadPoolExecutor).splitlines())
        self.assertEqual(err, b'')

    def test_builtins(self):
        module = importlib.import_module('unittest')
        _, out, err = assert_python_failure('-m', 'inspect',
                                            'sys')
        lines = err.decode().splitlines()
        self.assertEqual(lines, ["Can't get info for builtin modules."])

    def test_details(self):
        module = importlib.import_module('unittest')
        args = support.optim_args_from_interpreter_flags()
        rc, out, err = assert_python_ok(*args, '-m', 'inspect',
                                        'unittest', '--details')
        output = out.decode()
        # Just a quick sanity check on the output
        self.assertIn(module.__name__, output)
        self.assertIn(module.__file__, output)
        self.assertIn(module.__cached__, output)
        self.assertEqual(err, b'')


class TestReload(unittest.TestCase):

    src_before = textwrap.dedent("""\
def foo():
    print("Bla")
    """)

    src_after = textwrap.dedent("""\
def foo():
    print("Oh no!")
    """)

    def assertInspectEqual(self, path, source):
        inspected_src = inspect.getsource(source)
        with open(path, encoding='utf-8') as src:
            self.assertEqual(
                src.read().splitlines(True),
                inspected_src.splitlines(True)
            )

    def test_getsource_reload(self):
        # see issue 1218234
        with _ready_to_import('reload_bug', self.src_before) as (name, path):
            module = importlib.import_module(name)
            self.assertInspectEqual(path, module)
            with open(path, 'w', encoding='utf-8') as src:
                src.write(self.src_after)
            self.assertInspectEqual(path, module)


def test_main():
    run_unittest(
        TestDecorators, TestRetrievingSourceCode, TestOneliners, TestBlockComments,
        TestBuggyCases, TestInterpreterStack, TestClassesAndFunctions, TestPredicates,
        TestGetcallargsFunctions, TestGetcallargsMethods,
        TestGetcallargsUnboundMethods, TestGetattrStatic, TestGetGeneratorState,
        TestNoEOL, TestSignatureObject, TestSignatureBind, TestParameterObject,
        TestBoundArguments, TestSignaturePrivateHelpers,
        TestSignatureDefinitions, TestIsDataDescriptor,
        TestGetClosureVars, TestUnwrap, TestMain, TestReload,
        TestGetCoroutineState, TestGettingSourceOfToplevelFrames
    )

if __name__ == "__main__":
    test_main()
