'''
   Test cases for pyclbr.py
   Nick Mathewson
'''
from test_support import run_unittest
import unittest, sys
from types import ClassType, FunctionType, MethodType
import pyclbr

# This next line triggers an error on old versions of pyclbr.

from commands import getstatus

# Here we test the python class browser code.
#
# The main function in this suite, 'testModule', compares the output
# of pyclbr with the introspected members of a module.  Because pyclbr
# is imperfect (as designed), testModule is called with a set of
# members to ignore.

class PyclbrTest(unittest.TestCase):

    def assertListEq(self, l1, l2, ignore):
        ''' succeed iff {l1} - {ignore} == {l2} - {ignore} '''
        for p1, p2 in (l1, l2), (l2, l1):
            for item in p1:
                ok = (item in p2) or (item in ignore)
                if not ok:
                    self.fail("%r missing" % item)


    def assertHasattr(self, obj, attr, ignore):
        ''' succeed iff hasattr(obj,attr) or attr in ignore. '''
        if attr in ignore: return
        if not hasattr(obj, attr): print "???", attr
        self.failUnless(hasattr(obj, attr))


    def assertHaskey(self, obj, key, ignore):
        ''' succeed iff obj.has_key(key) or key in ignore. '''
        if key in ignore: return
        if not obj.has_key(key): print "***",key
        self.failUnless(obj.has_key(key))

    def assertEquals(self, a, b, ignore=None):
        ''' succeed iff a == b or a in ignore or b in ignore '''
        if (ignore == None) or (a in ignore) or (b in ignore): return

        unittest.TestCase.assertEquals(self, a, b)

    def checkModule(self, moduleName, module=None, ignore=()):
        ''' succeed iff pyclbr.readmodule_ex(modulename) corresponds
            to the actual module object, module.  Any identifiers in
            ignore are ignored.   If no module is provided, the appropriate
            module is loaded with __import__.'''

        if module == None:
            module = __import__(moduleName, globals(), {}, [])

        dict = pyclbr.readmodule_ex(moduleName)

        # Make sure the toplevel functions and classes are the same.
        for name, value in dict.items():
            if name in ignore:
                continue
            self.assertHasattr(module, name, ignore)
            py_item = getattr(module, name)
            if isinstance(value, pyclbr.Function):
                self.assertEquals(type(py_item), FunctionType)
            else:
                self.assertEquals(type(py_item), ClassType)
                real_bases = [base.__name__ for base in py_item.__bases__]
                pyclbr_bases = [ getattr(base, 'name', base)
                                 for base in value.super ]

                self.assertListEq(real_bases, pyclbr_bases, ignore)

                actualMethods = []
                for m in py_item.__dict__.keys():
                    if type(getattr(py_item, m)) == MethodType:
                        actualMethods.append(m)
                foundMethods = []
                for m in value.methods.keys():
                    if m[:2] == '__' and m[-2:] != '__':
                        foundMethods.append('_'+name+m)
                    else:
                        foundMethods.append(m)

                self.assertListEq(foundMethods, actualMethods, ignore)
                self.assertEquals(py_item.__module__, value.module)

                self.assertEquals(py_item.__name__, value.name, ignore)
                # can't check file or lineno

        # Now check for missing stuff.
        for name in dir(module):
            item = getattr(module, name)
            if type(item) in (ClassType, FunctionType):
                self.assertHaskey(dict, name, ignore)

    def test_easy(self):
        self.checkModule('pyclbr')
        self.checkModule('doctest',
                         ignore=['_isclass',
                                 '_isfunction',
                                 '_ismodule',
                                 '_classify_class_attrs'])
        self.checkModule('rfc822', ignore=["get"])
        self.checkModule('xmllib')
        self.checkModule('difflib')

    def test_others(self):
        cm = self.checkModule

        # these are about the 20 longest modules.

        cm('random', ignore=('_verify',)) # deleted

        cm('cgi', ignore=('f', 'g',       # nested declarations
                          'log'))         # set with =, not def

        cm('mhlib', ignore=('do',          # nested declaration
                            'bisect'))     # imported method, set with =

        cm('urllib', ignore=('getproxies_environment', # set with =
                             'getproxies_registry',    # set with =
                             'open_https'))            # not on all platforms

        cm('pickle', ignore=('g',))       # deleted declaration

        cm('aifc', ignore=('openfp',))    # set with =

        cm('Cookie', ignore=('__str__', 'Cookie')) # set with =

        cm('sre_parse', ignore=('literal', # nested def
                                'makedict', 'dump' # from sre_constants
                                ))

        cm('test.test_pyclbr',
           module=sys.modules[__name__])

        # pydoc doesn't work because of string issues
        # cm('pydoc', pydoc)

        # pdb plays too many dynamic games
        # cm('pdb', pdb)


def test_main():
    run_unittest(PyclbrTest)


if __name__ == "__main__":
    test_main()
