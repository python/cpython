from annotationlib import Format, ForwardRef
import asyncio
import builtins
import collections
import copy
import datetime
import functools
import gc
import importlib
import inspect
import io
import linecache
import os
import dis
from os.path import normcase
import _pickle
import pickle
import shutil
import stat
import sys
import subprocess
import time
import types
import tempfile
import textwrap
import unicodedata
import unittest
import unittest.mock
import warnings
import weakref


try:
    from concurrent.futures import ThreadPoolExecutor
except ImportError:
    ThreadPoolExecutor = None

from test.support import cpython_only, import_helper
from test.support import MISSING_C_DOCSTRINGS, ALWAYS_EQ
from test.support import run_no_yield_async_fn, EqualToForwardRef
from test.support.import_helper import DirsOnSysPath, ready_to_import
from test.support.os_helper import TESTFN, temp_cwd
from test.support.script_helper import assert_python_ok, assert_python_failure, kill_python
from test.support import has_subprocess_support
from test import support

from test.test_inspect import inspect_fodder as mod
from test.test_inspect import inspect_fodder2 as mod2
from test.test_inspect import inspect_stringized_annotations
from test.test_inspect import inspect_deferred_annotations


# Functions tested in this suite:
# ismodule, isclass, ismethod, isfunction, istraceback, isframe, iscode,
# isbuiltin, isroutine, isgenerator, ispackage, isgeneratorfunction, getmembers,
# getdoc, getfile, getmodule, getsourcefile, getcomments, getsource,
# getclasstree, getargvalues, formatargvalues, currentframe,
# stack, trace, ismethoddescriptor, isdatadescriptor, ismethodwrapper

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
                      inspect.ismodule, inspect.istraceback, inspect.ispackage,
                      inspect.isgenerator, inspect.isgeneratorfunction,
                      inspect.iscoroutine, inspect.iscoroutinefunction,
                      inspect.isasyncgen, inspect.isasyncgenfunction,
                      inspect.ismethodwrapper])

    def istest(self, predicate, exp):
        obj = eval(exp)
        self.assertTrue(predicate(obj), '%s(%s)' % (predicate.__name__, exp))

        for other in self.predicates - set([predicate]):
            if (predicate == inspect.isgeneratorfunction or \
               predicate == inspect.isasyncgenfunction or \
               predicate == inspect.iscoroutinefunction) and \
               other == inspect.isfunction:
                continue
            if predicate == inspect.ispackage and other == inspect.ismodule:
                self.assertTrue(predicate(obj), '%s(%s)' % (predicate.__name__, exp))
            else:
                self.assertFalse(other(obj), 'not %s(%s)' % (other.__name__, exp))

    def test__all__(self):
        support.check__all__(self, inspect, not_exported=("modulesbyfile",), extra=("get_annotations",))

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

def meth_noargs(): pass
def meth_o(object, /): pass
def meth_self_noargs(self, /): pass
def meth_self_o(self, object, /): pass
def meth_type_noargs(type, /): pass
def meth_type_o(type, object, /): pass


class TestPredicates(IsTestBase):

    def test_excluding_predicates(self):
        global tb
        self.istest(inspect.isbuiltin, 'sys.exit')
        self.istest(inspect.isbuiltin, '[].append')
        self.istest(inspect.iscode, 'mod.spam.__code__')
        try:
            1/0
        except Exception as e:
            tb = e.__traceback__
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
        self.istest(inspect.ismethoddescriptor, 'int.__add__')
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
        self.istest(inspect.ismethodwrapper, "object().__str__")
        self.istest(inspect.ismethodwrapper, "object().__eq__")
        self.istest(inspect.ismethodwrapper, "object().__repr__")
        self.assertFalse(inspect.ismethodwrapper(type))
        self.assertFalse(inspect.ismethodwrapper(int))
        self.assertFalse(inspect.ismethodwrapper(type("AnyClass", (), {})))

    def test_ispackage(self):
        self.istest(inspect.ispackage, 'unittest')
        self.istest(inspect.ispackage, 'importlib')
        self.assertFalse(inspect.ispackage(inspect))
        self.assertFalse(inspect.ispackage(mod))
        self.assertFalse(inspect.ispackage(':)'))

        class FakePackage:
            __path__ = None

        self.assertFalse(inspect.ispackage(FakePackage()))

    def test_iscoroutine(self):
        async_gen_coro = async_generator_function_example(1)
        gen_coro = gen_coroutine_function_example(1)
        coro = coroutine_function_example(1)

        class PMClass:
            async_generator_partialmethod_example = functools.partialmethod(
                async_generator_function_example)
            coroutine_partialmethod_example = functools.partialmethod(
                coroutine_function_example)
            gen_coroutine_partialmethod_example = functools.partialmethod(
                gen_coroutine_function_example)

        # partialmethods on the class, bound to an instance
        pm_instance = PMClass()
        async_gen_coro_pmi = pm_instance.async_generator_partialmethod_example
        gen_coro_pmi = pm_instance.gen_coroutine_partialmethod_example
        coro_pmi = pm_instance.coroutine_partialmethod_example

        # partialmethods on the class, unbound but accessed via the class
        async_gen_coro_pmc = PMClass.async_generator_partialmethod_example
        gen_coro_pmc = PMClass.gen_coroutine_partialmethod_example
        coro_pmc = PMClass.coroutine_partialmethod_example

        self.assertFalse(
            inspect.iscoroutinefunction(gen_coroutine_function_example))
        self.assertFalse(
            inspect.iscoroutinefunction(
                functools.partial(functools.partial(
                    gen_coroutine_function_example))))
        self.assertFalse(inspect.iscoroutinefunction(gen_coro_pmi))
        self.assertFalse(inspect.iscoroutinefunction(gen_coro_pmc))
        self.assertFalse(inspect.iscoroutinefunction(inspect))
        self.assertFalse(inspect.iscoroutine(gen_coro))

        self.assertTrue(
            inspect.isgeneratorfunction(gen_coroutine_function_example))
        self.assertTrue(
            inspect.isgeneratorfunction(
                functools.partial(functools.partial(
                    gen_coroutine_function_example))))
        self.assertTrue(inspect.isgeneratorfunction(gen_coro_pmi))
        self.assertTrue(inspect.isgeneratorfunction(gen_coro_pmc))
        self.assertTrue(inspect.isgenerator(gen_coro))

        async def _fn3():
            pass

        @inspect.markcoroutinefunction
        def fn3():
            return _fn3()

        self.assertTrue(inspect.iscoroutinefunction(fn3))
        self.assertTrue(
            inspect.iscoroutinefunction(
                inspect.markcoroutinefunction(lambda: _fn3())
            )
        )

        class Cl:
            async def __call__(self):
                pass

        self.assertFalse(inspect.iscoroutinefunction(Cl))
        # instances with async def __call__ are NOT recognised.
        self.assertFalse(inspect.iscoroutinefunction(Cl()))
        # unless explicitly marked.
        self.assertTrue(inspect.iscoroutinefunction(
            inspect.markcoroutinefunction(Cl())
        ))

        class Cl2:
            @inspect.markcoroutinefunction
            def __call__(self):
                pass

        self.assertFalse(inspect.iscoroutinefunction(Cl2))
        # instances with marked __call__ are NOT recognised.
        self.assertFalse(inspect.iscoroutinefunction(Cl2()))
        # unless explicitly marked.
        self.assertTrue(inspect.iscoroutinefunction(
            inspect.markcoroutinefunction(Cl2())
        ))

        class Cl3:
            @inspect.markcoroutinefunction
            @classmethod
            def do_something_classy(cls):
                pass

            @inspect.markcoroutinefunction
            @staticmethod
            def do_something_static():
                pass

        self.assertTrue(inspect.iscoroutinefunction(Cl3.do_something_classy))
        self.assertTrue(inspect.iscoroutinefunction(Cl3.do_something_static))

        self.assertFalse(
            inspect.iscoroutinefunction(unittest.mock.Mock()))
        self.assertTrue(
            inspect.iscoroutinefunction(unittest.mock.AsyncMock()))
        self.assertTrue(
            inspect.iscoroutinefunction(coroutine_function_example))
        self.assertTrue(
            inspect.iscoroutinefunction(
                functools.partial(functools.partial(
                    coroutine_function_example))))
        self.assertTrue(inspect.iscoroutinefunction(coro_pmi))
        self.assertTrue(inspect.iscoroutinefunction(coro_pmc))
        self.assertTrue(inspect.iscoroutine(coro))

        self.assertFalse(
            inspect.isgeneratorfunction(unittest.mock.Mock()))
        self.assertFalse(
            inspect.isgeneratorfunction(unittest.mock.AsyncMock()))
        self.assertFalse(
            inspect.isgeneratorfunction(coroutine_function_example))
        self.assertFalse(
            inspect.isgeneratorfunction(
                functools.partial(functools.partial(
                    coroutine_function_example))))
        self.assertFalse(inspect.isgeneratorfunction(coro_pmi))
        self.assertFalse(inspect.isgeneratorfunction(coro_pmc))
        self.assertFalse(inspect.isgenerator(coro))

        self.assertFalse(
            inspect.isasyncgenfunction(unittest.mock.Mock()))
        self.assertFalse(
            inspect.isasyncgenfunction(unittest.mock.AsyncMock()))
        self.assertFalse(
            inspect.isasyncgenfunction(coroutine_function_example))
        self.assertTrue(
            inspect.isasyncgenfunction(async_generator_function_example))
        self.assertTrue(
            inspect.isasyncgenfunction(
                functools.partial(functools.partial(
                    async_generator_function_example))))
        self.assertTrue(inspect.isasyncgenfunction(async_gen_coro_pmi))
        self.assertTrue(inspect.isasyncgenfunction(async_gen_coro_pmc))
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
        # method
        self.assertTrue(inspect.isroutine(git.argue))
        self.assertTrue(inspect.isroutine(mod.custom_method))
        self.assertTrue(inspect.isroutine([].count))
        # function
        self.assertTrue(inspect.isroutine(mod.spam))
        self.assertTrue(inspect.isroutine(mod.StupidGit.abuse))
        # slot-wrapper
        self.assertTrue(inspect.isroutine(object.__init__))
        self.assertTrue(inspect.isroutine(object.__str__))
        self.assertTrue(inspect.isroutine(object.__lt__))
        self.assertTrue(inspect.isroutine(int.__lt__))
        # method-wrapper
        self.assertTrue(inspect.isroutine(object().__init__))
        self.assertTrue(inspect.isroutine(object().__str__))
        self.assertTrue(inspect.isroutine(object().__lt__))
        self.assertTrue(inspect.isroutine((42).__lt__))
        # method-descriptor
        self.assertTrue(inspect.isroutine(str.join))
        self.assertTrue(inspect.isroutine(list.append))
        self.assertTrue(inspect.isroutine(''.join))
        self.assertTrue(inspect.isroutine([].append))
        # object
        self.assertFalse(inspect.isroutine(object))
        self.assertFalse(inspect.isroutine(object()))
        self.assertFalse(inspect.isroutine(str()))
        # module
        self.assertFalse(inspect.isroutine(mod))
        # type
        self.assertFalse(inspect.isroutine(type))
        self.assertFalse(inspect.isroutine(int))
        self.assertFalse(inspect.isroutine(type('some_class', (), {})))
        # partial
        self.assertTrue(inspect.isroutine(functools.partial(mod.spam)))

    def test_isroutine_singledispatch(self):
        self.assertTrue(inspect.isroutine(functools.singledispatch(mod.spam)))

        class A:
            @functools.singledispatchmethod
            def method(self, arg):
                pass
            @functools.singledispatchmethod
            @classmethod
            def class_method(cls, arg):
                pass
            @functools.singledispatchmethod
            @staticmethod
            def static_method(arg):
                pass

        self.assertTrue(inspect.isroutine(A.method))
        self.assertTrue(inspect.isroutine(A().method))
        self.assertTrue(inspect.isroutine(A.static_method))
        self.assertTrue(inspect.isroutine(A.class_method))

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
        self.istest(inspect.istraceback, 'git.ex.__traceback__')
        self.istest(inspect.isframe, 'mod.fr')

    def test_stack(self):
        self.assertTrue(len(mod.st) >= 5)
        frame1, frame2, frame3, frame4, *_ = mod.st
        frameinfo = revise(*frame1[1:])
        self.assertEqual(frameinfo,
             (modfile, 16, 'eggs', ['    st = inspect.stack()\n'], 0))
        self.assertEqual(frame1.positions, dis.Positions(16, 16, 9, 24))
        frameinfo = revise(*frame2[1:])
        self.assertEqual(frameinfo,
             (modfile, 9, 'spam', ['    eggs(b + d, c + f)\n'], 0))
        self.assertEqual(frame2.positions, dis.Positions(9, 9, 4, 22))
        frameinfo = revise(*frame3[1:])
        self.assertEqual(frameinfo,
             (modfile, 43, 'argue', ['            spam(a, b, c)\n'], 0))
        self.assertEqual(frame3.positions, dis.Positions(43, 43, 12, 25))
        frameinfo = revise(*frame4[1:])
        self.assertEqual(frameinfo,
             (modfile, 39, 'abuse', ['        self.argue(a, b, c)\n'], 0))
        self.assertEqual(frame4.positions, dis.Positions(39, 39, 8, 27))
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
        frame1, frame2, frame3, = git.tr
        self.assertEqual(revise(*frame1[1:]),
             (modfile, 43, 'argue', ['            spam(a, b, c)\n'], 0))
        self.assertEqual(frame1.positions, dis.Positions(43, 43, 12, 25))
        self.assertEqual(revise(*frame2[1:]),
             (modfile, 9, 'spam', ['    eggs(b + d, c + f)\n'], 0))
        self.assertEqual(frame2.positions, dis.Positions(9, 9, 4, 22))
        self.assertEqual(revise(*frame3[1:]),
             (modfile, 18, 'eggs', ['    q = y / 0\n'], 0))
        self.assertEqual(frame3.positions, dis.Positions(18, 18, 8, 13))

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
        self.assertEqual(functions, [('after_closing', mod.after_closing),
                                     ('eggs', mod.eggs),
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

    cleandoc_testdata = [
        # first line should have different margin
        (' An\n  indented\n   docstring.', 'An\nindented\n docstring.'),
        # trailing whitespace are not removed.
        (' An \n   \n  indented \n   docstring. ',
         'An \n \nindented \n docstring. '),
        # NUL is not termination.
        ('doc\0string\n\n  second\0line\n  third\0line\0',
         'doc\0string\n\nsecond\0line\nthird\0line\0'),
        # first line is lstrip()-ped. other lines are kept when no margin.[w:
        ('   ', ''),
        # compiler.cleandoc() doesn't strip leading/trailing newlines
        # to keep maximum backward compatibility.
        # inspect.cleandoc() removes them.
        ('\n\n\n  first paragraph\n\n   second paragraph\n\n',
         '\n\n\nfirst paragraph\n\n second paragraph\n\n'),
        ('   \n \n  \n   ', '\n \n  \n   '),
    ]

    def test_cleandoc(self):
        func = inspect.cleandoc
        for i, (input, expected) in enumerate(self.cleandoc_testdata):
            # only inspect.cleandoc() strip \n
            expected = expected.strip('\n')
            with self.subTest(i=i):
                self.assertEqual(func(input), expected)

    @cpython_only
    def test_c_cleandoc(self):
        try:
            import _testinternalcapi
        except ImportError:
            return unittest.skip("requires _testinternalcapi")
        func = _testinternalcapi.compiler_cleandoc
        for i, (input, expected) in enumerate(self.cleandoc_testdata):
            with self.subTest(i=i):
                self.assertEqual(func(input), expected)

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

    def test_getmodule_file_not_found(self):
        # See bpo-45406
        def _getabsfile(obj, _filename):
            raise FileNotFoundError('bad file')
        with unittest.mock.patch('inspect.getabsfile', _getabsfile):
            f = inspect.currentframe()
            self.assertIsNone(inspect.getmodule(f))
            inspect.getouterframes(f)  # smoke test

    def test_getframeinfo_get_first_line(self):
        frame_info = inspect.getframeinfo(self.fodderModule.fr, 50)
        self.assertEqual(frame_info.code_context[0], "# line 1\n")
        self.assertEqual(frame_info.code_context[1], "'A module docstring.'\n")

    def test_getsource(self):
        self.assertSourceEqual(git.abuse, 29, 39)
        self.assertSourceEqual(mod.StupidGit, 21, 51)
        self.assertSourceEqual(mod.lobbest, 75, 76)
        self.assertSourceEqual(mod.after_closing, 120, 120)

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

    def test_getsource_empty_file(self):
        with temp_cwd() as cwd:
            with open('empty_file.py', 'w'):
                pass
            sys.path.insert(0, cwd)
            try:
                import empty_file
                self.assertEqual(inspect.getsource(empty_file), '\n')
                self.assertEqual(inspect.getsourcelines(empty_file), (['\n'], 0))
            finally:
                sys.path.remove(cwd)

    def test_getfile(self):
        self.assertEqual(inspect.getfile(mod.StupidGit), mod.__file__)

    def test_getfile_builtin_module(self):
        with self.assertRaises(TypeError) as e:
            inspect.getfile(sys)
        self.assertStartsWith(str(e.exception), '<module')

    def test_getfile_builtin_class(self):
        with self.assertRaises(TypeError) as e:
            inspect.getfile(int)
        self.assertStartsWith(str(e.exception), '<class')

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

    def test_getsource_on_generated_class(self):
        A = type('A', (unittest.TestCase,), {})
        self.assertEqual(inspect.getsourcefile(A), __file__)
        self.assertEqual(inspect.getfile(A), __file__)
        self.assertIs(inspect.getmodule(A), sys.modules[__name__])
        self.assertRaises(OSError, inspect.getsource, A)
        self.assertRaises(OSError, inspect.getsourcelines, A)
        self.assertIsNone(inspect.getcomments(A))

    def test_getsource_on_class_without_firstlineno(self):
        __firstlineno__ = 1
        class C:
            nonlocal __firstlineno__
        self.assertRaises(OSError, inspect.getsource, C)

class TestGetsourceStdlib(unittest.TestCase):
    # Test Python implementations of the stdlib modules

    def test_getsource_stdlib_collections_abc(self):
        import collections.abc
        lines, lineno = inspect.getsourcelines(collections.abc.Sequence)
        self.assertEqual(lines[0], 'class Sequence(Reversible, Collection):\n')
        src = inspect.getsource(collections.abc.Sequence)
        self.assertEqual(src.splitlines(True), lines)

    def test_getsource_stdlib_tomllib(self):
        import tomllib
        self.assertRaises(OSError, inspect.getsource, tomllib.TOMLDecodeError)
        self.assertRaises(OSError, inspect.getsourcelines, tomllib.TOMLDecodeError)

    def test_getsource_stdlib_abc(self):
        # Pure Python implementation
        abc = import_helper.import_fresh_module('abc', blocked=['_abc'])
        with support.swap_item(sys.modules, 'abc', abc):
            self.assertRaises(OSError, inspect.getsource, abc.ABCMeta)
            self.assertRaises(OSError, inspect.getsourcelines, abc.ABCMeta)
        # With C acceleration
        import abc
        try:
            src = inspect.getsource(abc.ABCMeta)
            lines, lineno = inspect.getsourcelines(abc.ABCMeta)
        except OSError:
            pass
        else:
            self.assertEqual(lines[0], '    class ABCMeta(type):\n')
            self.assertEqual(src.splitlines(True), lines)

    def test_getsource_stdlib_decimal(self):
        # Pure Python implementation
        decimal = import_helper.import_fresh_module('decimal', blocked=['_decimal'])
        with support.swap_item(sys.modules, 'decimal', decimal):
            src = inspect.getsource(decimal.Decimal)
            lines, lineno = inspect.getsourcelines(decimal.Decimal)
        self.assertEqual(lines[0], 'class Decimal(object):\n')
        self.assertEqual(src.splitlines(True), lines)

class TestGetsourceInteractive(unittest.TestCase):
    @support.force_not_colorized
    def test_getclasses_interactive(self):
        # bpo-44648: simulate a REPL session;
        # there is no `__file__` in the __main__ module
        code = "import sys, inspect; \
                assert not hasattr(sys.modules['__main__'], '__file__'); \
                A = type('A', (), {}); \
                inspect.getsource(A)"
        _, _, stderr = assert_python_failure("-c", code, __isolated=True)
        self.assertIn(b'OSError: source code not available', stderr)

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

    def test_parenthesized_multiline_lambda(self):
        # Test inspect.getsource with a parenthesized multi-line lambda
        # function.
        self.assertSourceEqual(mod2.parenthesized_lambda, 279, 279)
        self.assertSourceEqual(mod2.parenthesized_lambda2, 281, 281)
        self.assertSourceEqual(mod2.parenthesized_lambda3, 283, 283)

    def test_post_line_parenthesized_lambda(self):
        # Test inspect.getsource with a parenthesized multi-line lambda
        # function.
        self.assertSourceEqual(mod2.post_line_parenthesized_lambda1, 286, 287)

    def test_nested_lambda(self):
        # Test inspect.getsource with a nested lambda function.
        self.assertSourceEqual(mod2.nested_lambda, 291, 292)

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

    def test_enum(self):
        self.assertSourceEqual(mod2.enum322, 322, 323)
        self.assertSourceEqual(mod2.enum326, 326, 327)
        self.assertSourceEqual(mod2.flag330, 330, 331)
        self.assertSourceEqual(mod2.flag334, 334, 335)
        self.assertRaises(OSError, inspect.getsource, mod2.simple_enum338)
        self.assertRaises(OSError, inspect.getsource, mod2.simple_enum339)
        self.assertRaises(OSError, inspect.getsource, mod2.simple_flag340)
        self.assertRaises(OSError, inspect.getsource, mod2.simple_flag341)

    def test_namedtuple(self):
        self.assertSourceEqual(mod2.nt346, 346, 348)
        self.assertRaises(OSError, inspect.getsource, mod2.nt351)

    def test_typeddict(self):
        self.assertSourceEqual(mod2.td354, 354, 356)
        self.assertRaises(OSError, inspect.getsource, mod2.td359)

    def test_dataclass(self):
        self.assertSourceEqual(mod2.dc364, 364, 367)
        self.assertRaises(OSError, inspect.getsource, mod2.dc370)
        self.assertRaises(OSError, inspect.getsource, mod2.dc371)

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

    def test_findsource_on_func_with_out_of_bounds_lineno(self):
        mod_len = len(inspect.getsource(mod))
        src = '\n' * 2* mod_len + "def f(): pass"
        co = compile(src, mod.__file__, "exec")
        g, l = {}, {}
        eval(co, g, l)
        func = l['f']
        self.assertEqual(func.__code__.co_firstlineno, 1+2*mod_len)
        with self.assertRaisesRegex(OSError, "lineno is out of bounds"):
            inspect.findsource(func)

    def test_findsource_on_class_with_out_of_bounds_lineno(self):
        mod_len = len(inspect.getsource(mod))
        src = '\n' * 2* mod_len + "class A: pass"
        co = compile(src, mod.__file__, "exec")
        g, l = {'__name__': mod.__name__}, {}
        eval(co, g, l)
        cls = l['A']
        self.assertEqual(cls.__firstlineno__, 1+2*mod_len)
        with self.assertRaisesRegex(OSError, "lineno is out of bounds"):
            inspect.findsource(cls)

    def test_getsource_on_method(self):
        self.assertSourceEqual(mod2.ClassWithMethod.method, 118, 119)

    def test_getsource_on_class_code_object(self):
        self.assertSourceEqual(mod2.ClassWithCodeObject.code, 315, 317)

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

    @support.requires_docstrings
    def test_class_inside_conditional(self):
        # We skip this test when docstrings are not present,
        # because docstrings are one of the main factors of
        # finding the correct class in the source code.
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

    def test_class_with_method_from_other_module(self):
        with tempfile.TemporaryDirectory() as tempdir:
            with open(os.path.join(tempdir, 'inspect_actual%spy' % os.extsep),
                      'w', encoding='utf-8') as f:
                f.write(textwrap.dedent("""
                    import inspect_other
                    class A:
                        def f(self):
                            pass
                    class A:
                        def f(self):
                            pass  # correct one
                    A.f = inspect_other.A.f
                    """))

            with open(os.path.join(tempdir, 'inspect_other%spy' % os.extsep),
                      'w', encoding='utf-8') as f:
                f.write(textwrap.dedent("""
                    class A:
                        def f(self):
                            pass
                    """))

            with DirsOnSysPath(tempdir):
                import inspect_actual
                self.assertIn("correct", inspect.getsource(inspect_actual.A))
                # Remove the module from sys.modules to force it to be reloaded.
                # This is necessary when the test is run multiple times.
                sys.modules.pop("inspect_actual")

    def test_nested_class_definition_inside_async_function(self):
        run = run_no_yield_async_fn

        self.assertSourceEqual(run(mod2.func225), 226, 227)
        self.assertSourceEqual(mod2.cls226, 231, 235)
        self.assertSourceEqual(mod2.cls226.func232, 232, 235)
        self.assertSourceEqual(run(mod2.cls226().func232), 233, 234)

    def test_class_definition_same_name_diff_methods(self):
        self.assertSourceEqual(mod2.cls296, 296, 298)
        self.assertSourceEqual(mod2.cls310, 310, 312)

    def test_generator_expression(self):
        self.assertSourceEqual(next(mod2.ge377), 377, 380)
        self.assertSourceEqual(next(mod2.func383()), 385, 388)


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


class TestComplexDecorator(GetSourceBase):
    fodderModule = mod2

    def test_parens_in_decorator(self):
        self.assertSourceEqual(self.fodderModule.complex_decorated, 273, 275)

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

    def assertFullArgSpecEquals(self, routine, args_e, varargs_e=None,
                                    varkw_e=None, defaults_e=None,
                                    posonlyargs_e=[], kwonlyargs_e=[],
                                    kwonlydefaults_e=None,
                                    ann_e={}):
        args, varargs, varkw, defaults, kwonlyargs, kwonlydefaults, ann = \
            inspect.getfullargspec(routine)
        self.assertEqual(args, args_e)
        self.assertEqual(varargs, varargs_e)
        self.assertEqual(varkw, varkw_e)
        self.assertEqual(defaults, defaults_e)
        self.assertEqual(kwonlyargs, kwonlyargs_e)
        self.assertEqual(kwonlydefaults, kwonlydefaults_e)
        self.assertEqual(ann, ann_e)

    def test_getfullargspec(self):
        self.assertFullArgSpecEquals(mod2.keyworded, [], varargs_e='arg1',
                                     kwonlyargs_e=['arg2'],
                                     kwonlydefaults_e={'arg2':1})

        self.assertFullArgSpecEquals(mod2.annotated, ['arg1'],
                                     ann_e={'arg1' : list})
        self.assertFullArgSpecEquals(mod2.keyword_only_arg, [],
                                     kwonlyargs_e=['arg'])

        self.assertFullArgSpecEquals(mod2.all_markers, ['a', 'b', 'c', 'd'],
                                     kwonlyargs_e=['e', 'f'])

        self.assertFullArgSpecEquals(mod2.all_markers_with_args_and_kwargs,
                                     ['a', 'b', 'c', 'd'],
                                     varargs_e='args',
                                     varkw_e='kwargs',
                                     kwonlyargs_e=['e', 'f'])

        self.assertFullArgSpecEquals(mod2.all_markers_with_defaults, ['a', 'b', 'c', 'd'],
                                     defaults_e=(1,2,3),
                                     kwonlyargs_e=['e', 'f'],
                                     kwonlydefaults_e={'e': 4, 'f': 5})

    def test_argspec_api_ignores_wrapped(self):
        # Issue 20684: low level introspection API must ignore __wrapped__
        @functools.wraps(mod.spam)
        def ham(x, y):
            pass
        # Basic check
        self.assertFullArgSpecEquals(ham, ['x', 'y'])
        self.assertFullArgSpecEquals(functools.partial(ham),
                                     ['x', 'y'])

    def test_getfullargspec_signature_attr(self):
        def test():
            pass
        spam_param = inspect.Parameter('spam', inspect.Parameter.POSITIONAL_ONLY)
        test.__signature__ = inspect.Signature(parameters=(spam_param,))

        self.assertFullArgSpecEquals(test, ['spam'])

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
        self.assertFullArgSpecEquals(_pickle.Pickler.dump, ['self', 'obj'])

        self.assertFullArgSpecEquals(_pickle.Pickler(io.BytesIO()).dump, ['self', 'obj'])

        self.assertFullArgSpecEquals(
             os.stat,
             args_e=['path'],
             kwonlyargs_e=['dir_fd', 'follow_symlinks'],
             kwonlydefaults_e={'dir_fd': None, 'follow_symlinks': True})

    @cpython_only
    @unittest.skipIf(MISSING_C_DOCSTRINGS,
                     "Signature information for builtins requires docstrings")
    def test_getfullargspec_builtin_func(self):
        _testcapi = import_helper.import_module("_testcapi")
        builtin = _testcapi.docstring_with_signature_with_defaults
        spec = inspect.getfullargspec(builtin)
        self.assertEqual(spec.defaults[0], 'avocado')

    @cpython_only
    @unittest.skipIf(MISSING_C_DOCSTRINGS,
                     "Signature information for builtins requires docstrings")
    def test_getfullargspec_builtin_func_no_signature(self):
        _testcapi = import_helper.import_module("_testcapi")
        builtin = _testcapi.docstring_no_signature
        with self.assertRaises(TypeError):
            inspect.getfullargspec(builtin)

        cls = _testcapi.DocStringNoSignatureTest
        obj = _testcapi.DocStringNoSignatureTest()
        tests = [
            (_testcapi.docstring_no_signature_noargs, meth_noargs),
            (_testcapi.docstring_no_signature_o, meth_o),
            (cls.meth_noargs, meth_self_noargs),
            (cls.meth_o, meth_self_o),
            (obj.meth_noargs, meth_self_noargs),
            (obj.meth_o, meth_self_o),
            (cls.meth_noargs_class, meth_type_noargs),
            (cls.meth_o_class, meth_type_o),
            (cls.meth_noargs_static, meth_noargs),
            (cls.meth_o_static, meth_o),
            (cls.meth_noargs_coexist, meth_self_noargs),
            (cls.meth_o_coexist, meth_self_o),

            (time.time, meth_noargs),
            (str.lower, meth_self_noargs),
            (''.lower, meth_self_noargs),
            (set.add, meth_self_o),
            (set().add, meth_self_o),
            (set.__contains__, meth_self_o),
            (set().__contains__, meth_self_o),
            (datetime.datetime.__dict__['utcnow'], meth_type_noargs),
            (datetime.datetime.utcnow, meth_type_noargs),
            (dict.__dict__['__class_getitem__'], meth_type_o),
            (dict.__class_getitem__, meth_type_o),
        ]
        try:
            import _stat  # noqa: F401
        except ImportError:
            # if the _stat extension is not available, stat.S_IMODE() is
            # implemented in Python, not in C
            pass
        else:
            tests.append((stat.S_IMODE, meth_o))
        for builtin, template in tests:
            with self.subTest(builtin):
                self.assertEqual(inspect.getfullargspec(builtin),
                                 inspect.getfullargspec(template))

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

    def test_getmembers_custom_dir(self):
        class CorrectDir:
            def __init__(self, attr):
                self.attr = attr
            def method(self):
                return self.attr + 1
            def __dir__(self):
                return ['attr', 'method']

        cd = CorrectDir(5)
        self.assertEqual(inspect.getmembers(cd), [
            ('attr', 5),
            ('method', cd.method),
        ])
        self.assertEqual(inspect.getmembers(cd, inspect.ismethod), [
            ('method', cd.method),
        ])

    def test_getmembers_custom_broken_dir(self):
        # inspect.getmembers calls `dir()` on the passed object inside.
        # if `__dir__` mentions some non-existent attribute,
        # we still need to return others correctly.
        class BrokenDir:
            existing = 1
            def method(self):
                return self.existing + 1
            def __dir__(self):
                return ['method', 'missing', 'existing']

        bd = BrokenDir()
        self.assertEqual(inspect.getmembers(bd), [
            ('existing', 1),
            ('method', bd.method),
        ])
        self.assertEqual(inspect.getmembers(bd, inspect.ismethod), [
            ('method', bd.method),
        ])

    def test_getmembers_custom_duplicated_dir(self):
        # Duplicates in `__dir__` must not fail and return just one result.
        class DuplicatedDir:
            attr = 1
            def __dir__(self):
                return ['attr', 'attr']

        dd = DuplicatedDir()
        self.assertEqual(inspect.getmembers(dd), [
            ('attr', 1),
        ])

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
        class B:
            def __getattr__(self, attribute):
                return None
        self.assertIn(('eggs', 'scrambled'), inspect.getmembers(A))
        self.assertIn(('eggs', 'spam'), inspect.getmembers(A()))
        b = B()
        self.assertIn(('__getattr__', b.__getattr__), inspect.getmembers(b))

    def test_getmembers_static(self):
        class A:
            @property
            def name(self):
                raise NotImplementedError
            @types.DynamicClassAttribute
            def eggs(self):
                raise NotImplementedError

        a = A()
        instance_members = inspect.getmembers_static(a)
        class_members = inspect.getmembers_static(A)
        self.assertIn(('name', inspect.getattr_static(a, 'name')), instance_members)
        self.assertIn(('eggs', inspect.getattr_static(a, 'eggs')), instance_members)
        self.assertIn(('name', inspect.getattr_static(A, 'name')), class_members)
        self.assertIn(('eggs', inspect.getattr_static(A, 'eggs')), class_members)

    def test_getmembers_with_buggy_dir(self):
        class M(type):
            def __dir__(cls):
                return ['__class__', '__name__', 'missing']
        class C(metaclass=M):
            pass
        attrs = [a[0] for a in inspect.getmembers(C)]
        self.assertNotIn('missing', attrs)


class TestFormatAnnotation(unittest.TestCase):
    def test_typing_replacement(self):
        from test.typinganndata.ann_module9 import ann, ann1
        self.assertEqual(inspect.formatannotation(ann), 'List[str] | int')
        self.assertEqual(inspect.formatannotation(ann1), 'List[testModule.typing.A] | int')

    def test_forwardref(self):
        fwdref = ForwardRef('fwdref')
        self.assertEqual(inspect.formatannotation(fwdref), 'fwdref')


class TestIsMethodDescriptor(unittest.TestCase):

    def test_custom_descriptors(self):
        class MethodDescriptor:
            def __get__(self, *_): pass
        class MethodDescriptorSub(MethodDescriptor):
            pass
        class DataDescriptorWithNoGet:
            def __set__(self, *_): pass
        class DataDescriptorWithGetSet:
            def __get__(self, *_): pass
            def __set__(self, *_): pass
        class DataDescriptorWithGetDelete:
            def __get__(self, *_): pass
            def __delete__(self, *_): pass
        class DataDescriptorSub(DataDescriptorWithNoGet,
                                DataDescriptorWithGetDelete):
            pass

        # Custom method descriptors:
        self.assertTrue(
            inspect.ismethoddescriptor(MethodDescriptor()),
            '__get__ and no __set__/__delete__ => method descriptor')
        self.assertTrue(
            inspect.ismethoddescriptor(MethodDescriptorSub()),
            '__get__ (inherited) and no __set__/__delete__'
            ' => method descriptor')

        # Custom data descriptors:
        self.assertFalse(
            inspect.ismethoddescriptor(DataDescriptorWithNoGet()),
            '__set__ (and no __get__) => not a method descriptor')
        self.assertFalse(
            inspect.ismethoddescriptor(DataDescriptorWithGetSet()),
            '__get__ and __set__ => not a method descriptor')
        self.assertFalse(
            inspect.ismethoddescriptor(DataDescriptorWithGetDelete()),
            '__get__ and __delete__ => not a method descriptor')
        self.assertFalse(
            inspect.ismethoddescriptor(DataDescriptorSub()),
            '__get__, __set__ and __delete__ => not a method descriptor')

        # Classes of descriptors (are *not* descriptors themselves):
        self.assertFalse(inspect.ismethoddescriptor(MethodDescriptor))
        self.assertFalse(inspect.ismethoddescriptor(MethodDescriptorSub))
        self.assertFalse(inspect.ismethoddescriptor(DataDescriptorSub))

    def test_builtin_descriptors(self):
        builtin_slot_wrapper = int.__add__  # This one is mentioned in docs.
        class Owner:
            def instance_method(self): pass
            @classmethod
            def class_method(cls): pass
            @staticmethod
            def static_method(): pass
            @property
            def a_property(self): pass
        class Slotermeyer:
            __slots__ = 'a_slot',
        def function():
            pass
        a_lambda = lambda: None

        # Example builtin method descriptors:
        self.assertTrue(
            inspect.ismethoddescriptor(builtin_slot_wrapper),
            'a builtin slot wrapper is a method descriptor')
        self.assertTrue(
            inspect.ismethoddescriptor(Owner.__dict__['class_method']),
            'a classmethod object is a method descriptor')
        self.assertTrue(
            inspect.ismethoddescriptor(Owner.__dict__['static_method']),
            'a staticmethod object is a method descriptor')

        # Example builtin data descriptors:
        self.assertFalse(
            inspect.ismethoddescriptor(Owner.__dict__['a_property']),
            'a property is not a method descriptor')
        self.assertFalse(
            inspect.ismethoddescriptor(Slotermeyer.__dict__['a_slot']),
            'a slot is not a method descriptor')

        # `types.MethodType`/`types.FunctionType` instances (they *are*
        # method descriptors, but `ismethoddescriptor()` explicitly
        # excludes them):
        self.assertFalse(inspect.ismethoddescriptor(Owner().instance_method))
        self.assertFalse(inspect.ismethoddescriptor(Owner().class_method))
        self.assertFalse(inspect.ismethoddescriptor(Owner().static_method))
        self.assertFalse(inspect.ismethoddescriptor(Owner.instance_method))
        self.assertFalse(inspect.ismethoddescriptor(Owner.class_method))
        self.assertFalse(inspect.ismethoddescriptor(Owner.static_method))
        self.assertFalse(inspect.ismethoddescriptor(function))
        self.assertFalse(inspect.ismethoddescriptor(a_lambda))
        self.assertTrue(inspect.ismethoddescriptor(functools.partial(function)))

    def test_descriptor_being_a_class(self):
        class MethodDescriptorMeta(type):
            def __get__(self, *_): pass
        class ClassBeingMethodDescriptor(metaclass=MethodDescriptorMeta):
            pass
        # `ClassBeingMethodDescriptor` itself *is* a method descriptor,
        # but it is *also* a class, and `ismethoddescriptor()` explicitly
        # excludes classes.
        self.assertFalse(
            inspect.ismethoddescriptor(ClassBeingMethodDescriptor),
            'classes (instances of type) are explicitly excluded')

    def test_non_descriptors(self):
        class Test:
            pass
        self.assertFalse(inspect.ismethoddescriptor(Test()))
        self.assertFalse(inspect.ismethoddescriptor(Test))
        self.assertFalse(inspect.ismethoddescriptor([42]))
        self.assertFalse(inspect.ismethoddescriptor(42))


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

    def test_attribute_same_name_as_global_var(self):
        class C:
            _global_ref = object()
        def f():
            print(C._global_ref, _global_ref)
        nonlocal_vars = {"C": C}
        global_vars = {"_global_ref": _global_ref}
        builtin_vars = {"print": print}
        unbound_names = {"_global_ref"}
        expected = inspect.ClosureVars(nonlocal_vars, global_vars,
                                       builtin_vars, unbound_names)
        self.assertEqual(inspect.getclosurevars(f), expected)

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
            self.assertEqualException(f, '2, 3, 4, a=1, c=5')
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
            self.assertEqualException(f, '1, c=3, a=2')
        # issue11256:
        f3 = self.makeCallable('**c')
        self.assertEqualException(f3, '1, 2')
        self.assertEqualException(f3, '1, 2, a=1, b=2')
        f4 = self.makeCallable('*, a, b=0')
        self.assertEqualException(f4, '1, 2')
        self.assertEqualException(f4, '1, 2, a=1, b=2')
        self.assertEqualException(f4, 'a=1, a=3')
        self.assertEqualException(f4, 'a=1, c=3')
        self.assertEqualException(f4, 'a=1, a=3, b=4')
        self.assertEqualException(f4, 'a=1, b=2, a=3, b=4')
        self.assertEqualException(f4, 'a=1, a=2, a=3, b=4')

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

        del descriptor.__set__
        descriptor.__delete__ = lambda s, i, o: None
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

        class Bar(Foo): pass

        bar = Bar()
        bar.a = 5
        self.assertEqual(inspect.getattr_static(bar, 'a'), 3)
        self.assertFalse(test.called)

    def test_mutated_mro(self):
        test = self
        test.called = False

        class Foo(dict):
            a = 3
            @property
            def __dict__(self):
                test.called = True
                return {}

        class Bar(dict):
            a = 4

        class Baz(Bar): pass

        baz = Baz()
        self.assertEqual(inspect.getattr_static(baz, 'a'), 4)
        Baz.__bases__ = (Foo,)
        self.assertEqual(inspect.getattr_static(baz, 'a'), 3)
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

    def test_custom___getattr__(self):
        test = self
        test.called = False

        class Foo:
            def __getattr__(self, attr):
                test.called = True
                return {}

        with self.assertRaises(AttributeError):
            inspect.getattr_static(Foo(), 'whatever')

        self.assertFalse(test.called)

    def test_custom___getattribute__(self):
        test = self
        test.called = False

        class Foo:
            def __getattribute__(self, attr):
                test.called = True
                return {}

        with self.assertRaises(AttributeError):
            inspect.getattr_static(Foo(), 'really_could_be_anything')

        self.assertFalse(test.called)

    def test_cache_does_not_cause_classes_to_persist(self):
        # regression test for gh-118013:
        # check that the internal _shadowed_dict cache does not cause
        # dynamically created classes to have extended lifetimes even
        # when no other strong references to those classes remain.
        # Since these classes can themselves hold strong references to
        # other objects, this can cause unexpected memory consumption.
        class Foo: pass
        Foo.instance = Foo()
        weakref_to_class = weakref.ref(Foo)
        inspect.getattr_static(Foo.instance, 'whatever', 'irrelevant')
        del Foo
        gc.collect()
        self.assertIsNone(weakref_to_class())


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

    def test_closed_after_close(self):
        self.generator.close()
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

    def test_closed_after_close(self):
        self.coroutine.close()
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


@support.requires_working_socket()
class TestGetAsyncGenState(unittest.IsolatedAsyncioTestCase):

    def setUp(self):
        async def number_asyncgen():
            for number in range(5):
                yield number
        self.asyncgen = number_asyncgen()

    async def asyncTearDown(self):
        await self.asyncgen.aclose()

    @classmethod
    def tearDownClass(cls):
        asyncio.events._set_event_loop_policy(None)

    def _asyncgenstate(self):
        return inspect.getasyncgenstate(self.asyncgen)

    def test_created(self):
        self.assertEqual(self._asyncgenstate(), inspect.AGEN_CREATED)

    async def test_suspended(self):
        value = await anext(self.asyncgen)
        self.assertEqual(self._asyncgenstate(), inspect.AGEN_SUSPENDED)
        self.assertEqual(value, 0)

    async def test_closed_after_exhaustion(self):
        countdown = 7
        with self.assertRaises(StopAsyncIteration):
            while countdown := countdown - 1:
                await anext(self.asyncgen)
        self.assertEqual(countdown, 1)
        self.assertEqual(self._asyncgenstate(), inspect.AGEN_CLOSED)

    async def test_closed_after_immediate_exception(self):
        with self.assertRaises(RuntimeError):
            await self.asyncgen.athrow(RuntimeError)
        self.assertEqual(self._asyncgenstate(), inspect.AGEN_CLOSED)

    async def test_running(self):
        async def running_check_asyncgen():
            for number in range(5):
                self.assertEqual(self._asyncgenstate(), inspect.AGEN_RUNNING)
                yield number
                self.assertEqual(self._asyncgenstate(), inspect.AGEN_RUNNING)
        self.asyncgen = running_check_asyncgen()
        # Running up to the first yield
        await anext(self.asyncgen)
        self.assertEqual(self._asyncgenstate(), inspect.AGEN_SUSPENDED)
        # Running after the first yield
        await anext(self.asyncgen)
        self.assertEqual(self._asyncgenstate(), inspect.AGEN_SUSPENDED)

    def test_easy_debugging(self):
        # repr() and str() of a asyncgen state should contain the state name
        names = 'AGEN_CREATED AGEN_RUNNING AGEN_SUSPENDED AGEN_CLOSED'.split()
        for name in names:
            state = getattr(inspect, name)
            self.assertIn(name, repr(state))
            self.assertIn(name, str(state))

    async def test_getasyncgenlocals(self):
        async def each(lst, a=None):
            b=(1, 2, 3)
            for v in lst:
                if v == 3:
                    c = 12
                yield v

        numbers = each([1, 2, 3])
        self.assertEqual(inspect.getasyncgenlocals(numbers),
                         {'a': None, 'lst': [1, 2, 3]})
        await anext(numbers)
        self.assertEqual(inspect.getasyncgenlocals(numbers),
                         {'a': None, 'lst': [1, 2, 3], 'v': 1,
                          'b': (1, 2, 3)})
        await anext(numbers)
        self.assertEqual(inspect.getasyncgenlocals(numbers),
                         {'a': None, 'lst': [1, 2, 3], 'v': 2,
                          'b': (1, 2, 3)})
        await anext(numbers)
        self.assertEqual(inspect.getasyncgenlocals(numbers),
                         {'a': None, 'lst': [1, 2, 3], 'v': 3,
                          'b': (1, 2, 3), 'c': 12})
        with self.assertRaises(StopAsyncIteration):
            await anext(numbers)
        self.assertEqual(inspect.getasyncgenlocals(numbers), {})

    async def test_getasyncgenlocals_empty(self):
        async def yield_one():
            yield 1
        one = yield_one()
        self.assertEqual(inspect.getasyncgenlocals(one), {})
        await anext(one)
        self.assertEqual(inspect.getasyncgenlocals(one), {})
        with self.assertRaises(StopAsyncIteration):
            await anext(one)
        self.assertEqual(inspect.getasyncgenlocals(one), {})

    def test_getasyncgenlocals_error(self):
        self.assertRaises(TypeError, inspect.getasyncgenlocals, 1)
        self.assertRaises(TypeError, inspect.getasyncgenlocals, lambda x: True)
        self.assertRaises(TypeError, inspect.getasyncgenlocals, set)
        self.assertRaises(TypeError, inspect.getasyncgenlocals, (2,3))


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

        def test(po, /, pk, pkd=100, *args, ko, kod=10, **kwargs):
            pass

        sig = inspect.signature(test)
        self.assertStartsWith(repr(sig), '<Signature')
        self.assertTrue('(po, /, pk' in repr(sig))

        # We need two functions, because it is impossible to represent
        # all param kinds in a single one.
        def test2(pod=42, /):
            pass

        sig2 = inspect.signature(test2)
        self.assertStartsWith(repr(sig2), '<Signature')
        self.assertTrue('(pod=42, /)' in repr(sig2))

        po = sig.parameters['po']
        pod = sig2.parameters['pod']
        pk = sig.parameters['pk']
        pkd = sig.parameters['pkd']
        args = sig.parameters['args']
        ko = sig.parameters['ko']
        kod = sig.parameters['kod']
        kwargs = sig.parameters['kwargs']

        S((po, pk, args, ko, kwargs))
        S((po, pk, ko, kod))
        S((po, pod, ko))
        S((po, pod, kod))
        S((pod, ko, kod))
        S((pod, kod))
        S((pod, args, kod, kwargs))
        # keyword-only parameters without default values
        # can follow keyword-only parameters with default values:
        S((kod, ko))
        S((kod, ko, kwargs))
        S((args, kod, ko))

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
            S((pod, pk))

        with self.assertRaisesRegex(ValueError, 'follows default argument'):
            S((po, pod, pk))

        with self.assertRaisesRegex(ValueError, 'follows default argument'):
            S((po, pkd, pk))

        with self.assertRaisesRegex(ValueError, 'follows default argument'):
            S((pkd, pk))

        second_args = args.replace(name="second_args")
        with self.assertRaisesRegex(ValueError, 'more than one variadic positional parameter'):
            S((args, second_args))

        with self.assertRaisesRegex(ValueError, 'more than one variadic positional parameter'):
            S((args, ko, second_args))

        second_kwargs = kwargs.replace(name="second_kwargs")
        with self.assertRaisesRegex(ValueError, 'more than one variadic keyword parameter'):
            S((kwargs, second_kwargs))

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
        _testcapi = import_helper.import_module("_testcapi")

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
        self.assertEqual(p('exp'), sys.maxsize - 1)

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
        class ThisWorksNow:
            __call__ = type
        # TODO: Support type.
        self.assertEqual(ThisWorksNow()(1), int)
        self.assertEqual(ThisWorksNow()('A', (), {}).__name__, 'A')
        with self.assertRaisesRegex(ValueError, "no signature found"):
            test_callable(ThisWorksNow())

        # Regression test for issue #20786
        test_unbound_method(dict.__delitem__)
        test_unbound_method(property.__delete__)

        # Regression test for issue #20586
        test_callable(_testcapi.docstring_with_signature_but_no_doc)

        # Regression test for gh-104955
        method = bytearray.__release_buffer__
        sig = test_unbound_method(method)
        self.assertEqual(list(sig.parameters), ['self', 'buffer'])

    @cpython_only
    @unittest.skipIf(MISSING_C_DOCSTRINGS,
                     "Signature information for builtins requires docstrings")
    def test_signature_on_decorated_builtins(self):
        _testcapi = import_helper.import_module("_testcapi")
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
        _testcapi = import_helper.import_module("_testcapi")
        with self.assertRaisesRegex(ValueError,
                                    'no signature found for builtin'):
            inspect.signature(_testcapi.docstring_no_signature)

        with self.assertRaisesRegex(ValueError,
                                    'no signature found for builtin'):
            inspect.signature(str)

        cls = _testcapi.DocStringNoSignatureTest
        obj = _testcapi.DocStringNoSignatureTest()
        tests = [
            (_testcapi.docstring_no_signature_noargs, meth_noargs),
            (_testcapi.docstring_no_signature_o, meth_o),
            (cls.meth_noargs, meth_self_noargs),
            (cls.meth_o, meth_self_o),
            (obj.meth_noargs, meth_noargs),
            (obj.meth_o, meth_o),
            (cls.meth_noargs_class, meth_noargs),
            (cls.meth_o_class, meth_o),
            (cls.meth_noargs_static, meth_noargs),
            (cls.meth_o_static, meth_o),
            (cls.meth_noargs_coexist, meth_self_noargs),
            (cls.meth_o_coexist, meth_self_o),

            (time.time, meth_noargs),
            (str.lower, meth_self_noargs),
            (''.lower, meth_noargs),
            (set.add, meth_self_o),
            (set().add, meth_o),
            (set.__contains__, meth_self_o),
            (set().__contains__, meth_o),
            (datetime.datetime.__dict__['utcnow'], meth_type_noargs),
            (datetime.datetime.utcnow, meth_noargs),
            (dict.__dict__['__class_getitem__'], meth_type_o),
            (dict.__class_getitem__, meth_o),
        ]
        try:
            import _stat  # noqa: F401
        except ImportError:
            # if the _stat extension is not available, stat.S_IMODE() is
            # implemented in Python, not in C
            pass
        else:
            tests.append((stat.S_IMODE, meth_o))
        for builtin, template in tests:
            with self.subTest(builtin):
                self.assertEqual(inspect.signature(builtin),
                                 inspect.signature(template))

    @unittest.skipIf(MISSING_C_DOCSTRINGS,
                     "Signature information for builtins requires docstrings")
    def test_signature_parsing_with_defaults(self):
        _testcapi = import_helper.import_module("_testcapi")
        meth = _testcapi.DocStringUnrepresentableSignatureTest.with_default
        self.assertEqual(str(inspect.signature(meth)), '(self, /, x=1)')

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
        if not support.MISSING_C_DOCSTRINGS:
            self.assertEqual(self.signature(classmethod),
                            ((('function', ..., ..., "positional_only"),),
                            ...))

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
        if not support.MISSING_C_DOCSTRINGS:
            self.assertEqual(self.signature(staticmethod),
                            ((('function', ..., ..., "positional_only"),),
                            ...))

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
        from functools import partial, Placeholder

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

        # With Placeholder
        self.assertEqual(self.signature(partial(test, Placeholder, 1)),
                         ((('a', ..., ..., "positional_only"),
                           ('c', ..., ..., "keyword_only"),
                           ('d', ..., ..., "keyword_only")),
                          ...))

        self.assertEqual(self.signature(partial(test, Placeholder, 1, c=2)),
                         ((('a', ..., ..., "positional_only"),
                           ('c', 2, ..., "keyword_only"),
                           ('d', ..., ..., "keyword_only")),
                          ...))

        # Ensure unittest.mock.ANY & similar do not get picked up as a Placeholder
        self.assertEqual(self.signature(partial(test, unittest.mock.ANY, 1, c=2)),
                         ((('c', 2, ..., "keyword_only"),
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

        # With Placeholder
        p = partial(test, Placeholder, Placeholder, 1, b=0, test=1)
        self.assertEqual(self.signature(p),
                         ((('a', ..., ..., "positional_only"),
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


        def foo(a, b, /, c, d, **kwargs):
            pass
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

        # Positional only With Placeholder
        p = partial(foo, Placeholder, 1, c=0, d=1)
        self.assertEqual(self.signature(p),
                         ((('a', ..., ..., "positional_only"),
                           ('c', 0, ..., "keyword_only"),
                           ('d', 1, ..., "keyword_only"),
                           ('kwargs', ..., ..., "var_keyword")),
                          ...))

        # Optionals Positional With Placeholder
        def foo(a=0, b=1, /, c=2, d=3):
            pass

        # Positional
        p = partial(foo, Placeholder, 1, c=0, d=1)
        self.assertEqual(self.signature(p),
                         ((('a', ..., ..., "positional_only"),
                           ('c', 0, ..., "keyword_only"),
                           ('d', 1, ..., "keyword_only")),
                          ...))

        # Positional or Keyword - transformed to positional
        p = partial(foo, Placeholder, 1, Placeholder, 1)
        self.assertEqual(self.signature(p),
                         ((('a', ..., ..., "positional_only"),
                           ('c', ..., ..., "positional_only")),
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
            def test(it, a, b, *, c) -> 'spam':
                pass
            ham = partialmethod(test, c=1)
            bar = partialmethod(test, functools.Placeholder, 1, c=1)

        self.assertEqual(self.signature(Spam.ham, eval_str=False),
                         ((('it', ..., ..., 'positional_or_keyword'),
                           ('a', ..., ..., 'positional_or_keyword'),
                           ('b', ..., ..., 'positional_or_keyword'),
                           ('c', 1, ..., 'keyword_only')),
                          'spam'))

        self.assertEqual(self.signature(Spam().ham, eval_str=False),
                         ((('a', ..., ..., 'positional_or_keyword'),
                           ('b', ..., ..., 'positional_or_keyword'),
                           ('c', 1, ..., 'keyword_only')),
                          'spam'))

        # With Placeholder
        self.assertEqual(self.signature(Spam.bar, eval_str=False),
                         ((('it', ..., ..., 'positional_only'),
                           ('a', ..., ..., 'positional_only'),
                           ('c', 1, ..., 'keyword_only')),
                          'spam'))
        self.assertEqual(self.signature(Spam().bar, eval_str=False),
                         ((('a', ..., ..., 'positional_only'),
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
        foo.__partialmethod__ = 'spam'
        self.assertEqual(str(inspect.signature(foo)), '(a)')

    def test_signature_on_decorated(self):
        def decorator(func):
            @functools.wraps(func)
            def wrapper(*args, **kwargs) -> int:
                return func(*args, **kwargs)
            return wrapper

        class Foo:
            @decorator
            def bar(self, a, b):
                pass

        bar = decorator(Foo().bar)

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

        self.assertEqual(self.signature(bar),
                         ((('a', ..., ..., "positional_or_keyword"),
                           ('b', ..., ..., "positional_or_keyword")),
                          ...))

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

        with self.subTest('classmethod'):
            class CM(type):
                @classmethod
                def __call__(cls, a):
                    return a
            class C(metaclass=CM):
                def __init__(self, b):
                    pass

            self.assertEqual(C(1), 1)
            self.assertEqual(self.signature(C),
                            ((('a', ..., ..., "positional_or_keyword"),),
                            ...))

        with self.subTest('staticmethod'):
            class CM(type):
                @staticmethod
                def __call__(a):
                    return a
            class C(metaclass=CM):
                def __init__(self, b):
                    pass

            self.assertEqual(C(1), 1)
            self.assertEqual(self.signature(C),
                            ((('a', ..., ..., "positional_or_keyword"),),
                            ...))

        with self.subTest('MethodType'):
            class A:
                def call(self, a):
                    return a
            class CM(type):
                __call__ = A().call
            class C(metaclass=CM):
                def __init__(self, b):
                    pass

            self.assertEqual(C(1), 1)
            self.assertEqual(self.signature(C),
                            ((('a', ..., ..., "positional_or_keyword"),),
                            ...))

        with self.subTest('partial'):
            class CM(type):
                __call__ = functools.partial(lambda x, a, b: (x, a, b), 2)
            class C(metaclass=CM):
                def __init__(self, c):
                    pass

            self.assertEqual(C(1), (2, C, 1))
            self.assertEqual(self.signature(C),
                            ((('b', ..., ..., "positional_or_keyword"),),
                            ...))

        with self.subTest('partialmethod'):
            class CM(type):
                __call__ = functools.partialmethod(lambda self, x, a: (x, a), 2)
            class C(metaclass=CM):
                def __init__(self, b):
                    pass

            self.assertEqual(C(1), (2, 1))
            self.assertEqual(self.signature(C),
                            ((('a', ..., ..., "positional_or_keyword"),),
                            ...))

        with self.subTest('BuiltinMethodType'):
            class CM(type):
                __call__ = ':'.join
            class C(metaclass=CM):
                def __init__(self, b):
                    pass

            self.assertEqual(C(['a', 'bc']), 'a:bc')
            # BUG: Returns '<Signature (b)>'
            with self.assertRaises(AssertionError):
                self.assertEqual(self.signature(C), self.signature(''.join))

        with self.subTest('MethodWrapperType'):
            class CM(type):
                __call__ = (2).__pow__
            class C(metaclass=CM):
                def __init__(self, b):
                    pass

            self.assertEqual(C(3), 8)
            self.assertEqual(C(3, 7), 1)
            if not support.MISSING_C_DOCSTRINGS:
                # BUG: Returns '<Signature (b)>'
                with self.assertRaises(AssertionError):
                    self.assertEqual(self.signature(C), self.signature((0).__pow__))

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

    def test_signature_on_class_with_decorated_new(self):
        def identity(func):
            @functools.wraps(func)
            def wrapped(*args, **kwargs):
                return func(*args, **kwargs)
            return wrapped

        class Foo:
            @identity
            def __new__(cls, a, b):
                pass

        self.assertEqual(self.signature(Foo),
                         ((('a', ..., ..., "positional_or_keyword"),
                           ('b', ..., ..., "positional_or_keyword")),
                          ...))

        self.assertEqual(self.signature(Foo.__new__),
                         ((('cls', ..., ..., "positional_or_keyword"),
                           ('a', ..., ..., "positional_or_keyword"),
                           ('b', ..., ..., "positional_or_keyword")),
                          ...))

        class Bar:
            __new__ = identity(object.__new__)

        varargs_signature = (
            (('args', ..., ..., 'var_positional'),
             ('kwargs', ..., ..., 'var_keyword')),
            ...,
        )

        self.assertEqual(self.signature(Bar), ((), ...))
        self.assertEqual(self.signature(Bar.__new__), varargs_signature)
        self.assertEqual(self.signature(Bar, follow_wrapped=False),
                         varargs_signature)
        self.assertEqual(self.signature(Bar.__new__, follow_wrapped=False),
                         varargs_signature)

    def test_signature_on_class_with_init(self):
        class C:
            def __init__(self, b):
                pass

        C(1)  # does not raise
        self.assertEqual(self.signature(C),
                        ((('b', ..., ..., "positional_or_keyword"),),
                        ...))

        with self.subTest('classmethod'):
            class C:
                @classmethod
                def __init__(cls, b):
                    pass

            C(1)  # does not raise
            self.assertEqual(self.signature(C),
                            ((('b', ..., ..., "positional_or_keyword"),),
                            ...))

        with self.subTest('staticmethod'):
            class C:
                @staticmethod
                def __init__(b):
                    pass

            C(1)  # does not raise
            self.assertEqual(self.signature(C),
                            ((('b', ..., ..., "positional_or_keyword"),),
                            ...))

        with self.subTest('MethodType'):
            class A:
                def call(self, a):
                    pass
            class C:
                __init__ = A().call

            C(1)  # does not raise
            self.assertEqual(self.signature(C),
                            ((('a', ..., ..., "positional_or_keyword"),),
                            ...))

        with self.subTest('partial'):
            class C:
                __init__ = functools.partial(lambda x, a, b: None, 2)

            C(1)  # does not raise
            self.assertEqual(self.signature(C),
                            ((('b', ..., ..., "positional_or_keyword"),),
                            ...))

        with self.subTest('partialmethod'):
            class C:
                def _init(self, x, a):
                    self.a = (x, a)
                __init__ = functools.partialmethod(_init, 2)

            self.assertEqual(C(1).a, (2, 1))
            self.assertEqual(self.signature(C),
                            ((('a', ..., ..., "positional_or_keyword"),),
                            ...))

    def test_signature_on_class_with_new(self):
        with self.subTest('FunctionType'):
            class C:
                def __new__(cls, a):
                    return a

            self.assertEqual(C(1), 1)
            self.assertEqual(self.signature(C),
                            ((('a', ..., ..., "positional_or_keyword"),),
                            ...))

        with self.subTest('classmethod'):
            class C:
                @classmethod
                def __new__(cls, cls2, a):
                    return a

            self.assertEqual(C(1), 1)
            self.assertEqual(self.signature(C),
                            ((('a', ..., ..., "positional_or_keyword"),),
                            ...))

        with self.subTest('staticmethod'):
            class C:
                @staticmethod
                def __new__(cls, a):
                    return a

            self.assertEqual(C(1), 1)
            self.assertEqual(self.signature(C),
                            ((('a', ..., ..., "positional_or_keyword"),),
                            ...))

        with self.subTest('MethodType'):
            class A:
                def call(self, cls, a):
                    return a
            class C:
                __new__ = A().call

            self.assertEqual(C(1), 1)
            self.assertEqual(self.signature(C),
                            ((('a', ..., ..., "positional_or_keyword"),),
                            ...))

        with self.subTest('partial'):
            class C:
                __new__ = functools.partial(lambda x, cls, a: (x, a), 2)

            self.assertEqual(C(1), (2, 1))
            self.assertEqual(self.signature(C),
                            ((('a', ..., ..., "positional_or_keyword"),),
                            ...))

        with self.subTest('partialmethod'):
            class C:
                __new__ = functools.partialmethod(lambda cls, x, a: (x, a), 2)

            self.assertEqual(C(1), (2, 1))
            self.assertEqual(self.signature(C),
                            ((('a', ..., ..., "positional_or_keyword"),),
                            ...))

        with self.subTest('BuiltinMethodType'):
            class C:
                __new__ = str.__subclasscheck__

            self.assertEqual(C(), False)
            # TODO: Support BuiltinMethodType
            # self.assertEqual(self.signature(C), ((), ...))
            self.assertRaises(ValueError, self.signature, C)

        with self.subTest('MethodWrapperType'):
            class C:
                __new__ = type.__or__.__get__(int, type)

            self.assertEqual(C(), C | int)
            # TODO: Support MethodWrapperType
            # self.assertEqual(self.signature(C), ((), ...))
            self.assertRaises(ValueError, self.signature, C)

        # TODO: Test ClassMethodDescriptorType

        with self.subTest('MethodDescriptorType'):
            class C:
                __new__ = type.__dict__['__subclasscheck__']

            self.assertEqual(C(C), True)
            self.assertEqual(self.signature(C), self.signature(C.__subclasscheck__))

        with self.subTest('WrapperDescriptorType'):
            class C:
                __new__ = type.__or__

            self.assertEqual(C(int), C | int)
            # TODO: Support WrapperDescriptorType
            # self.assertEqual(self.signature(C), self.signature(C.__or__))
            self.assertRaises(ValueError, self.signature, C)

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
        self.assertEqual(C('A', (), {}).__name__, 'A')
        # TODO: Support type.
        with self.assertRaisesRegex(ValueError, "callable.*is not supported"):
            self.assertEqual(inspect.signature(C), None)
        self.assertEqual(D('A', (), {}).__name__, 'A')
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

        with self.subTest('classmethod'):
            class C:
                @classmethod
                def __call__(cls, a):
                    pass

            self.assertEqual(self.signature(C()),
                            ((('a', ..., ..., "positional_or_keyword"),),
                            ...))

        with self.subTest('staticmethod'):
            class C:
                @staticmethod
                def __call__(a):
                    pass

            self.assertEqual(self.signature(C()),
                            ((('a', ..., ..., "positional_or_keyword"),),
                            ...))

        with self.subTest('MethodType'):
            class A:
                def call(self, a):
                    return a
            class C:
                __call__ = A().call

            self.assertEqual(C()(1), 1)
            self.assertEqual(self.signature(C()),
                            ((('a', ..., ..., "positional_or_keyword"),),
                            ...))

        with self.subTest('partial'):
            class C:
                __call__ = functools.partial(lambda x, a, b: (x, a, b), 2)

            c = C()
            self.assertEqual(c(1), (2, c, 1))
            self.assertEqual(self.signature(C()),
                            ((('b', ..., ..., "positional_or_keyword"),),
                            ...))

        with self.subTest('partialmethod'):
            class C:
                __call__ = functools.partialmethod(lambda self, x, a: (x, a), 2)

            self.assertEqual(C()(1), (2, 1))
            self.assertEqual(self.signature(C()),
                            ((('a', ..., ..., "positional_or_keyword"),),
                            ...))

        with self.subTest('BuiltinMethodType'):
            class C:
                __call__ = ':'.join

            self.assertEqual(C()(['a', 'bc']), 'a:bc')
            self.assertEqual(self.signature(C()), self.signature(''.join))

        with self.subTest('MethodWrapperType'):
            class C:
                __call__ = (2).__pow__

            self.assertEqual(C()(3), 8)
            if not support.MISSING_C_DOCSTRINGS:
                self.assertEqual(self.signature(C()), self.signature((0).__pow__))

        with self.subTest('ClassMethodDescriptorType'):
            class C(dict):
                __call__ = dict.__dict__['fromkeys']

            res = C()([1, 2], 3)
            self.assertEqual(res, {1: 3, 2: 3})
            self.assertEqual(type(res), C)
            if not support.MISSING_C_DOCSTRINGS:
                self.assertEqual(self.signature(C()), self.signature(dict.fromkeys))

        with self.subTest('MethodDescriptorType'):
            class C(str):
                __call__ = str.join

            self.assertEqual(C(':')(['a', 'bc']), 'a:bc')
            self.assertEqual(self.signature(C()), self.signature(''.join))

        with self.subTest('WrapperDescriptorType'):
            class C(int):
                __call__ = int.__pow__

            self.assertEqual(C(2)(3), 8)
            if not support.MISSING_C_DOCSTRINGS:
                self.assertEqual(self.signature(C()), self.signature((0).__pow__))

        with self.subTest('MemberDescriptorType'):
            class C:
                __slots__ = '__call__'
            c = C()
            c.__call__ = lambda a: a
            self.assertEqual(c(1), 1)
            self.assertEqual(self.signature(c),
                            ((('a', ..., ..., "positional_or_keyword"),),
                            ...))

    def test_signature_on_callable_objects_with_text_signature_attr(self):
        class C:
            __text_signature__ = '(a, /, b, c=True)'
            def __call__(self, *args, **kwargs):
                pass

        if not support.MISSING_C_DOCSTRINGS:
            self.assertEqual(self.signature(C), ((), ...))
        self.assertEqual(self.signature(C()),
                         ((('a', ..., ..., "positional_only"),
                           ('b', ..., ..., "positional_or_keyword"),
                           ('c', True, ..., "positional_or_keyword"),
                          ),
                          ...))

        c = C()
        c.__text_signature__ = '(x, y)'
        self.assertEqual(self.signature(c),
                         ((('x', ..., ..., "positional_or_keyword"),
                           ('y', ..., ..., "positional_or_keyword"),
                          ),
                          ...))

    def test_signature_on_wrapper(self):
        class Wrapper:
            def __call__(self, b):
                pass
        wrapper = Wrapper()
        wrapper.__wrapped__ = lambda a: None
        self.assertEqual(self.signature(wrapper),
                         ((('a', ..., ..., "positional_or_keyword"),),
                          ...))
        # wrapper loop:
        wrapper = Wrapper()
        wrapper.__wrapped__ = wrapper
        with self.assertRaisesRegex(ValueError, 'wrapper loop'):
            self.signature(wrapper)

    def test_signature_on_lambdas(self):
        self.assertEqual(self.signature((lambda a=10: a)),
                         ((('a', 10, ..., "positional_or_keyword"),),
                          ...))

    def test_signature_on_mocks(self):
        # https://github.com/python/cpython/issues/96127
        for mock in (
            unittest.mock.Mock(),
            unittest.mock.AsyncMock(),
            unittest.mock.MagicMock(),
        ):
            with self.subTest(mock=mock):
                self.assertEqual(str(inspect.signature(mock)), '(*args, **kwargs)')

    def test_signature_on_noncallable_mocks(self):
        for mock in (
            unittest.mock.NonCallableMock(),
            unittest.mock.NonCallableMagicMock(),
        ):
            with self.subTest(mock=mock):
                with self.assertRaises(TypeError):
                    inspect.signature(mock)

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
        self.assertEqual(str(inspect.signature(foo)),
                         inspect.signature(foo).format())

        def foo(a:int=1, *args, b, c=None, **kwargs) -> 42:
            pass
        self.assertEqual(str(inspect.signature(foo)),
                         '(a: int = 1, *args, b, c=None, **kwargs) -> 42')
        self.assertEqual(str(inspect.signature(foo)),
                         inspect.signature(foo).format())

        def foo():
            pass
        self.assertEqual(str(inspect.signature(foo)), '()')
        self.assertEqual(str(inspect.signature(foo)),
                         inspect.signature(foo).format())

        def foo(a: list[str]) -> tuple[str, float]:
            pass
        self.assertEqual(str(inspect.signature(foo)),
                         '(a: list[str]) -> tuple[str, float]')
        self.assertEqual(str(inspect.signature(foo)),
                         inspect.signature(foo).format())

        from typing import Tuple
        def foo(a: list[str]) -> Tuple[str, float]:
            pass
        self.assertEqual(str(inspect.signature(foo)),
                         '(a: list[str]) -> Tuple[str, float]')
        self.assertEqual(str(inspect.signature(foo)),
                         inspect.signature(foo).format())

        def foo(x: undef):
            pass
        sig = inspect.signature(foo, annotation_format=Format.FORWARDREF)
        self.assertEqual(str(sig), '(x: undef)')

    def test_signature_str_positional_only(self):
        P = inspect.Parameter
        S = inspect.Signature

        def test(a_po, /, *, b, **kwargs):
            return a_po, kwargs

        self.assertEqual(str(inspect.signature(test)),
                         '(a_po, /, *, b, **kwargs)')
        self.assertEqual(str(inspect.signature(test)),
                         inspect.signature(test).format())

        test = S(parameters=[P('foo', P.POSITIONAL_ONLY)])
        self.assertEqual(str(test), '(foo, /)')
        self.assertEqual(str(test), test.format())

        test = S(parameters=[P('foo', P.POSITIONAL_ONLY),
                             P('bar', P.VAR_KEYWORD)])
        self.assertEqual(str(test), '(foo, /, **bar)')
        self.assertEqual(str(test), test.format())

        test = S(parameters=[P('foo', P.POSITIONAL_ONLY),
                             P('bar', P.VAR_POSITIONAL)])
        self.assertEqual(str(test), '(foo, /, *bar)')
        self.assertEqual(str(test), test.format())

    def test_signature_format(self):
        from typing import Annotated, Literal

        def func(x: Annotated[int, 'meta'], y: Literal['a', 'b'], z: 'LiteralString'):
            pass

        expected_singleline = "(x: Annotated[int, 'meta'], y: Literal['a', 'b'], z: 'LiteralString')"
        expected_multiline = """(
    x: Annotated[int, 'meta'],
    y: Literal['a', 'b'],
    z: 'LiteralString'
)"""
        self.assertEqual(
            inspect.signature(func).format(),
            expected_singleline,
        )
        self.assertEqual(
            inspect.signature(func).format(max_width=None),
            expected_singleline,
        )
        self.assertEqual(
            inspect.signature(func).format(max_width=len(expected_singleline)),
            expected_singleline,
        )
        self.assertEqual(
            inspect.signature(func).format(max_width=len(expected_singleline) - 1),
            expected_multiline,
        )
        self.assertEqual(
            inspect.signature(func).format(max_width=0),
            expected_multiline,
        )
        self.assertEqual(
            inspect.signature(func).format(max_width=-1),
            expected_multiline,
        )

    def test_signature_format_all_arg_types(self):
        from typing import Annotated, Literal

        def func(
            x: Annotated[int, 'meta'],
            /,
            y: Literal['a', 'b'],
            *,
            z: 'LiteralString',
            **kwargs: object,
        ) -> None:
            pass

        expected_multiline = """(
    x: Annotated[int, 'meta'],
    /,
    y: Literal['a', 'b'],
    *,
    z: 'LiteralString',
    **kwargs: object
) -> None"""
        self.assertEqual(
            inspect.signature(func).format(max_width=-1),
            expected_multiline,
        )

    def test_signature_format_unquote(self):
        def func(x: 'int') -> 'str': ...

        self.assertEqual(
            inspect.signature(func).format(),
            "(x: 'int') -> 'str'"
        )
        self.assertEqual(
            inspect.signature(func).format(quote_annotation_strings=False),
            "(x: int) -> str"
        )

    def test_signature_replace_parameters(self):
        def test(a, b) -> 42:
            pass

        sig = inspect.signature(test)
        parameters = sig.parameters
        sig = sig.replace(parameters=list(parameters.values())[1:])
        self.assertEqual(list(sig.parameters), ['b'])
        self.assertEqual(sig.parameters['b'], parameters['b'])
        self.assertEqual(sig.return_annotation, 42)
        sig = sig.replace(parameters=())
        self.assertEqual(dict(sig.parameters), {})

        sig = inspect.signature(test)
        parameters = sig.parameters
        sig = copy.replace(sig, parameters=list(parameters.values())[1:])
        self.assertEqual(list(sig.parameters), ['b'])
        self.assertEqual(sig.parameters['b'], parameters['b'])
        self.assertEqual(sig.return_annotation, 42)
        sig = copy.replace(sig, parameters=())
        self.assertEqual(dict(sig.parameters), {})

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

        sig = inspect.signature(test)
        sig = copy.replace(sig, return_annotation=None)
        self.assertIs(sig.return_annotation, None)
        sig = copy.replace(sig, return_annotation=sig.empty)
        self.assertIs(sig.return_annotation, sig.empty)
        sig = copy.replace(sig, return_annotation=42)
        self.assertEqual(sig.return_annotation, 42)
        self.assertEqual(sig, inspect.signature(test))

    def test_signature_replaced(self):
        def test():
            pass

        spam_param = inspect.Parameter('spam', inspect.Parameter.POSITIONAL_ONLY)
        sig = test.__signature__ = inspect.Signature(parameters=(spam_param,))
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

    @unittest.skipIf(MISSING_C_DOCSTRINGS,
                     "Signature information for builtins requires docstrings")
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

                if not MISSING_C_DOCSTRINGS:
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

    def test_signature_annotation_format(self):
        ida = inspect_deferred_annotations
        sig = inspect.Signature
        par = inspect.Parameter
        PORK = inspect.Parameter.POSITIONAL_OR_KEYWORD
        for signature_func in (inspect.signature, inspect.Signature.from_callable):
            with self.subTest(signature_func=signature_func):
                self.assertEqual(
                    signature_func(ida.f, annotation_format=Format.STRING),
                    sig([par("x", PORK, annotation="undefined")])
                )
                s1 = signature_func(ida.f, annotation_format=Format.FORWARDREF)
                s2 = sig([par("x", PORK, annotation=EqualToForwardRef("undefined", owner=ida.f))])
                #breakpoint()
                self.assertEqual(
                    signature_func(ida.f, annotation_format=Format.FORWARDREF),
                    sig([par("x", PORK, annotation=EqualToForwardRef("undefined", owner=ida.f))])
                )
                with self.assertRaisesRegex(NameError, "undefined"):
                    signature_func(ida.f, annotation_format=Format.VALUE)
                with self.assertRaisesRegex(NameError, "undefined"):
                    signature_func(ida.f)

    def test_signature_deferred_annotations(self):
        def f(x: undef):
            pass

        class C:
            x: undef

            def __init__(self, x: undef):
                self.x = x

        sig = inspect.signature(f, annotation_format=Format.FORWARDREF)
        self.assertEqual(list(sig.parameters), ['x'])
        sig = inspect.signature(C, annotation_format=Format.FORWARDREF)
        self.assertEqual(list(sig.parameters), ['x'])

        class CallableWrapper:
            def __init__(self, func):
                self.func = func
                self.__annotate__ = func.__annotate__

            def __call__(self, *args, **kwargs):
                return self.func(*args, **kwargs)

            @property
            def __annotations__(self):
                return self.__annotate__(Format.VALUE)

        cw = CallableWrapper(f)
        sig = inspect.signature(cw, annotation_format=Format.FORWARDREF)
        self.assertEqual(list(sig.parameters), ['args', 'kwargs'])

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

    def test_signature_on_derived_classes(self):
        # gh-105080: Make sure that signatures are consistent on derived classes

        class B:
            def __new__(self, *args, **kwargs):
                return super().__new__(self)
            def __init__(self, value):
                self.value = value

        class D1(B):
            def __init__(self, value):
                super().__init__(value)

        class D2(D1):
            pass

        self.assertEqual(inspect.signature(D2), inspect.signature(D1))

    def test_signature_on_non_comparable(self):
        class NoncomparableCallable:
            def __call__(self, a):
                pass
            def __eq__(self, other):
                1/0
        self.assertEqual(self.signature(NoncomparableCallable()),
                         ((('a', ..., ..., 'positional_or_keyword'),),
                          ...))


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

        with self.assertRaisesRegex(ValueError, 'not a valid parameter name'):
            inspect.Parameter('from', kind=inspect.Parameter.VAR_KEYWORD)

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

        self.assertStartsWith(repr(p), '<Parameter')
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

        self.assertIsNot(p.replace(), p)
        self.assertEqual(p.replace(), p)
        self.assertIsNot(copy.replace(p), p)
        self.assertEqual(copy.replace(p), p)

        p2 = p.replace(annotation=1)
        self.assertEqual(p2.annotation, 1)
        p2 = p2.replace(annotation=p2.empty)
        self.assertEqual(p2, p)
        p3 = copy.replace(p, annotation=1)
        self.assertEqual(p3.annotation, 1)
        p3 = copy.replace(p3, annotation=p3.empty)
        self.assertEqual(p3, p)

        p2 = p2.replace(name='bar')
        self.assertEqual(p2.name, 'bar')
        self.assertNotEqual(p2, p)
        p3 = copy.replace(p3, name='bar')
        self.assertEqual(p3.name, 'bar')
        self.assertNotEqual(p3, p)

        with self.assertRaisesRegex(ValueError,
                                    'name is a required attribute'):
            p2 = p2.replace(name=p2.empty)
        with self.assertRaisesRegex(ValueError,
                                    'name is a required attribute'):
            p3 = copy.replace(p3, name=p3.empty)

        p2 = p2.replace(name='foo', default=None)
        self.assertIs(p2.default, None)
        self.assertNotEqual(p2, p)
        p3 = copy.replace(p3, name='foo', default=None)
        self.assertIs(p3.default, None)
        self.assertNotEqual(p3, p)

        p2 = p2.replace(name='foo', default=p2.empty)
        self.assertIs(p2.default, p2.empty)
        p3 = copy.replace(p3, name='foo', default=p3.empty)
        self.assertIs(p3.default, p3.empty)

        p2 = p2.replace(default=42, kind=p2.POSITIONAL_OR_KEYWORD)
        self.assertEqual(p2.kind, p2.POSITIONAL_OR_KEYWORD)
        self.assertNotEqual(p2, p)
        p3 = copy.replace(p3, default=42, kind=p3.POSITIONAL_OR_KEYWORD)
        self.assertEqual(p3.kind, p3.POSITIONAL_OR_KEYWORD)
        self.assertNotEqual(p3, p)

        with self.assertRaisesRegex(ValueError,
                                    "value <class 'inspect._empty'> "
                                    "is not a valid Parameter.kind"):
            p2 = p2.replace(kind=p2.empty)
        with self.assertRaisesRegex(ValueError,
                                    "value <class 'inspect._empty'> "
                                    "is not a valid Parameter.kind"):
            p3 = copy.replace(p3, kind=p3.empty)

        p2 = p2.replace(kind=p2.KEYWORD_ONLY)
        self.assertEqual(p2, p)
        p3 = copy.replace(p3, kind=p3.KEYWORD_ONLY)
        self.assertEqual(p3, p)

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
        # Prevent unexpected success of assertRaises(TypeError, ...)
        try:
            return func(*ba.args, **ba.kwargs)
        except TypeError as e:
            raise AssertionError from e

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
                                     "missing a required keyword-only "
                                     "argument: 'bar'"):
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
        def test(a_po, b_po, c_po=3, /, foo=42, *, bar=50, **kwargs):
            return a_po, b_po, c_po, foo, bar, kwargs

        self.assertEqual(self.call(test, 1, 2, 4, 5, bar=6),
                         (1, 2, 4, 5, 6, {}))

        self.assertEqual(self.call(test, 1, 2),
                         (1, 2, 3, 42, 50, {}))

        self.assertEqual(self.call(test, 1, 2, foo=4, bar=5),
                         (1, 2, 3, 4, 5, {}))

        self.assertEqual(self.call(test, 1, 2, foo=4, bar=5, c_po=10),
                         (1, 2, 3, 4, 5, {'c_po': 10}))

        self.assertEqual(self.call(test, 1, 2, 30, c_po=31, foo=4, bar=5),
                         (1, 2, 30, 4, 5, {'c_po': 31}))

        self.assertEqual(self.call(test, 1, 2, 30, foo=4, bar=5, c_po=31),
                         (1, 2, 30, 4, 5, {'c_po': 31}))

        self.assertEqual(self.call(test, 1, 2, c_po=4),
                         (1, 2, 3, 42, 50, {'c_po': 4}))

        with self.assertRaisesRegex(TypeError, "missing a required positional-only argument: 'a_po'"):
            self.call(test, a_po=1, b_po=2)

        def without_var_kwargs(c_po=3, d_po=4, /):
            return c_po, d_po

        with self.assertRaisesRegex(
            TypeError,
            "positional-only arguments passed as keyword arguments: 'c_po, d_po'",
        ):
            self.call(without_var_kwargs, c_po=33, d_po=44)

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
        # Issue #19611: getcallargs should work with comprehensions
        def make_set():
            return set(z * z for z in range(5))
        gencomp_code = make_set.__code__.co_consts[0]
        gencomp_func = types.FunctionType(gencomp_code, {})

        iterator = iter(range(5))
        self.assertEqual(set(self.call(gencomp_func, iterator)), {0, 1, 4, 9, 16})

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
    def _strip_non_python_syntax(self, input,
        clean_signature, self_parameter):
        computed_clean_signature, \
            computed_self_parameter = \
            inspect._signature_strip_non_python_syntax(input)
        self.assertEqual(computed_clean_signature, clean_signature)
        self.assertEqual(computed_self_parameter, self_parameter)

    def test_signature_strip_non_python_syntax(self):
        self._strip_non_python_syntax(
            "($module, /, path, mode, *, dir_fd=None, " +
                "effective_ids=False,\n       follow_symlinks=True)",
            "(module, /, path, mode, *, dir_fd=None, " +
                "effective_ids=False, follow_symlinks=True)",
            0)

        self._strip_non_python_syntax(
            "($module, word, salt, /)",
            "(module, word, salt, /)",
            0)

        self._strip_non_python_syntax(
            "(x, y=None, z=None, /)",
            "(x, y=None, z=None, /)",
            None)

        self._strip_non_python_syntax(
            "(x, y=None, z=None)",
            "(x, y=None, z=None)",
            None)

        self._strip_non_python_syntax(
            "(x,\n    y=None,\n      z = None  )",
            "(x, y=None, z=None)",
            None)

        self._strip_non_python_syntax(
            "",
            "",
            None)

        self._strip_non_python_syntax(
            None,
            None,
            None)

class TestSignatureDefinitions(unittest.TestCase):
    # This test case provides a home for checking that particular APIs
    # have signatures available for introspection

    @staticmethod
    def is_public(name):
        return not name.startswith('_') or name.startswith('__') and name.endswith('__')

    @cpython_only
    @unittest.skipIf(MISSING_C_DOCSTRINGS,
                     "Signature information for builtins requires docstrings")
    def _test_module_has_signatures(self, module,
                no_signature=(), unsupported_signature=(),
                methods_no_signature={}, methods_unsupported_signature={},
                good_exceptions=()):
        # This checks all builtin callables in CPython have signatures
        # A few have signatures Signature can't yet handle, so we skip those
        # since they will have to wait until PEP 457 adds the required
        # introspection support to the inspect module
        # Some others also haven't been converted yet for various other
        # reasons, so we also skip those for the time being, but design
        # the test to fail in order to indicate when it needs to be
        # updated.
        no_signature = no_signature or set()
        # Check the signatures we expect to be there
        ns = vars(module)
        try:
            names = set(module.__all__)
        except AttributeError:
            names = set(name for name in ns if self.is_public(name))
        for name, obj in sorted(ns.items()):
            if name not in names:
                continue
            if not callable(obj):
                continue
            if (isinstance(obj, type) and
                issubclass(obj, BaseException) and
                name not in good_exceptions):
                no_signature.add(name)
            if name not in no_signature and name not in unsupported_signature:
                with self.subTest('supported', builtin=name):
                    self.assertIsNotNone(inspect.signature(obj))
            if isinstance(obj, type):
                with self.subTest(type=name):
                    self._test_builtin_methods_have_signatures(obj,
                            methods_no_signature.get(name, ()),
                            methods_unsupported_signature.get(name, ()))
        # Check callables that haven't been converted don't claim a signature
        # This ensures this test will start failing as more signatures are
        # added, so the affected items can be moved into the scope of the
        # regression test above
        for name in no_signature:
            with self.subTest('none', builtin=name):
                obj = ns[name]
                self.assertIsNone(obj.__text_signature__)
                self.assertRaises(ValueError, inspect.signature, obj)
        for name in unsupported_signature:
            with self.subTest('unsupported', builtin=name):
                obj = ns[name]
                self.assertIsNotNone(obj.__text_signature__)
                self.assertRaises(ValueError, inspect.signature, obj)

    def _test_builtin_methods_have_signatures(self, cls, no_signature, unsupported_signature):
        ns = vars(cls)
        for name in ns:
            obj = getattr(cls, name, None)
            if not callable(obj) or isinstance(obj, type):
                continue
            if name not in no_signature and name not in unsupported_signature:
                with self.subTest('supported', method=name):
                    self.assertIsNotNone(inspect.signature(obj))
        for name in no_signature:
            with self.subTest('none', method=name):
                self.assertIsNone(getattr(cls, name).__text_signature__)
                self.assertRaises(ValueError, inspect.signature, getattr(cls, name))
        for name in unsupported_signature:
            with self.subTest('unsupported', method=name):
                self.assertIsNotNone(getattr(cls, name).__text_signature__)
                self.assertRaises(ValueError, inspect.signature, getattr(cls, name))

    def test_builtins_have_signatures(self):
        no_signature = {'type', 'super', 'bytearray', 'bytes', 'dict', 'int', 'str'}
        # These need PEP 457 groups
        needs_groups = {"range", "slice", "dir", "getattr",
                        "next", "iter", "vars"}
        no_signature |= needs_groups
        # These have unrepresentable parameter default values of NULL
        unsupported_signature = {"anext"}
        # These need *args support in Argument Clinic
        needs_varargs = {"min", "max", "__build_class__"}
        no_signature |= needs_varargs

        methods_no_signature = {
            'dict': {'update'},
            'object': {'__class__'},
        }
        methods_unsupported_signature = {
            'bytearray': {'count', 'endswith', 'find', 'hex', 'index', 'rfind', 'rindex', 'startswith'},
            'bytes': {'count', 'endswith', 'find', 'hex', 'index', 'rfind', 'rindex', 'startswith'},
            'dict': {'pop'},
            'memoryview': {'cast', 'hex'},
            'str': {'count', 'endswith', 'find', 'index', 'maketrans', 'rfind', 'rindex', 'startswith'},
        }
        self._test_module_has_signatures(builtins,
                no_signature, unsupported_signature,
                methods_no_signature, methods_unsupported_signature)

    def test_types_module_has_signatures(self):
        unsupported_signature = {'CellType'}
        methods_no_signature = {
            'AsyncGeneratorType': {'athrow'},
            'CoroutineType': {'throw'},
            'GeneratorType': {'throw'},
            'FrameLocalsProxyType': {'setdefault', 'pop', 'get'},
        }
        self._test_module_has_signatures(types,
                unsupported_signature=unsupported_signature,
                methods_no_signature=methods_no_signature)

    def test_sys_module_has_signatures(self):
        no_signature = {'getsizeof', 'set_asyncgen_hooks'}
        no_signature |= {name for name in ['getobjects']
                         if hasattr(sys, name)}
        self._test_module_has_signatures(sys, no_signature)

    def test_abc_module_has_signatures(self):
        import abc
        self._test_module_has_signatures(abc)

    def test_atexit_module_has_signatures(self):
        import atexit
        self._test_module_has_signatures(atexit)

    def test_codecs_module_has_signatures(self):
        import codecs
        methods_no_signature = {'StreamReader': {'charbuffertype'}}
        self._test_module_has_signatures(codecs,
                methods_no_signature=methods_no_signature)

    def test_collections_module_has_signatures(self):
        no_signature = {'OrderedDict', 'defaultdict'}
        unsupported_signature = {'deque'}
        methods_no_signature = {
            'OrderedDict': {'update'},
        }
        methods_unsupported_signature = {
            'deque': {'index'},
            'OrderedDict': {'pop'},
            'UserString': {'maketrans'},
        }
        self._test_module_has_signatures(collections,
                no_signature, unsupported_signature,
                methods_no_signature, methods_unsupported_signature)

    def test_collections_abc_module_has_signatures(self):
        import collections.abc
        self._test_module_has_signatures(collections.abc)

    def test_datetime_module_has_signatures(self):
        # Only test if the C implementation is available.
        import_helper.import_module('_datetime')
        import datetime
        no_signature = {'tzinfo'}
        unsupported_signature = {'timezone'}
        methods_unsupported_signature = {
            'date': {'replace'},
            'time': {'replace'},
            'datetime': {'replace', 'combine'},
        }
        self._test_module_has_signatures(datetime,
                no_signature, unsupported_signature,
                methods_unsupported_signature=methods_unsupported_signature)

    def test_errno_module_has_signatures(self):
        import errno
        self._test_module_has_signatures(errno)

    def test_faulthandler_module_has_signatures(self):
        import faulthandler
        unsupported_signature = {'dump_traceback', 'dump_traceback_later', 'enable', 'dump_c_stack'}
        unsupported_signature |= {name for name in ['register']
                                  if hasattr(faulthandler, name)}
        self._test_module_has_signatures(faulthandler, unsupported_signature=unsupported_signature)

    def test_functools_module_has_signatures(self):
        unsupported_signature = {"reduce"}
        self._test_module_has_signatures(functools, unsupported_signature=unsupported_signature)

    def test_gc_module_has_signatures(self):
        import gc
        no_signature = {'set_threshold'}
        self._test_module_has_signatures(gc, no_signature)

    def test_io_module_has_signatures(self):
        methods_no_signature = {
            'BufferedRWPair': {'read', 'peek', 'read1', 'readinto', 'readinto1', 'write'},
        }
        self._test_module_has_signatures(io,
                methods_no_signature=methods_no_signature)

    def test_itertools_module_has_signatures(self):
        import itertools
        no_signature = {'islice', 'repeat'}
        self._test_module_has_signatures(itertools, no_signature)

    def test_locale_module_has_signatures(self):
        import locale
        self._test_module_has_signatures(locale)

    def test_marshal_module_has_signatures(self):
        import marshal
        self._test_module_has_signatures(marshal)

    def test_operator_module_has_signatures(self):
        import operator
        self._test_module_has_signatures(operator)

    def test_os_module_has_signatures(self):
        unsupported_signature = {'chmod', 'utime'}
        unsupported_signature |= {name for name in
            ['get_terminal_size', 'link', 'posix_spawn', 'posix_spawnp',
             'register_at_fork', 'startfile']
            if hasattr(os, name)}
        self._test_module_has_signatures(os, unsupported_signature=unsupported_signature)

    def test_pwd_module_has_signatures(self):
        pwd = import_helper.import_module('pwd')
        self._test_module_has_signatures(pwd)

    def test_re_module_has_signatures(self):
        import re
        methods_no_signature = {'Match': {'group'}}
        self._test_module_has_signatures(re,
                methods_no_signature=methods_no_signature,
                good_exceptions={'error', 'PatternError'})

    def test_signal_module_has_signatures(self):
        import signal
        self._test_module_has_signatures(signal)

    def test_stat_module_has_signatures(self):
        import stat
        self._test_module_has_signatures(stat)

    def test_string_module_has_signatures(self):
        import string
        self._test_module_has_signatures(string)

    def test_symtable_module_has_signatures(self):
        import symtable
        self._test_module_has_signatures(symtable)

    def test_sysconfig_module_has_signatures(self):
        import sysconfig
        self._test_module_has_signatures(sysconfig)

    def test_threading_module_has_signatures(self):
        import threading
        self._test_module_has_signatures(threading)
        self.assertIsNotNone(inspect.signature(threading.__excepthook__))

    def test_thread_module_has_signatures(self):
        import _thread
        no_signature = {'RLock'}
        self._test_module_has_signatures(_thread, no_signature)

    def test_time_module_has_signatures(self):
        no_signature = {
            'asctime', 'ctime', 'get_clock_info', 'gmtime', 'localtime',
            'strftime', 'strptime'
        }
        no_signature |= {name for name in
            ['clock_getres', 'clock_settime', 'clock_settime_ns',
             'pthread_getcpuclockid']
            if hasattr(time, name)}
        self._test_module_has_signatures(time, no_signature)

    def test_tokenize_module_has_signatures(self):
        import tokenize
        self._test_module_has_signatures(tokenize)

    def test_tracemalloc_module_has_signatures(self):
        import tracemalloc
        self._test_module_has_signatures(tracemalloc)

    def test_typing_module_has_signatures(self):
        import typing
        no_signature = {'ParamSpec', 'ParamSpecArgs', 'ParamSpecKwargs',
                        'Text', 'TypeAliasType', 'TypeVar', 'TypeVarTuple'}
        methods_no_signature = {
            'Generic': {'__class_getitem__', '__init_subclass__'},
        }
        methods_unsupported_signature = {
            'Text': {'count', 'find', 'index', 'rfind', 'rindex', 'startswith', 'endswith', 'maketrans'},
        }
        self._test_module_has_signatures(typing, no_signature,
                methods_no_signature=methods_no_signature,
                methods_unsupported_signature=methods_unsupported_signature)

    def test_warnings_module_has_signatures(self):
        unsupported_signature = {'warn', 'warn_explicit'}
        self._test_module_has_signatures(warnings, unsupported_signature=unsupported_signature)

    def test_weakref_module_has_signatures(self):
        import weakref
        no_signature = {'ReferenceType', 'ref'}
        self._test_module_has_signatures(weakref, no_signature)

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

        func.__text_signature__ = '(self, a=1+2, b=4-3, c=1 | 3 | 16)'
        sig = inspect.signature(func)
        self.assertEqual(str(sig), '(self, a=3, b=1, c=19)')

        func.__text_signature__ = '(self, a=1,\nb=2,\n\n\n   c=3)'
        sig = inspect.signature(func)
        self.assertEqual(str(sig), '(self, a=1, b=2, c=3)')

        func.__text_signature__ = '(self, x=does_not_exist)'
        with self.assertRaises(ValueError):
            inspect.signature(func)
        func.__text_signature__ = '(self, x=sys, y=inspect)'
        with self.assertRaises(ValueError):
            inspect.signature(func)
        func.__text_signature__ = '(self, 123)'
        with self.assertRaises(ValueError):
            inspect.signature(func)

    @support.requires_docstrings
    def test_base_class_have_text_signature(self):
        # see issue 43118
        from test.typinganndata.ann_module7 import BufferedReader
        class MyBufferedReader(BufferedReader):
            """buffer reader class."""

        text_signature = BufferedReader.__text_signature__
        self.assertEqual(text_signature, '(raw, buffer_size=DEFAULT_BUFFER_SIZE)')
        sig = inspect.signature(MyBufferedReader)
        self.assertEqual(str(sig), '(raw, buffer_size=8192)')


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

    def test_wrapped_descriptor(self):
        self.assertIs(inspect.unwrap(NTimesUnwrappable), NTimesUnwrappable)
        self.assertIs(inspect.unwrap(staticmethod), staticmethod)
        self.assertIs(inspect.unwrap(classmethod), classmethod)
        self.assertIs(inspect.unwrap(staticmethod(classmethod)), classmethod)
        self.assertIs(inspect.unwrap(classmethod(staticmethod)), staticmethod)


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
        self.assertIn(module.__spec__.name, output)
        self.assertIn(module.__name__, output)
        self.assertIn(module.__spec__.origin, output)
        self.assertIn(module.__file__, output)
        self.assertIn(module.__spec__.cached, output)
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
        with ready_to_import('reload_bug', self.src_before) as (name, path):
            module = importlib.import_module(name)
            self.assertInspectEqual(path, module)
            with open(path, 'w', encoding='utf-8') as src:
                src.write(self.src_after)
            self.assertInspectEqual(path, module)


class TestRepl(unittest.TestCase):

    def spawn_repl(self, *args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, **kw):
        """Run the Python REPL with the given arguments.

        kw is extra keyword args to pass to subprocess.Popen. Returns a Popen
        object.
        """

        # TODO(picnixz): refactor this as it's used by test_repl.py

        # To run the REPL without using a terminal, spawn python with the command
        # line option '-i' and the process name set to '<stdin>'.
        # The directory of argv[0] must match the directory of the Python
        # executable for the Popen() call to python to succeed as the directory
        # path may be used by PyConfig_Get("module_search_paths") to build the
        # default module search path.
        stdin_fname = os.path.join(os.path.dirname(sys.executable), "<stdin>")
        cmd_line = [stdin_fname, '-E', '-i']
        cmd_line.extend(args)

        # Set TERM=vt100, for the rationale see the comments in spawn_python() of
        # test.support.script_helper.
        env = kw.setdefault('env', dict(os.environ))
        env['TERM'] = 'vt100'
        return subprocess.Popen(cmd_line,
                                executable=sys.executable,
                                text=True,
                                stdin=subprocess.PIPE,
                                stdout=stdout, stderr=stderr,
                                **kw)

    def run_on_interactive_mode(self, source):
        """Spawn a new Python interpreter, pass the given
        input source code from the stdin and return the
        result back. If the interpreter exits non-zero, it
        raises a ValueError."""

        process = self.spawn_repl()
        process.stdin.write(source)
        output = kill_python(process)

        if process.returncode != 0:
            raise ValueError("Process didn't exit properly.")
        return output

    @unittest.skipIf(not has_subprocess_support, "test requires subprocess")
    def test_getsource(self):
        output = self.run_on_interactive_mode(textwrap.dedent("""\
        def f():
            print(0)
            return 1 + 2

        import inspect
        print(f"The source is: <<<{inspect.getsource(f)}>>>")
        """))

        expected = "The source is: <<<def f():\n    print(0)\n    return 1 + 2\n>>>"
        self.assertIn(expected, output)




if __name__ == "__main__":
    unittest.main()
