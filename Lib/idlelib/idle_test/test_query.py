"""Test idlelib.query.

Non-gui tests for Query, SectionName, ModuleName, and HelpSource use
dummy versions that extract the non-gui methods and add other needed
attributes.  GUI tests create an instance of each class and simulate
entries and button clicks.  Subclass tests only target the new code in
the subclass definition.

The appearance of the widgets is checked by the Query and
HelpSource htests.  These are run by running query.py.

Coverage: 94% (100% for Query and SectionName).
6 of 8 missing are ModuleName exceptions I don't know how to trigger.
"""
from test.support import requires
from tkinter import Tk
import unittest
from unittest import mock
from idlelib.idle_test.mock_tk import Var, Mbox_func
from idlelib import query

# Mock entry.showerror messagebox so don't need click to continue
# when entry_ok and path_ok methods call it to display errors.

orig_showerror = query.showerror
showerror = Mbox_func()  # Instance has __call__ method.

def setUpModule():
    query.showerror = showerror

def tearDownModule():
    query.showerror = orig_showerror


# NON-GUI TESTS

class QueryTest(unittest.TestCase):
    "Test Query base class."

    class Dummy_Query:
        # Test the following Query methods.
        entry_ok = query.Query.entry_ok
        ok = query.Query.ok
        cancel = query.Query.cancel
        # Add attributes needed for the tests.
        entry = Var()
        result = None
        destroyed = False
        def destroy(self):
            self.destroyed = True

    dialog = Dummy_Query()

    def setUp(self):
        showerror.title = None
        self.dialog.result = None
        self.dialog.destroyed = False

    def test_entry_ok_blank(self):
        dialog = self.dialog
        Equal = self.assertEqual
        dialog.entry.set(' ')
        Equal(dialog.entry_ok(), None)
        Equal((dialog.result, dialog.destroyed), (None, False))
        Equal(showerror.title, 'Entry Error')
        self.assertIn('Blank', showerror.message)

    def test_entry_ok_good(self):
        dialog = self.dialog
        Equal = self.assertEqual
        dialog.entry.set('  good ')
        Equal(dialog.entry_ok(), 'good')
        Equal((dialog.result, dialog.destroyed), (None, False))
        Equal(showerror.title, None)

    def test_ok_blank(self):
        dialog = self.dialog
        Equal = self.assertEqual
        dialog.entry.set('')
        dialog.entry.focus_set = mock.Mock()
        Equal(dialog.ok(), None)
        self.assertTrue(dialog.entry.focus_set.called)
        del dialog.entry.focus_set
        Equal((dialog.result, dialog.destroyed), (None, False))

    def test_ok_good(self):
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


class SectionNameTest(unittest.TestCase):
    "Test SectionName subclass of Query."

    class Dummy_SectionName:
        entry_ok = query.SectionName.entry_ok  # Function being tested.
        used_names = ['used']
        entry = Var()

    dialog = Dummy_SectionName()

    def setUp(self):
        showerror.title = None

    def test_blank_section_name(self):
        dialog = self.dialog
        Equal = self.assertEqual
        dialog.entry.set(' ')
        Equal(dialog.entry_ok(), None)
        Equal(showerror.title, 'Name Error')
        self.assertIn('No', showerror.message)

    def test_used_section_name(self):
        dialog = self.dialog
        Equal = self.assertEqual
        dialog.entry.set('used')
        Equal(self.dialog.entry_ok(), None)
        Equal(showerror.title, 'Name Error')
        self.assertIn('use', showerror.message)

    def test_long_section_name(self):
        dialog = self.dialog
        Equal = self.assertEqual
        dialog.entry.set('good'*8)
        Equal(self.dialog.entry_ok(), None)
        Equal(showerror.title, 'Name Error')
        self.assertIn('too long', showerror.message)

    def test_good_section_name(self):
        dialog = self.dialog
        Equal = self.assertEqual
        dialog.entry.set('  good ')
        Equal(dialog.entry_ok(), 'good')
        Equal(showerror.title, None)


class ModuleNameTest(unittest.TestCase):
    "Test ModuleName subclass of Query."

    class Dummy_ModuleName:
        entry_ok = query.ModuleName.entry_ok  # Funtion being tested.
        text0 = ''
        entry = Var()

    dialog = Dummy_ModuleName()

    def setUp(self):
        showerror.title = None

    def test_blank_module_name(self):
        dialog = self.dialog
        Equal = self.assertEqual
        dialog.entry.set(' ')
        Equal(dialog.entry_ok(), None)
        Equal(showerror.title, 'Name Error')
        self.assertIn('No', showerror.message)

    def test_bogus_module_name(self):
        dialog = self.dialog
        Equal = self.assertEqual
        dialog.entry.set('__name_xyz123_should_not_exist__')
        Equal(self.dialog.entry_ok(), None)
        Equal(showerror.title, 'Import Error')
        self.assertIn('not found', showerror.message)

    def test_c_source_name(self):
        dialog = self.dialog
        Equal = self.assertEqual
        dialog.entry.set('itertools')
        Equal(self.dialog.entry_ok(), None)
        Equal(showerror.title, 'Import Error')
        self.assertIn('source-based', showerror.message)

    def test_good_module_name(self):
        dialog = self.dialog
        Equal = self.assertEqual
        dialog.entry.set('idlelib')
        self.assertTrue(dialog.entry_ok().endswith('__init__.py'))
        Equal(showerror.title, None)


# 3 HelpSource test classes each test one function.

orig_platform = query.platform

class HelpsourceBrowsefileTest(unittest.TestCase):
    "Test browse_file method of ModuleName subclass of Query."

    class Dummy_HelpSource:
        browse_file = query.HelpSource.browse_file
        pathvar = Var()  

    dialog = Dummy_HelpSource()

    def test_file_replaces_path(self):
        # Path is widget entry, file is file dialog return.
        dialog = self.dialog
        for path, func, result in (
                # We need all combination to test all (most) code paths.
                ('', lambda a,b,c:'', ''),
                ('', lambda a,b,c: __file__, __file__),
                ('htest', lambda a,b,c:'', 'htest'),
                ('htest', lambda a,b,c: __file__, __file__)):
            with self.subTest():
                dialog.pathvar.set(path)
                dialog.askfilename = func
                dialog.browse_file()
                self.assertEqual(dialog.pathvar.get(), result)


class HelpsourcePathokTest(unittest.TestCase):
    "Test path_ok method of ModuleName subclass of Query."

    class Dummy_HelpSource:
        path_ok = query.HelpSource.path_ok
        path = Var()

    dialog = Dummy_HelpSource()

    @classmethod
    def tearDownClass(cls):
        query.platform = orig_platform

    def setUp(self):
        showerror.title = None

    def test_path_ok_blank(self):
        dialog = self.dialog
        Equal = self.assertEqual
        dialog.path.set(' ')
        Equal(dialog.path_ok(), None)
        Equal(showerror.title, 'File Path Error')
        self.assertIn('No help', showerror.message)

    def test_path_ok_bad(self):
        dialog = self.dialog
        Equal = self.assertEqual
        dialog.path.set(__file__ + 'bad-bad-bad')
        Equal(dialog.path_ok(), None)
        Equal(showerror.title, 'File Path Error')
        self.assertIn('not exist', showerror.message)

    def test_path_ok_web(self):
        dialog = self.dialog
        Equal = self.assertEqual
        for url in 'www.py.org', 'http://py.org':
            with self.subTest():
                dialog.path.set(url)
                Equal(dialog.path_ok(), url)
                Equal(showerror.title, None)

    def test_path_ok_file(self):
        dialog = self.dialog
        Equal = self.assertEqual
        for platform, prefix in ('darwin', 'file://'), ('other', ''):
            with self.subTest():
                query.platform = platform
                dialog.path.set(__file__)
                Equal(dialog.path_ok(), prefix + __file__)
                Equal(showerror.title, None)


class HelpsourceEntryokTest(unittest.TestCase):
    "Test entry_ok method of ModuleName subclass of Query."

    class Dummy_HelpSource:
        entry_ok = query.HelpSource.entry_ok
        def item_ok(self):
            return self.name
        def path_ok(self):
            return self.path

    dialog = Dummy_HelpSource()

    def test_entry_ok_helpsource(self):
        dialog = self.dialog
        for name, path, result in ((None, None, None),
                                   (None, 'doc.txt', None),
                                   ('doc', None, None),
                                   ('doc', 'doc.txt', ('doc', 'doc.txt'))):
            with self.subTest():
                dialog.name, dialog.path = name, path
                self.assertEqual(self.dialog.entry_ok(), result)


# GUI TESTS

class QueryGuiTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = root = Tk()
        cls.dialog = query.Query(root, 'TEST', 'test', _utest=True)
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


class SectionnameGuiTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')

    def test_click_section_name(self):
        root = Tk()
        dialog =  query.SectionName(root, 'T', 't', {'abc'}, _utest=True)
        Equal = self.assertEqual
        Equal(dialog.used_names, {'abc'})
        dialog.entry.insert(0, 'okay')
        dialog.button_ok.invoke()
        Equal(dialog.result, 'okay')
        del dialog
        root.destroy()
        del root


class ModulenameGuiTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')

    def test_click_module_name(self):
        root = Tk()
        dialog =  query.ModuleName(root, 'T', 't', 'idlelib', _utest=True)
        Equal = self.assertEqual
        Equal(dialog.text0, 'idlelib')
        Equal(dialog.entry.get(), 'idlelib')
        dialog.button_ok.invoke()
        self.assertTrue(dialog.result.endswith('__init__.py'))
        del dialog
        root.destroy()
        del root


class HelpsourceGuiTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')

    def test_click_help_source(self):
        root = Tk()
        dialog =  query.HelpSource(root, 'T', menuitem='__test__',
                                   filepath=__file__, _utest=True)
        Equal = self.assertEqual
        Equal(dialog.entry.get(), '__test__')
        Equal(dialog.path.get(), __file__)
        dialog.button_ok.invoke()
        Equal(dialog.result, ('__test__', __file__))
        del dialog
        root.destroy()
        del root


if __name__ == '__main__':
    unittest.main(verbosity=2, exit=False)
