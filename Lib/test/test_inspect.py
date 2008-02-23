import sys
import types
import unittest
import inspect
import datetime
import collections
from os.path import normcase

from test.test_support import TESTFN, run_unittest

from test import inspect_fodder as mod
from test import inspect_fodder2 as mod2

# Functions tested in this suite:
# ismodule, isclass, ismethod, isfunction, istraceback, isframe, iscode,
# isbuiltin, isroutine, isgenerator, isgeneratorfunction, getmembers,
# getdoc, getfile, getmodule, getsourcefile, getcomments, getsource,
# getclasstree, getargspec, getargvalues, formatargspec, formatargvalues,
# currentframe, stack, trace, isdatadescriptor

modfile = mod.__file__
if modfile.endswith(('c', 'o')):
    modfile = modfile[:-1]

# Normalize file names: on Windows, the case of file names of compiled
# modules depends on the path used to start the python executable.
modfile = normcase(modfile)

def revise(filename, *args):
    return (normcase(filename),) + args

import builtins

try:
    1/0
except:
    tb = sys.exc_info()[2]

git = mod.StupidGit()

class IsTestBase(unittest.TestCase):
    predicates = set([inspect.isbuiltin, inspect.isclass, inspect.iscode,
                      inspect.isframe, inspect.isfunction, inspect.ismethod,
                      inspect.ismodule, inspect.istraceback,
                      inspect.isgenerator, inspect.isgeneratorfunction])

    def istest(self, predicate, exp):
        obj = eval(exp)
        self.failUnless(predicate(obj), '%s(%s)' % (predicate.__name__, exp))

        for other in self.predicates - set([predicate]):
            if predicate == inspect.isgeneratorfunction and\
               other == inspect.isfunction:
                continue
            self.failIf(other(obj), 'not %s(%s)' % (other.__name__, exp))

def generator_function_example(self):
    for i in range(2):
        yield i

class TestPredicates(IsTestBase):
    def test_fifteen(self):
        count = len([x for x in dir(inspect) if x.startswith('is')])
        # This test is here for remember you to update Doc/library/inspect.rst
        # which claims there are 15 such functions
        expected = 15
        err_msg = "There are %d (not %d) is* functions" % (count, expected)
        self.assertEqual(count, expected, err_msg)


    def test_excluding_predicates(self):
        self.istest(inspect.isbuiltin, 'sys.exit')
        self.istest(inspect.isbuiltin, '[].append')
        self.istest(inspect.isclass, 'mod.StupidGit')
        self.istest(inspect.iscode, 'mod.spam.__code__')
        self.istest(inspect.isframe, 'tb.tb_frame')
        self.istest(inspect.isfunction, 'mod.spam')
        self.istest(inspect.isfunction, 'mod.StupidGit.abuse')
        self.istest(inspect.ismethod, 'git.argue')
        self.istest(inspect.ismodule, 'mod')
        self.istest(inspect.istraceback, 'tb')
        self.istest(inspect.isdatadescriptor, 'collections.defaultdict.default_factory')
        self.istest(inspect.isgenerator, '(x for x in range(2))')
        self.istest(inspect.isgeneratorfunction, 'generator_function_example')
        if hasattr(types, 'GetSetDescriptorType'):
            self.istest(inspect.isgetsetdescriptor,
                        'type(tb.tb_frame).f_locals')
        else:
            self.failIf(inspect.isgetsetdescriptor(type(tb.tb_frame).f_locals))
        if hasattr(types, 'MemberDescriptorType'):
            self.istest(inspect.ismemberdescriptor, 'datetime.timedelta.days')
        else:
            self.failIf(inspect.ismemberdescriptor(datetime.timedelta.days))

    def test_isroutine(self):
        self.assert_(inspect.isroutine(mod.spam))
        self.assert_(inspect.isroutine([].count))

class TestInterpreterStack(IsTestBase):
    def __init__(self, *args, **kwargs):
        unittest.TestCase.__init__(self, *args, **kwargs)

        git.abuse(7, 8, 9)

    def test_abuse_done(self):
        self.istest(inspect.istraceback, 'git.ex[2]')
        self.istest(inspect.isframe, 'mod.fr')

    def test_stack(self):
        self.assert_(len(mod.st) >= 5)
        self.assertEqual(revise(*mod.st[0][1:]),
             (modfile, 16, 'eggs', ['    st = inspect.stack()\n'], 0))
        self.assertEqual(revise(*mod.st[1][1:]),
             (modfile, 9, 'spam', ['    eggs(b + d, c + f)\n'], 0))
        self.assertEqual(revise(*mod.st[2][1:]),
             (modfile, 43, 'argue', ['            spam(a, b, c)\n'], 0))
        self.assertEqual(revise(*mod.st[3][1:]),
             (modfile, 39, 'abuse', ['        self.argue(a, b, c)\n'], 0))

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
    fodderFile = None

    def __init__(self, *args, **kwargs):
        unittest.TestCase.__init__(self, *args, **kwargs)

        self.source = open(inspect.getsourcefile(self.fodderFile)).read()

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
                          ('StupidGit', mod.StupidGit)])
        tree = inspect.getclasstree([cls[1] for cls in classes], 1)
        self.assertEqual(tree,
                         [(object, ()),
                          [(mod.ParrotDroppings, (object,)),
                           (mod.StupidGit, (object,)),
                           [(mod.MalodorousPervert, (mod.StupidGit,)),
                            [(mod.FesteringGob, (mod.MalodorousPervert,
                                                    mod.ParrotDroppings))
                             ]
                            ]
                           ]
                          ])

    def test_getfunctions(self):
        functions = inspect.getmembers(mod, inspect.isfunction)
        self.assertEqual(functions, [('eggs', mod.eggs),
                                     ('spam', mod.spam)])

    def test_getdoc(self):
        self.assertEqual(inspect.getdoc(mod), 'A module docstring.')
        self.assertEqual(inspect.getdoc(mod.StupidGit),
                         'A longer,\n\nindented\n\ndocstring.')
        self.assertEqual(inspect.getdoc(git.abuse),
                         'Another\n\ndocstring\n\ncontaining\n\ntabs')

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
        self.assertEqual(inspect.getmodule(str), sys.modules["builtins"])
        # Check filename override
        self.assertEqual(inspect.getmodule(None, modfile), mod)

    def test_getsource(self):
        self.assertSourceEqual(git.abuse, 29, 39)
        self.assertSourceEqual(mod.StupidGit, 21, 46)

    def test_getsourcefile(self):
        self.assertEqual(normcase(inspect.getsourcefile(mod.spam)), modfile)
        self.assertEqual(normcase(inspect.getsourcefile(git.abuse)), modfile)

    def test_getfile(self):
        self.assertEqual(inspect.getfile(mod.StupidGit), mod.__file__)

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
        args, varargs, varkw, defaults = inspect.getargspec(routine)
        self.assertEqual(args, args_e)
        self.assertEqual(varargs, varargs_e)
        self.assertEqual(varkw, varkw_e)
        self.assertEqual(defaults, defaults_e)
        if formatted is not None:
            self.assertEqual(inspect.formatargspec(args, varargs, varkw, defaults),
                             formatted)

    def assertFullArgSpecEquals(self, routine, args_e, varargs_e=None,
                                    varkw_e=None, defaults_e=None,
                                    kwonlyargs_e=[], kwonlydefaults_e=None,
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

    def test_getfullargspec(self):
        self.assertFullArgSpecEquals(mod2.keyworded, [], varargs_e='arg1',
                                     kwonlyargs_e=['arg2'],
                                     kwonlydefaults_e={'arg2':1},
                                     formatted='(*arg1, arg2=1)')

        self.assertFullArgSpecEquals(mod2.annotated, ['arg1'],
                                     ann_e={'arg1' : list},
                                     formatted='(arg1: list)')

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

        attrs = attrs_wo_objs(A)
        self.assert_(('s', 'static method', A) in attrs, 'missing static method')
        self.assert_(('c', 'class method', A) in attrs, 'missing class method')
        self.assert_(('p', 'property', A) in attrs, 'missing property')
        self.assert_(('m', 'method', A) in attrs,
            'missing plain method: %r' % attrs)
        self.assert_(('m1', 'method', A) in attrs, 'missing plain method')
        self.assert_(('datablob', 'data', A) in attrs, 'missing data')

        class B(A):

            def m(self): pass

        attrs = attrs_wo_objs(B)
        self.assert_(('s', 'static method', A) in attrs, 'missing static method')
        self.assert_(('c', 'class method', A) in attrs, 'missing class method')
        self.assert_(('p', 'property', A) in attrs, 'missing property')
        self.assert_(('m', 'method', B) in attrs, 'missing plain method')
        self.assert_(('m1', 'method', A) in attrs, 'missing plain method')
        self.assert_(('datablob', 'data', A) in attrs, 'missing data')


        class C(A):

            def m(self): pass
            def c(self): pass

        attrs = attrs_wo_objs(C)
        self.assert_(('s', 'static method', A) in attrs, 'missing static method')
        self.assert_(('c', 'method', C) in attrs, 'missing plain method')
        self.assert_(('p', 'property', A) in attrs, 'missing property')
        self.assert_(('m', 'method', C) in attrs, 'missing plain method')
        self.assert_(('m1', 'method', A) in attrs, 'missing plain method')
        self.assert_(('datablob', 'data', A) in attrs, 'missing data')

        class D(B, C):

            def m1(self): pass

        attrs = attrs_wo_objs(D)
        self.assert_(('s', 'static method', A) in attrs, 'missing static method')
        self.assert_(('c', 'method', C) in attrs, 'missing plain method')
        self.assert_(('p', 'property', A) in attrs, 'missing property')
        self.assert_(('m', 'method', B) in attrs, 'missing plain method')
        self.assert_(('m1', 'method', D) in attrs, 'missing plain method')
        self.assert_(('datablob', 'data', A) in attrs, 'missing data')

def test_main():
    run_unittest(TestDecorators, TestRetrievingSourceCode, TestOneliners,
                 TestBuggyCases,
                 TestInterpreterStack, TestClassesAndFunctions, TestPredicates)

if __name__ == "__main__":
    test_main()
