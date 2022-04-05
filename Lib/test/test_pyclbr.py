'''
   Test cases for pyclbr.py
   Nick Mathewson
'''

import sys
from textwrap import dedent
from types import FunctionType, MethodType, BuiltinFunctionType
import pyclbr
from unittest import TestCase, main as unittest_main
from test.test_importlib import util as test_importlib_util


StaticMethodType = type(staticmethod(lambda: None))
ClassMethodType = type(classmethod(lambda c: None))

# Here we test the python class browser code.
#
# The main function in this suite, 'testModule', compares the output
# of pyclbr with the introspected members of a module.  Because pyclbr
# is imperfect (as designed), testModule is called with a set of
# members to ignore.

class PyclbrTest(TestCase):

    def assertListEq(self, l1, l2, ignore):
        ''' succeed iff {l1} - {ignore} == {l2} - {ignore} '''
        missing = (set(l1) ^ set(l2)) - set(ignore)
        if missing:
            print("l1=%r\nl2=%r\nignore=%r" % (l1, l2, ignore), file=sys.stderr)
            self.fail("%r missing" % missing.pop())

    def assertHasattr(self, obj, attr, ignore):
        ''' succeed iff hasattr(obj,attr) or attr in ignore. '''
        if attr in ignore: return
        if not hasattr(obj, attr): print("???", attr)
        self.assertTrue(hasattr(obj, attr),
                        'expected hasattr(%r, %r)' % (obj, attr))


    def assertHaskey(self, obj, key, ignore):
        ''' succeed iff key in obj or key in ignore. '''
        if key in ignore: return
        if key not in obj:
            print("***",key, file=sys.stderr)
        self.assertIn(key, obj)

    def assertEqualsOrIgnored(self, a, b, ignore):
        ''' succeed iff a == b or a in ignore or b in ignore '''
        if a not in ignore and b not in ignore:
            self.assertEqual(a, b)

    def checkModule(self, moduleName, module=None, ignore=()):
        ''' succeed iff pyclbr.readmodule_ex(modulename) corresponds
            to the actual module object, module.  Any identifiers in
            ignore are ignored.   If no module is provided, the appropriate
            module is loaded with __import__.'''

        ignore = set(ignore) | set(['object'])

        if module is None:
            # Import it.
            # ('<silly>' is to work around an API silliness in __import__)
            module = __import__(moduleName, globals(), {}, ['<silly>'])

        dict = pyclbr.readmodule_ex(moduleName)

        def ismethod(oclass, obj, name):
            classdict = oclass.__dict__
            if isinstance(obj, MethodType):
                # could be a classmethod
                if (not isinstance(classdict[name], ClassMethodType) or
                    obj.__self__ is not oclass):
                    return False
            elif not isinstance(obj, FunctionType):
                return False

            objname = obj.__name__
            if objname.startswith("__") and not objname.endswith("__"):
                objname = "_%s%s" % (oclass.__name__, objname)
            return objname == name

        # Make sure the toplevel functions and classes are the same.
        for name, value in dict.items():
            if name in ignore:
                continue
            self.assertHasattr(module, name, ignore)
            py_item = getattr(module, name)
            if isinstance(value, pyclbr.Function):
                self.assertIsInstance(py_item, (FunctionType, BuiltinFunctionType))
                if py_item.__module__ != moduleName:
                    continue   # skip functions that came from somewhere else
                self.assertEqual(py_item.__module__, value.module)
            else:
                self.assertIsInstance(py_item, type)
                if py_item.__module__ != moduleName:
                    continue   # skip classes that came from somewhere else

                real_bases = [base.__name__ for base in py_item.__bases__]
                pyclbr_bases = [ getattr(base, 'name', base)
                                 for base in value.super ]

                try:
                    self.assertListEq(real_bases, pyclbr_bases, ignore)
                except:
                    print("class=%s" % py_item, file=sys.stderr)
                    raise

                actualMethods = []
                for m in py_item.__dict__.keys():
                    if ismethod(py_item, getattr(py_item, m), m):
                        actualMethods.append(m)
                foundMethods = []
                for m in value.methods.keys():
                    if m[:2] == '__' and m[-2:] != '__':
                        foundMethods.append('_'+name+m)
                    else:
                        foundMethods.append(m)

                try:
                    self.assertListEq(foundMethods, actualMethods, ignore)
                    self.assertEqual(py_item.__module__, value.module)

                    self.assertEqualsOrIgnored(py_item.__name__, value.name,
                                               ignore)
                    # can't check file or lineno
                except:
                    print("class=%s" % py_item, file=sys.stderr)
                    raise

        # Now check for missing stuff.
        def defined_in(item, module):
            if isinstance(item, type):
                return item.__module__ == module.__name__
            if isinstance(item, FunctionType):
                return item.__globals__ is module.__dict__
            return False
        for name in dir(module):
            item = getattr(module, name)
            if isinstance(item,  (type, FunctionType)):
                if defined_in(item, module):
                    self.assertHaskey(dict, name, ignore)

    def test_easy(self):
        self.checkModule('pyclbr')
        # XXX: Metaclasses are not supported
        # self.checkModule('ast')
        self.checkModule('doctest', ignore=("TestResults", "_SpoofOut",
                                            "DocTestCase", '_DocTestSuite'))
        self.checkModule('difflib', ignore=("Match",))

    def test_decorators(self):
        self.checkModule('test.pyclbr_input', ignore=['om'])

    def test_nested(self):
        mb = pyclbr
        # Set arguments for descriptor creation and _creat_tree call.
        m, p, f, t, i = 'test', '', 'test.py', {}, None
        source = dedent("""\
        def f0():
            def f1(a,b,c):
                def f2(a=1, b=2, c=3): pass
                return f1(a,b,d)
            class c1: pass
        class C0:
            "Test class."
            def F1():
                "Method."
                return 'return'
            class C1():
                class C2:
                    "Class nested within nested class."
                    def F3(): return 1+1

        """)
        actual = mb._create_tree(m, p, f, source, t, i)

        # Create descriptors, linked together, and expected dict.
        f0 = mb.Function(m, 'f0', f, 1, end_lineno=5)
        f1 = mb._nest_function(f0, 'f1', 2, 4)
        f2 = mb._nest_function(f1, 'f2', 3, 3)
        c1 = mb._nest_class(f0, 'c1', 5, 5)
        C0 = mb.Class(m, 'C0', None, f, 6, end_lineno=14)
        F1 = mb._nest_function(C0, 'F1', 8, 10)
        C1 = mb._nest_class(C0, 'C1', 11, 14)
        C2 = mb._nest_class(C1, 'C2', 12, 14)
        F3 = mb._nest_function(C2, 'F3', 14, 14)
        expected = {'f0':f0, 'C0':C0}

        def compare(parent1, children1, parent2, children2):
            """Return equality of tree pairs.

            Each parent,children pair define a tree.  The parents are
            assumed equal.  Comparing the children dictionaries as such
            does not work due to comparison by identity and double
            linkage.  We separate comparing string and number attributes
            from comparing the children of input children.
            """
            self.assertEqual(children1.keys(), children2.keys())
            for ob in children1.values():
                self.assertIs(ob.parent, parent1)
            for ob in children2.values():
                self.assertIs(ob.parent, parent2)
            for key in children1.keys():
                o1, o2 = children1[key], children2[key]
                t1 = type(o1), o1.name, o1.file, o1.module, o1.lineno, o1.end_lineno
                t2 = type(o2), o2.name, o2.file, o2.module, o2.lineno, o2.end_lineno
                self.assertEqual(t1, t2)
                if type(o1) is mb.Class:
                    self.assertEqual(o1.methods, o2.methods)
                # Skip superclasses for now as not part of example
                compare(o1, o1.children, o2, o2.children)

        compare(None, actual, None, expected)

    def test_others(self):
        cm = self.checkModule

        # These were once about the 10 longest modules
        cm('random', ignore=('Random',))  # from _random import Random as CoreGenerator
        cm('cgi', ignore=('log',))      # set with = in module
        cm('pickle', ignore=('partial', 'PickleBuffer'))
        cm('aifc', ignore=('_aifc_params',))  # set with = in module
        cm('re._parser', ignore=('dump', 'groups', 'pos')) # from ._constants import *; property
        cm(
            'pdb',
            # pyclbr does not handle elegantly `typing` or properties
            ignore=('Union', 'ModuleTarget', 'ScriptTarget'),
        )
        cm('pydoc', ignore=('input', 'output',)) # properties

        # Tests for modules inside packages
        cm('email.parser')
        cm('test.test_pyclbr')


class ReadmoduleTests(TestCase):

    def setUp(self):
        self._modules = pyclbr._modules.copy()

    def tearDown(self):
        pyclbr._modules = self._modules


    def test_dotted_name_not_a_package(self):
        # test ImportError is raised when the first part of a dotted name is
        # not a package.
        #
        # Issue #14798.
        self.assertRaises(ImportError, pyclbr.readmodule_ex, 'asyncio.foo')

    def test_module_has_no_spec(self):
        module_name = "doesnotexist"
        assert module_name not in pyclbr._modules
        with test_importlib_util.uncache(module_name):
            with self.assertRaises(ModuleNotFoundError):
                pyclbr.readmodule_ex(module_name)


if __name__ == "__main__":
    unittest_main()
