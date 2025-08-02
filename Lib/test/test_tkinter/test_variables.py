import unittest
from test import support

import gc
import tkinter
from tkinter import (Variable, StringVar, IntVar, DoubleVar, BooleanVar, Tcl,
                     TclError)
from test.support import ALWAYS_EQ
from test.test_tkinter.support import AbstractDefaultRootTest, tcl_version


class Var(Variable):

    _default = "default"
    side_effect = False

    def set(self, value):
        self.side_effect = True
        super().set(value)


class TestBase(unittest.TestCase):

    def setUp(self):
        self.root = Tcl()

    def tearDown(self):
        del self.root


class TestVariable(TestBase):

    def info_exists(self, *args):
        return self.root.getboolean(self.root.call("info", "exists", *args))

    def test_default(self):
        v = Variable(self.root)
        self.assertEqual("", v.get())
        self.assertRegex(str(v), r"^PY_VAR(\d+)$")

    def test_name_and_value(self):
        v = Variable(self.root, "sample string", "varname")
        self.assertEqual("sample string", v.get())
        self.assertEqual("varname", str(v))

    def test___del__(self):
        self.assertFalse(self.info_exists("varname"))
        v = Variable(self.root, "sample string", "varname")
        self.assertTrue(self.info_exists("varname"))
        del v
        support.gc_collect()  # For PyPy or other GCs.
        self.assertFalse(self.info_exists("varname"))

    def test_dont_unset_not_existing(self):
        self.assertFalse(self.info_exists("varname"))
        v1 = Variable(self.root, name="name")
        v2 = Variable(self.root, name="name")
        del v1
        support.gc_collect()  # For PyPy or other GCs.
        self.assertFalse(self.info_exists("name"))
        # shouldn't raise exception
        del v2
        support.gc_collect()  # For PyPy or other GCs.
        self.assertFalse(self.info_exists("name"))

    def test_equality(self):
        # values doesn't matter, only class and name are checked
        v1 = Variable(self.root, name="abc")
        v2 = Variable(self.root, name="abc")
        self.assertIsNot(v1, v2)
        self.assertEqual(v1, v2)

        v3 = Variable(self.root, name="cba")
        self.assertNotEqual(v1, v3)

        v4 = StringVar(self.root, name="abc")
        self.assertEqual(str(v1), str(v4))
        self.assertNotEqual(v1, v4)

        V = type('Variable', (), {})
        self.assertNotEqual(v1, V())

        self.assertNotEqual(v1, object())
        self.assertEqual(v1, ALWAYS_EQ)

        root2 = tkinter.Tk()
        self.addCleanup(root2.destroy)
        v5 = Variable(root2, name="abc")
        self.assertEqual(str(v1), str(v5))
        self.assertNotEqual(v1, v5)

    def test_invalid_name(self):
        with self.assertRaises(TypeError):
            Variable(self.root, name=123)

    def test_null_in_name(self):
        with self.assertRaises(ValueError):
            Variable(self.root, name='var\x00name')
        with self.assertRaises(ValueError):
            self.root.globalsetvar('var\x00name', "value")
        with self.assertRaises(ValueError):
            self.root.globalsetvar(b'var\x00name', "value")
        with self.assertRaises(ValueError):
            self.root.setvar('var\x00name', "value")
        with self.assertRaises(ValueError):
            self.root.setvar(b'var\x00name', "value")

    def test_initialize(self):
        v = Var(self.root)
        self.assertFalse(v.side_effect)
        v.set("value")
        self.assertTrue(v.side_effect)

    def test_trace_old(self):
        if tcl_version >= (9, 0):
            self.skipTest('requires Tcl version < 9.0')
        # Old interface
        v = Variable(self.root)
        vname = str(v)
        trace = []
        def read_tracer(*args):
            trace.append(('read',) + args)
        def write_tracer(*args):
            trace.append(('write',) + args)
        with self.assertWarns(DeprecationWarning) as cm:
            cb1 = v.trace_variable('r', read_tracer)
        self.assertEqual(cm.filename, __file__)
        with self.assertWarns(DeprecationWarning):
            cb2 = v.trace_variable('wu', write_tracer)
        with self.assertWarns(DeprecationWarning) as cm:
            self.assertEqual(sorted(v.trace_vinfo()), [('r', cb1), ('wu', cb2)])
        self.assertEqual(cm.filename, __file__)
        self.assertEqual(trace, [])

        v.set('spam')
        self.assertEqual(trace, [('write', vname, '', 'w')])

        trace = []
        v.get()
        self.assertEqual(trace, [('read', vname, '', 'r')])

        trace = []
        with self.assertWarns(DeprecationWarning):
            info = sorted(v.trace_vinfo())
        with self.assertWarns(DeprecationWarning):
            v.trace_vdelete('w', cb1)  # Wrong mode
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(sorted(v.trace_vinfo()), info)
        with self.assertRaises(TclError):
            with self.assertWarns(DeprecationWarning):
                v.trace_vdelete('r', 'spam')  # Wrong command name
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(sorted(v.trace_vinfo()), info)
        with self.assertWarns(DeprecationWarning):
            v.trace_vdelete('r', (cb1, 43)) # Wrong arguments
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(sorted(v.trace_vinfo()), info)
        v.get()
        self.assertEqual(trace, [('read', vname, '', 'r')])

        trace = []
        with self.assertWarns(DeprecationWarning) as cm:
            v.trace_vdelete('r', cb1)
        self.assertEqual(cm.filename, __file__)
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(v.trace_vinfo(), [('wu', cb2)])
        v.get()
        self.assertEqual(trace, [])

        trace = []
        del write_tracer
        gc.collect()
        v.set('eggs')
        self.assertEqual(trace, [('write', vname, '', 'w')])

        trace = []
        del v
        gc.collect()
        self.assertEqual(trace, [('write', vname, '', 'u')])

    def test_trace(self):
        v = Variable(self.root)
        vname = str(v)
        trace = []
        def read_tracer(*args):
            trace.append(('read',) + args)
        def write_tracer(*args):
            trace.append(('write',) + args)
        tr1 = v.trace_add('read', read_tracer)
        tr2 = v.trace_add(['write', 'unset'], write_tracer)
        self.assertEqual(sorted(v.trace_info()), [
                         (('read',), tr1),
                         (('write', 'unset'), tr2)])
        self.assertEqual(trace, [])

        v.set('spam')
        self.assertEqual(trace, [('write', vname, '', 'write')])

        trace = []
        v.get()
        self.assertEqual(trace, [('read', vname, '', 'read')])

        trace = []
        info = sorted(v.trace_info())
        v.trace_remove('write', tr1)  # Wrong mode
        self.assertEqual(sorted(v.trace_info()), info)
        with self.assertRaises(TclError):
            v.trace_remove('read', 'spam')  # Wrong command name
        self.assertEqual(sorted(v.trace_info()), info)
        v.get()
        self.assertEqual(trace, [('read', vname, '', 'read')])

        trace = []
        v.trace_remove('read', tr1)
        self.assertEqual(v.trace_info(), [(('write', 'unset'), tr2)])
        v.get()
        self.assertEqual(trace, [])

        trace = []
        del write_tracer
        gc.collect()
        v.set('eggs')
        self.assertEqual(trace, [('write', vname, '', 'write')])

        trace = []
        del v
        gc.collect()
        self.assertEqual(trace, [('write', vname, '', 'unset')])


class TestStringVar(TestBase):

    def test_default(self):
        v = StringVar(self.root)
        self.assertEqual("", v.get())

    def test_get(self):
        v = StringVar(self.root, "abc", "name")
        self.assertEqual("abc", v.get())
        self.root.globalsetvar("name", "value")
        self.assertEqual("value", v.get())

    def test_get_null(self):
        v = StringVar(self.root, "abc\x00def", "name")
        self.assertEqual("abc\x00def", v.get())
        self.root.globalsetvar("name", "val\x00ue")
        self.assertEqual("val\x00ue", v.get())


class TestIntVar(TestBase):

    def test_default(self):
        v = IntVar(self.root)
        self.assertEqual(0, v.get())

    def test_get(self):
        v = IntVar(self.root, 123, "name")
        self.assertEqual(123, v.get())
        self.root.globalsetvar("name", "345")
        self.assertEqual(345, v.get())
        self.root.globalsetvar("name", "876.5")
        self.assertEqual(876, v.get())

    def test_invalid_value(self):
        v = IntVar(self.root, name="name")
        self.root.globalsetvar("name", "value")
        with self.assertRaises((ValueError, TclError)):
            v.get()


class TestDoubleVar(TestBase):

    def test_default(self):
        v = DoubleVar(self.root)
        self.assertEqual(0.0, v.get())

    def test_get(self):
        v = DoubleVar(self.root, 1.23, "name")
        self.assertAlmostEqual(1.23, v.get())
        self.root.globalsetvar("name", "3.45")
        self.assertAlmostEqual(3.45, v.get())

    def test_get_from_int(self):
        v = DoubleVar(self.root, 1.23, "name")
        self.assertAlmostEqual(1.23, v.get())
        self.root.globalsetvar("name", "3.45")
        self.assertAlmostEqual(3.45, v.get())
        self.root.globalsetvar("name", "456")
        self.assertAlmostEqual(456, v.get())

    def test_invalid_value(self):
        v = DoubleVar(self.root, name="name")
        self.root.globalsetvar("name", "value")
        with self.assertRaises((ValueError, TclError)):
            v.get()


class TestBooleanVar(TestBase):

    def test_default(self):
        v = BooleanVar(self.root)
        self.assertIs(v.get(), False)

    def test_get(self):
        v = BooleanVar(self.root, True, "name")
        self.assertIs(v.get(), True)
        self.root.globalsetvar("name", "0")
        self.assertIs(v.get(), False)
        self.root.globalsetvar("name", 42 if self.root.wantobjects() else 1)
        self.assertIs(v.get(), True)
        self.root.globalsetvar("name", 0)
        self.assertIs(v.get(), False)
        self.root.globalsetvar("name", "on")
        self.assertIs(v.get(), True)

    def test_set(self):
        true = 1 if self.root.wantobjects() else "1"
        false = 0 if self.root.wantobjects() else "0"
        v = BooleanVar(self.root, name="name")
        v.set(True)
        self.assertEqual(self.root.globalgetvar("name"), true)
        v.set("0")
        self.assertEqual(self.root.globalgetvar("name"), false)
        v.set(42)
        self.assertEqual(self.root.globalgetvar("name"), true)
        v.set(0)
        self.assertEqual(self.root.globalgetvar("name"), false)
        v.set("on")
        self.assertEqual(self.root.globalgetvar("name"), true)

    def test_invalid_value_domain(self):
        false = 0 if self.root.wantobjects() else "0"
        v = BooleanVar(self.root, name="name")
        with self.assertRaises(TclError):
            v.set("value")
        self.assertEqual(self.root.globalgetvar("name"), false)
        self.root.globalsetvar("name", "value")
        with self.assertRaises(ValueError):
            v.get()
        self.root.globalsetvar("name", "1.0")
        with self.assertRaises(ValueError):
            v.get()


class DefaultRootTest(AbstractDefaultRootTest, unittest.TestCase):

    def test_variable(self):
        self.assertRaises(RuntimeError, Variable)
        root = tkinter.Tk()
        v = Variable()
        v.set("value")
        self.assertEqual(v.get(), "value")
        root.destroy()
        tkinter.NoDefaultRoot()
        self.assertRaises(RuntimeError, Variable)


if __name__ == "__main__":
    unittest.main()
