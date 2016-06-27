"""Test idlelib.query.

Coverage: 100%.
"""
from test.support import requires
from tkinter import Tk
import unittest
from unittest import mock
from idlelib.idle_test.mock_tk import Var, Mbox_func
from idlelib import query
Query, SectionName = query.Query, query.SectionName

class Dummy_Query:
    # Mock for testing the following methods Query
    entry_ok = Query.entry_ok
    ok = Query.ok
    cancel = Query.cancel
    # Attributes, constant or variable, needed for tests
    entry = Var()
    result = None
    destroyed = False
    def destroy(self):
        self.destroyed = True

# entry_ok calls modal messagebox.showerror if entry is not ok.
# Mock showerrer returns, so don't need to click to continue.
orig_showerror = query.showerror
showerror = Mbox_func()  # Instance has __call__ method.

def setUpModule():
    query.showerror = showerror

def tearDownModule():
    query.showerror = orig_showerror


class QueryTest(unittest.TestCase):
    dialog = Dummy_Query()

    def setUp(self):
        showerror.title = None
        self.dialog.result = None
        self.dialog.destroyed = False

    def test_blank_entry(self):
        dialog = self.dialog
        Equal = self.assertEqual
        dialog.entry.set(' ')
        Equal(dialog.entry_ok(), '')
        Equal((dialog.result, dialog.destroyed), (None, False))
        Equal(showerror.title, 'Entry Error')
        self.assertIn('Blank', showerror.message)

    def test_good_entry(self):
        dialog = self.dialog
        Equal = self.assertEqual
        dialog.entry.set('  good ')
        Equal(dialog.entry_ok(), 'good')
        Equal((dialog.result, dialog.destroyed), (None, False))
        Equal(showerror.title, None)

    def test_ok(self):
        dialog = self.dialog
        Equal = self.assertEqual
        dialog.entry.set('good')
        Equal(dialog.ok(), None)
        Equal((dialog.result, dialog.destroyed), ('good', True))

    def test_cancel(self):
        dialog = self.dialog
        Equal = self.assertEqual
        Equal(self.dialog.cancel(), None)
        Equal((dialog.result, dialog.destroyed), (None, True))


class Dummy_SectionName:
    # Mock for testing the following method of Section_Name
    entry_ok = SectionName.entry_ok
    # Attributes, constant or variable, needed for tests
    used_names = ['used']
    entry = Var()

class SectionNameTest(unittest.TestCase):
    dialog = Dummy_SectionName()


    def setUp(self):
        showerror.title = None

    def test_blank_name(self):
        dialog = self.dialog
        Equal = self.assertEqual
        dialog.entry.set(' ')
        Equal(dialog.entry_ok(), '')
        Equal(showerror.title, 'Name Error')
        self.assertIn('No', showerror.message)

    def test_used_name(self):
        dialog = self.dialog
        Equal = self.assertEqual
        dialog.entry.set('used')
        Equal(self.dialog.entry_ok(), '')
        Equal(showerror.title, 'Name Error')
        self.assertIn('use', showerror.message)

    def test_long_name(self):
        dialog = self.dialog
        Equal = self.assertEqual
        dialog.entry.set('good'*8)
        Equal(self.dialog.entry_ok(), '')
        Equal(showerror.title, 'Name Error')
        self.assertIn('too long', showerror.message)

    def test_good_entry(self):
        dialog = self.dialog
        Equal = self.assertEqual
        dialog.entry.set('  good ')
        Equal(dialog.entry_ok(), 'good')
        Equal(showerror.title, None)


class QueryGuiTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.dialog = Query(cls.root, 'TEST', 'test', _utest=True)
        cls.dialog.destroy = mock.Mock()

    @classmethod
    def tearDownClass(cls):
        del cls.dialog
        cls.root.destroy()
        del cls.root

    def setUp(self):
        self.dialog.entry.delete(0, 'end')
        self.dialog.result = None
        self.dialog.destroy.reset_mock()

    def test_click_ok(self):
        dialog = self.dialog
        dialog.entry.insert(0, 'abc')
        dialog.button_ok.invoke()
        self.assertEqual(dialog.result, 'abc')
        self.assertTrue(dialog.destroy.called)

    def test_click_blank(self):
        dialog = self.dialog
        dialog.button_ok.invoke()
        self.assertEqual(dialog.result, None)
        self.assertFalse(dialog.destroy.called)

    def test_click_cancel(self):
        dialog = self.dialog
        dialog.entry.insert(0, 'abc')
        dialog.button_cancel.invoke()
        self.assertEqual(dialog.result, None)
        self.assertTrue(dialog.destroy.called)


if __name__ == '__main__':
    unittest.main(verbosity=2, exit=False)
