import unittest
import sys
import os
import _testcapi
from test import support

# Skip this test if the _tkinter module wasn't built.
_tkinter = support.import_module('_tkinter')

# Make sure tkinter._fix runs to set up the environment
support.import_fresh_module('tkinter')

from tkinter import Tcl
from _tkinter import TclError

tcl_version = _tkinter.TCL_VERSION.split('.')
try:
    for i in range(len(tcl_version)):
        tcl_version[i] = int(tcl_version[i])
except ValueError:
    pass
tcl_version = tuple(tcl_version)

_tk_patchlevel = None
def get_tk_patchlevel():
    global _tk_patchlevel
    if _tk_patchlevel is None:
        tcl = Tcl()
        patchlevel = []
        for x in tcl.call('info', 'patchlevel').split('.'):
            try:
                x = int(x, 10)
            except ValueError:
                x = -1
            patchlevel.append(x)
        _tk_patchlevel = tuple(patchlevel)
    return _tk_patchlevel


class TkinterTest(unittest.TestCase):

    def testFlattenLen(self):
        # flatten(<object with no length>)
        self.assertRaises(TypeError, _tkinter._flatten, True)


class TclTest(unittest.TestCase):

    def setUp(self):
        self.interp = Tcl()
        self.wantobjects = self.interp.tk.wantobjects()

    def testEval(self):
        tcl = self.interp
        tcl.eval('set a 1')
        self.assertEqual(tcl.eval('set a'),'1')

    def testEvalException(self):
        tcl = self.interp
        self.assertRaises(TclError,tcl.eval,'set a')

    def testEvalException2(self):
        tcl = self.interp
        self.assertRaises(TclError,tcl.eval,'this is wrong')

    def testCall(self):
        tcl = self.interp
        tcl.call('set','a','1')
        self.assertEqual(tcl.call('set','a'),'1')

    def testCallException(self):
        tcl = self.interp
        self.assertRaises(TclError,tcl.call,'set','a')

    def testCallException2(self):
        tcl = self.interp
        self.assertRaises(TclError,tcl.call,'this','is','wrong')

    def testSetVar(self):
        tcl = self.interp
        tcl.setvar('a','1')
        self.assertEqual(tcl.eval('set a'),'1')

    def testSetVarArray(self):
        tcl = self.interp
        tcl.setvar('a(1)','1')
        self.assertEqual(tcl.eval('set a(1)'),'1')

    def testGetVar(self):
        tcl = self.interp
        tcl.eval('set a 1')
        self.assertEqual(tcl.getvar('a'),'1')

    def testGetVarArray(self):
        tcl = self.interp
        tcl.eval('set a(1) 1')
        self.assertEqual(tcl.getvar('a(1)'),'1')

    def testGetVarException(self):
        tcl = self.interp
        self.assertRaises(TclError,tcl.getvar,'a')

    def testGetVarArrayException(self):
        tcl = self.interp
        self.assertRaises(TclError,tcl.getvar,'a(1)')

    def testUnsetVar(self):
        tcl = self.interp
        tcl.setvar('a',1)
        self.assertEqual(tcl.eval('info exists a'),'1')
        tcl.unsetvar('a')
        self.assertEqual(tcl.eval('info exists a'),'0')

    def testUnsetVarArray(self):
        tcl = self.interp
        tcl.setvar('a(1)',1)
        tcl.setvar('a(2)',2)
        self.assertEqual(tcl.eval('info exists a(1)'),'1')
        self.assertEqual(tcl.eval('info exists a(2)'),'1')
        tcl.unsetvar('a(1)')
        self.assertEqual(tcl.eval('info exists a(1)'),'0')
        self.assertEqual(tcl.eval('info exists a(2)'),'1')

    def testUnsetVarException(self):
        tcl = self.interp
        self.assertRaises(TclError,tcl.unsetvar,'a')

    def testEvalFile(self):
        tcl = self.interp
        filename = "testEvalFile.tcl"
        fd = open(filename,'w')
        script = """set a 1
        set b 2
        set c [ expr $a + $b ]
        """
        fd.write(script)
        fd.close()
        tcl.evalfile(filename)
        os.remove(filename)
        self.assertEqual(tcl.eval('set a'),'1')
        self.assertEqual(tcl.eval('set b'),'2')
        self.assertEqual(tcl.eval('set c'),'3')

    def testEvalFileException(self):
        tcl = self.interp
        filename = "doesnotexists"
        try:
            os.remove(filename)
        except Exception as e:
            pass
        self.assertRaises(TclError,tcl.evalfile,filename)

    def testPackageRequireException(self):
        tcl = self.interp
        self.assertRaises(TclError,tcl.eval,'package require DNE')

    @unittest.skipUnless(sys.platform == 'win32', 'Requires Windows')
    def testLoadWithUNC(self):
        # Build a UNC path from the regular path.
        # Something like
        #   \\%COMPUTERNAME%\c$\python27\python.exe

        fullname = os.path.abspath(sys.executable)
        if fullname[1] != ':':
            raise unittest.SkipTest('Absolute path should have drive part')
        unc_name = r'\\%s\%s$\%s' % (os.environ['COMPUTERNAME'],
                                    fullname[0],
                                    fullname[3:])
        if not os.path.exists(unc_name):
            raise unittest.SkipTest('Cannot connect to UNC Path')

        with support.EnvironmentVarGuard() as env:
            env.unset("TCL_LIBRARY")
            f = os.popen('%s -c "import tkinter; print(tkinter)"' % (unc_name,))

        self.assertIn('tkinter', f.read())
        # exit code must be zero
        self.assertEqual(f.close(), None)

    def test_passing_values(self):
        def passValue(value):
            return self.interp.call('set', '_', value)

        self.assertEqual(passValue(True), True if self.wantobjects else '1')
        self.assertEqual(passValue(False), False if self.wantobjects else '0')
        self.assertEqual(passValue('string'), 'string')
        self.assertEqual(passValue('string\u20ac'), 'string\u20ac')
        for i in (0, 1, -1, 2**31-1, -2**31):
            self.assertEqual(passValue(i), i if self.wantobjects else str(i))
        for f in (0.0, 1.0, -1.0, 1/3,
                  sys.float_info.min, sys.float_info.max,
                  -sys.float_info.min, -sys.float_info.max):
            if self.wantobjects:
                self.assertEqual(passValue(f), f)
            else:
                self.assertEqual(float(passValue(f)), f)
        if self.wantobjects:
            f = passValue(float('nan'))
            self.assertNotEqual(f, f)
            self.assertEqual(passValue(float('inf')), float('inf'))
            self.assertEqual(passValue(-float('inf')), -float('inf'))
        else:
            f = float(passValue(float('nan')))
            self.assertNotEqual(f, f)
            self.assertEqual(float(passValue(float('inf'))), float('inf'))
            self.assertEqual(float(passValue(-float('inf'))), -float('inf'))
        self.assertEqual(passValue((1, '2', (3.4,))),
                         (1, '2', (3.4,)) if self.wantobjects else '1 2 3.4')

    def test_user_command(self):
        result = None
        def testfunc(arg):
            nonlocal result
            result = arg
            return arg
        self.interp.createcommand('testfunc', testfunc)
        def check(value, expected, eq=self.assertEqual):
            r = self.interp.call('testfunc', value)
            self.assertIsInstance(result, str)
            eq(result, expected)
            self.assertIsInstance(r, str)
            eq(r, expected)
        def float_eq(actual, expected):
            expected = float(expected)
            self.assertAlmostEqual(float(actual), expected,
                                   delta=abs(expected) * 1e-10)
        def nan_eq(actual, expected):
            actual = float(actual)
            self.assertNotEqual(actual, actual)

        check(True, '1')
        check(False, '0')
        check('string', 'string')
        check('string\xbd', 'string\xbd')
        check('string\u20ac', 'string\u20ac')
        for i in (0, 1, -1, 2**31-1, -2**31):
            check(i, str(i))
        for f in (0.0, 1.0, -1.0):
            check(f, repr(f))
        for f in (1/3.0, sys.float_info.min, sys.float_info.max,
                  -sys.float_info.min, -sys.float_info.max):
            check(f, f, eq=float_eq)
        check(float('inf'), 'Inf', eq=float_eq)
        check(-float('inf'), '-Inf', eq=float_eq)
        check(float('nan'), 'NaN', eq=nan_eq)
        check((), '')
        check((1, (2,), (3, 4), '5 6', ()), '1 2 {3 4} {5 6} {}')

    def test_splitlist(self):
        splitlist = self.interp.tk.splitlist
        call = self.interp.tk.call
        self.assertRaises(TypeError, splitlist)
        self.assertRaises(TypeError, splitlist, 'a', 'b')
        self.assertRaises(TypeError, splitlist, 2)
        testcases = [
            ('2', ('2',)),
            ('', ()),
            ('{}', ('',)),
            ('""', ('',)),
            ('a\n b\t\r c\n ', ('a', 'b', 'c')),
            (b'a\n b\t\r c\n ', ('a', 'b', 'c')),
            ('a \u20ac', ('a', '\u20ac')),
            (b'a \xe2\x82\xac', ('a', '\u20ac')),
            ('a {b c}', ('a', 'b c')),
            (r'a b\ c', ('a', 'b c')),
            (('a', 'b c'), ('a', 'b c')),
            ('a 2', ('a', '2')),
            (('a', 2), ('a', 2)),
            ('a 3.4', ('a', '3.4')),
            (('a', 3.4), ('a', 3.4)),
            ((), ()),
            (call('list', 1, '2', (3.4,)),
                (1, '2', (3.4,)) if self.wantobjects else
                ('1', '2', '3.4')),
        ]
        if tcl_version >= (8, 5):
            if not self.wantobjects or get_tk_patchlevel() < (8, 5, 5):
                # Before 8.5.5 dicts were converted to lists through string
                expected = ('12', '\u20ac', '\u20ac', '3.4')
            else:
                expected = (12, '\u20ac', '\u20ac', (3.4,))
            testcases += [
                (call('dict', 'create', 12, '\u20ac', b'\xe2\x82\xac', (3.4,)),
                    expected),
            ]
        for arg, res in testcases:
            self.assertEqual(splitlist(arg), res, msg=arg)
        self.assertRaises(TclError, splitlist, '{')

    def test_split(self):
        split = self.interp.tk.split
        call = self.interp.tk.call
        self.assertRaises(TypeError, split)
        self.assertRaises(TypeError, split, 'a', 'b')
        self.assertRaises(TypeError, split, 2)
        testcases = [
            ('2', '2'),
            ('', ''),
            ('{}', ''),
            ('""', ''),
            ('{', '{'),
            ('a\n b\t\r c\n ', ('a', 'b', 'c')),
            (b'a\n b\t\r c\n ', ('a', 'b', 'c')),
            ('a \u20ac', ('a', '\u20ac')),
            (b'a \xe2\x82\xac', ('a', '\u20ac')),
            ('a {b c}', ('a', ('b', 'c'))),
            (r'a b\ c', ('a', ('b', 'c'))),
            (('a', b'b c'), ('a', ('b', 'c'))),
            (('a', 'b c'), ('a', ('b', 'c'))),
            ('a 2', ('a', '2')),
            (('a', 2), ('a', 2)),
            ('a 3.4', ('a', '3.4')),
            (('a', 3.4), ('a', 3.4)),
            (('a', (2, 3.4)), ('a', (2, 3.4))),
            ((), ()),
            (call('list', 1, '2', (3.4,)),
                (1, '2', (3.4,)) if self.wantobjects else
                ('1', '2', '3.4')),
        ]
        if tcl_version >= (8, 5):
            if not self.wantobjects or get_tk_patchlevel() < (8, 5, 5):
                # Before 8.5.5 dicts were converted to lists through string
                expected = ('12', '\u20ac', '\u20ac', '3.4')
            else:
                expected = (12, '\u20ac', '\u20ac', (3.4,))
            testcases += [
                (call('dict', 'create', 12, '\u20ac', b'\xe2\x82\xac', (3.4,)),
                    expected),
            ]
        for arg, res in testcases:
            self.assertEqual(split(arg), res, msg=arg)


class BigmemTclTest(unittest.TestCase):

    def setUp(self):
        self.interp = Tcl()

    @unittest.skipUnless(_testcapi.INT_MAX < _testcapi.PY_SSIZE_T_MAX,
                         "needs UINT_MAX < SIZE_MAX")
    @support.bigmemtest(size=_testcapi.INT_MAX + 1, memuse=5, dry_run=False)
    def test_huge_string(self, size):
        value = ' ' * size
        self.assertRaises(OverflowError, self.interp.call, 'set', '_', value)


def setUpModule():
    if support.verbose:
        tcl = Tcl()
        print('patchlevel =', tcl.call('info', 'patchlevel'))


def test_main():
    support.run_unittest(TclTest, TkinterTest, BigmemTclTest)

if __name__ == "__main__":
    test_main()
