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
import sys
from tkinter import Tk
import unittest
from unittest import mock
from idlelib.idle_test.mock_tk import Var
from idlelib import query


# NON-GUI TESTS

class QueryTest(unittest.TestCase):
    "Test Query base class."

    class Dummy_Query:
        # Test the following Query methods.
        entry_ok = query.Query.entry_ok
        ok = query.Query.ok
        cancel = query.Query.cancel
        # Add attributes and initialization needed for tests.
        entry = Var()
        entry_error = {}
        def __init__(self, dummy_entry):
            self.entry.set(dummy_entry)
            self.entry_error['text'] = ''
            self.result = None
            self.destroyed = False
        def showerror(self, message):
            self.entry_error['text'] = message
        def destroy(self):
            self.destroyed = True

    def test_entry_ok_blank(self):
        dialog = self.Dummy_Query(' ')
        self.assertEqual(dialog.entry_ok(), None)
        self.assertEqual((dialog.result, dialog.destroyed), (None, False))
        self.assertIn('blank line', dialog.entry_error['text'])

    def test_entry_ok_good(self):
        dialog = self.Dummy_Query('  good ')
        Equal = self.assertEqual
        Equal(dialog.entry_ok(), 'good')
        Equal((dialog.result, dialog.destroyed), (None, False))
        Equal(dialog.entry_error['text'], '')

    def test_ok_blank(self):
        dialog = self.Dummy_Query('')
        dialog.entry.focus_set = mock.Mock()
        self.assertEqual(dialog.ok(), None)
        self.assertTrue(dialog.entry.focus_set.called)
        del dialog.entry.focus_set
        self.assertEqual((dialog.result, dialog.destroyed), (None, False))

    def test_ok_good(self):
        dialog = self.Dummy_Query('good')
        self.assertEqual(dialog.ok(), None)
        self.assertEqual((dialog.result, dialog.destroyed), ('good', True))

    def test_cancel(self):
        dialog = self.Dummy_Query('does not matter')
        self.assertEqual(dialog.cancel(), None)
        self.assertEqual((dialog.result, dialog.destroyed), (None, True))


class SectionNameTest(unittest.TestCase):
    "Test SectionName subclass of Query."

    class Dummy_SectionName:
        entry_ok = query.SectionName.entry_ok  # Function being tested.
        used_names = ['used']
        entry = Var()
        entry_error = {}
        def __init__(self, dummy_entry):
            self.entry.set(dummy_entry)
            self.entry_error['text'] = ''
        def showerror(self, message):
            self.entry_error['text'] = message

    def test_blank_section_name(self):
        dialog = self.Dummy_SectionName(' ')
        self.assertEqual(dialog.entry_ok(), None)
        self.assertIn('no name', dialog.entry_error['text'])

    def test_used_section_name(self):
        dialog = self.Dummy_SectionName('used')
        self.assertEqual(dialog.entry_ok(), None)
        self.assertIn('use', dialog.entry_error['text'])

    def test_long_section_name(self):
        dialog = self.Dummy_SectionName('good'*8)
        self.assertEqual(dialog.entry_ok(), None)
        self.assertIn('longer than 30', dialog.entry_error['text'])

    def test_good_section_name(self):
        dialog = self.Dummy_SectionName('  good ')
        self.assertEqual(dialog.entry_ok(), 'good')
        self.assertEqual(dialog.entry_error['text'], '')


class ModuleNameTest(unittest.TestCase):
    "Test ModuleName subclass of Query."

    class Dummy_ModuleName:
        entry_ok = query.ModuleName.entry_ok  # Function being tested.
        text0 = ''
        entry = Var()
        entry_error = {}
        def __init__(self, dummy_entry):
            self.entry.set(dummy_entry)
            self.entry_error['text'] = ''
        def showerror(self, message):
            self.entry_error['text'] = message

    def test_blank_module_name(self):
        dialog = self.Dummy_ModuleName(' ')
        self.assertEqual(dialog.entry_ok(), None)
        self.assertIn('no name', dialog.entry_error['text'])

    def test_bogus_module_name(self):
        dialog = self.Dummy_ModuleName('__name_xyz123_should_not_exist__')
        self.assertEqual(dialog.entry_ok(), None)
        self.assertIn('not found', dialog.entry_error['text'])

    def test_c_source_name(self):
        dialog = self.Dummy_ModuleName('itertools')
        self.assertEqual(dialog.entry_ok(), None)
        self.assertIn('source-based', dialog.entry_error['text'])

    def test_good_module_name(self):
        dialog = self.Dummy_ModuleName('idlelib')
        self.assertTrue(dialog.entry_ok().endswith('__init__.py'))
        self.assertEqual(dialog.entry_error['text'], '')


# 3 HelpSource test classes each test one function.

orig_platform = query.platform

class HelpsourceBrowsefileTest(unittest.TestCase):
    "Test browse_file method of ModuleName subclass of Query."

    class Dummy_HelpSource:
        browse_file = query.HelpSource.browse_file
        pathvar = Var()

    def test_file_replaces_path(self):
        dialog = self.Dummy_HelpSource()
        # Path is widget entry, either '' or something.
        # Func return is file dialog return, either '' or something.
        # Func return should override widget entry.
        # We need all 4 combination to test all (most) code paths.
        for path, func, result in (
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
    "Test path_ok method of HelpSource subclass of Query."

    class Dummy_HelpSource:
        path_ok = query.HelpSource.path_ok
        path = Var()
        path_error = {}
        def __init__(self, dummy_path):
            self.path.set(dummy_path)
            self.path_error['text'] = ''
        def showerror(self, message, widget=None):
            self.path_error['text'] = message

    @classmethod
    def tearDownClass(cls):
        query.platform = orig_platform

    def test_path_ok_blank(self):
        dialog = self.Dummy_HelpSource(' ')
        self.assertEqual(dialog.path_ok(), None)
        self.assertIn('no help file', dialog.path_error['text'])

    def test_path_ok_bad(self):
        dialog = self.Dummy_HelpSource(__file__ + 'bad-bad-bad')
        self.assertEqual(dialog.path_ok(), None)
        self.assertIn('not exist', dialog.path_error['text'])

    def test_path_ok_web(self):
        dialog = self.Dummy_HelpSource('')
        Equal = self.assertEqual
        for url in 'www.py.org', 'http://py.org':
            with self.subTest():
                dialog.path.set(url)
                self.assertEqual(dialog.path_ok(), url)
                self.assertEqual(dialog.path_error['text'], '')

    def test_path_ok_file(self):
        dialog = self.Dummy_HelpSource('')
        for platform, prefix in ('darwin', 'file://'), ('other', ''):
            with self.subTest():
                query.platform = platform
                dialog.path.set(__file__)
                self.assertEqual(dialog.path_ok(), prefix + __file__)
                self.assertEqual(dialog.path_error['text'], '')


class HelpsourceEntryokTest(unittest.TestCase):
    "Test entry_ok method of HelpSource subclass of Query."

    class Dummy_HelpSource:
        entry_ok = query.HelpSource.entry_ok
        entry_error = {}
        path_error = {}
        def item_ok(self):
            return self.name
        def path_ok(self):
            return self.path

    def test_entry_ok_helpsource(self):
        dialog = self.Dummy_HelpSource()
        for name, path, result in ((None, None, None),
                                   (None, 'doc.txt', None),
                                   ('doc', None, None),
                                   ('doc', 'doc.txt', ('doc', 'doc.txt'))):
            with self.subTest():
                dialog.name, dialog.path = name, path
                self.assertEqual(dialog.entry_ok(), result)


# GUI TESTS

class QueryGuiTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = root = Tk()
        cls.root.withdraw()
        cls.dialog = query.Query(root, 'TEST', 'test', _utest=True)
        cls.dialog.destroy = mock.Mock()

    @classmethod
    def tearDownClass(cls):
        del cls.dialog.destroy
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
        root.withdraw()
        dialog =  query.SectionName(root, 'T', 't', {'abc'}, _utest=True)
        Equal = self.assertEqual
        self.assertEqual(dialog.used_names, {'abc'})
        dialog.entry.insert(0, 'okay')
        dialog.button_ok.invoke()
        self.assertEqual(dialog.result, 'okay')
        del dialog
        root.destroy()
        del root


class ModulenameGuiTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')

    def test_click_module_name(self):
        root = Tk()
        root.withdraw()
        dialog =  query.ModuleName(root, 'T', 't', 'idlelib', _utest=True)
        self.assertEqual(dialog.text0, 'idlelib')
        self.assertEqual(dialog.entry.get(), 'idlelib')
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
        root.withdraw()
        dialog =  query.HelpSource(root, 'T', menuitem='__test__',
                                   filepath=__file__, _utest=True)
        Equal = self.assertEqual
        Equal(dialog.entry.get(), '__test__')
        Equal(dialog.path.get(), __file__)
        dialog.button_ok.invoke()
        prefix = "file://" if sys.platform == 'darwin' else ''
        Equal(dialog.result, ('__test__', prefix + __file__))
        del dialog
        root.destroy()
        del root


if __name__ == '__main__':
    unittest.main(verbosity=2, exit=False)
