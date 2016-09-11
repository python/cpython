'''
   Test cases for pyclbr.py
   Nick Mathewson
'''
import sys
from types import FunctionType, MethodType, BuiltinFunctionType
import pyclbr
from unittest import TestCase, main as unittest_main

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
        self.checkModule('ast')
        self.checkModule('doctest', ignore=("TestResults", "_SpoofOut",
                                            "DocTestCase", '_DocTestSuite'))
        self.checkModule('difflib', ignore=("Match",))

    def test_decorators(self):
        # XXX: See comment in pyclbr_input.py for a test that would fail
        #      if it were not commented out.
        #
        self.checkModule('test.pyclbr_input', ignore=['om'])

    def test_others(self):
        cm = self.checkModule

        # These were once about the 10 longest modules
        cm('random', ignore=('Random',))  # from _random import Random as CoreGenerator
        cm('cgi', ignore=('log',))      # set with = in module
        cm('pickle', ignore=('partial',))
        cm('aifc', ignore=('openfp', '_aifc_params'))  # set with = in module
        cm('sre_parse', ignore=('dump', 'groups', 'pos')) # from sre_constants import *; property
        cm('pdb')
        cm('pydoc')

        # Tests for modules inside packages
        cm('email.parser')
        cm('test.test_pyclbr')

    def test_issue_14798(self):
        # test ImportError is raised when the first part of a dotted name is
        # not a package
        self.assertRaises(ImportError, pyclbr.readmodule_ex, 'asyncore.foo')


if __name__ == "__main__":
    unittest_main()
