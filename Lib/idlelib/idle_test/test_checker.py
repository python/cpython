import unittest
from test.support import requires
requires('gui')
from unittest.mock import Mock, call
from idlelib.idle_test.mock_tk import Var, Mbox
from tkinter import Listbox, END, Tk
import idlelib.Checker as checker_module


class Dummy_editwin:
    io = Mock()
    top = None
    extensions = {'ScriptBinding': Mock()}
    flist = None

    def __init__(self, *args, **kwargs):
        self.filename = None


class Dummy_tempfile:
    def __init__(self, delete=False):
        pass

    def __enter__(self, *args, **kwargs):
        pass

    def __exit__(self, *args, **kwargs):
        pass

checker_dialog = checker_module.Checker
config_checker_dialog = checker_module.ConfigCheckerDialog
editwin = Dummy_editwin()
run = checker_module.run_checker(editwin, 'three')

# methods call Mbox.showerror if values are not ok
orig_mbox = checker_module.tkMessageBox
orig_idleConf = checker_module.idleConf
orig_config_checker_dialog = checker_module.ConfigCheckerDialog
orig_get_checker_config = checker_module.get_checker_config
orig_named_temporary_file = checker_module.NamedTemporaryFile
orig_popen = checker_module.Popen
orig_checker_window = checker_module.CheckerWindow
showerror = Mbox.showerror
askyesno = Mbox.askyesno


def setUpModule():
    checker_module.idleConf = idleConf = Mock()
    idleConf.GetSectionList = Mock(return_value=['one', 'two', 'three'])
    attrs = {'userCfg': {'checker': Mock(), 'Save': Mock()},
             'SetOption': Mock()}
    idleConf.configure_mock(**attrs)
    checker_module.tkMessageBox = Mbox


def tearDownModule():
    checker_module.idleConf = orig_idleConf
    checker_module.tkMessageBox = orig_mbox


class Dummy_checker:
    # Mock for testing methods of Checker
    add_checker = checker_dialog.add_checker
    edit_checker = checker_dialog.edit_checker
    remove_checker = checker_dialog.remove_checker
    _selected_checker = checker_dialog._selected_checker
    update_listbox = checker_dialog.update_listbox
    update_menu = Mock()
    editwin = editwin


class Dummy_config_checker_dialog:
    # Mock for testing methods of ConfigCheckerDialog
    name_ok = config_checker_dialog.is_name_ok
    command_ok = config_checker_dialog.is_command_ok
    additional_ok = config_checker_dialog.additional_ok
    update_call_string = config_checker_dialog.update_call_string
    ok = config_checker_dialog.ok
    cancel = config_checker_dialog.cancel

    name = Var()
    command = Var()
    additional = Var()
    reload_source = Var()
    show_result = Var()
    enabled = Var()
    call_string = Var()
    destroyed = False
    editwin = editwin

    def destroy(cls):
        cls.destroyed = True

    def close(cls):
        cls.destroy()


class CheckerUtilityTest(unittest.TestCase):
    def test_get_checkers(self):
        self.assertIsInstance(checker_module.get_checkers(), list)

    def test_get_enabled_checkers(self):
        checkers_list = checker_module.get_checkers()
        enabled_checkers = checker_module.get_enabled_checkers()
        self.assertIsInstance(enabled_checkers, list)
        self.assertTrue(set(enabled_checkers) <= set(checkers_list))

    def test_checker_config(self):
        get = checker_module.get_checker_config
        with self.assertRaises(ValueError) as ve:
            get('')
        self.assertIn('empty', str(ve.exception))
        cfg_list = list(get(checker_module.get_enabled_checkers()[0]))
        for item in ('enabled', 'name', 'command', 'additional',
                     'reload_source', 'show_result'):
            self.assertIn(item, cfg_list, '{} config not found'.format(item))


class RunCheckerTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        checker_module.get_checker_config = Mock()
        checker_module.NamedTemporaryFile = Dummy_tempfile
        checker_module.CheckerWindow = Mock()
        cls.get_config = checker_module.get_checker_config
        script_binding = editwin.extensions.get('ScriptBinding')
        script_binding.getfilename = lambda: editwin.filename

    @classmethod
    def tearDownClass(cls):
        checker_module.get_checker_config = orig_get_checker_config
        checker_module.NamedTemporaryFile = Mock()
        checker_module.CheckerWindow = orig_checker_window
        checker_module.Popen = orig_popen

    def setUp(self):
        self.checker_config = {'name': 'three', 'enabled': True,
                               'command': 'python -m three',
                               'additional': '-v 2 format=True o',
                               'reload_source': 1, 'show_result': 1}
        self.get_config.configure_mock(return_value=self.checker_config)
        editwin.filename = 'foo/bar/filename.py'
        checker_module.Popen = Mock()

    def test_blank_filename(self):
        editwin.filename = ''
        self.assertFalse(run())

    def test_blank_command(self):
        self.checker_config['command'] = ''
        self.get_config.configure_mock(return_value=self.checker_config)
        self.assertFalse(run())
        self.assertEqual('Empty Command', showerror.title)
        self.assertIn('three', showerror.message)
        self.assertIn('Command', showerror.message)

    def test_bad_run(self):
        editwin.filename = 'foo/bar/imaginary_file.py'
        error_message = 'No such file or directory'
        error_filename = 'imaginary_file.py'
        checker_module.Popen = Mock(side_effect=OSError(2,
                                                        error_message,
                                                        error_filename))
        self.assertFalse(run())
        self.assertIn('imaginary_file.py', str(showerror.title))
        self.assertIn('No such file or directory', showerror.message)
        self.assertIn('Traceback', showerror.message)

    def test_good_run(self):
        self.assertTrue(run())
        call_args = checker_module.Popen.call_args
        args = call_args[0][0]
        cwd = call_args[1]['cwd']
        self.assertListEqual(args, ['python', '-m', 'three', '-v', '2',
                                    'format=True', 'o', 'filename.py'])
        self.assertEqual(cwd, 'foo/bar')


class CheckerTest(unittest.TestCase):
    dialog = Dummy_checker()

    @classmethod
    def setUpClass(cls):
        cls.dialog.dialog = cls.dialog
        checker_module.tkMessageBox = Mbox
        checker_module.ConfigCheckerDialog = Mock()
        cls.dialog.root = Tk()
        cls.dialog.listbox = Listbox(cls.dialog.root)

    @classmethod
    def tearDownClass(cls):
        checker_module.ConfigCheckerDialog = orig_config_checker_dialog
        cls.dialog.listbox.destroy()
        cls.dialog.root.destroy()
        del cls.dialog.listbox, cls.dialog.root

    def tearDown(self):
        self.dialog.update_listbox()

    def test_add_checker(self):
        self.dialog.add_checker()
        checker_module.ConfigCheckerDialog.assert_called_with(self.dialog, '')

    def test_bad_edit(self):
        showerror.title = ''
        showerror.message = ''
        self.dialog.listbox.selection_clear(0, END)
        self.dialog.edit_checker()
        self.assertEqual(showerror.title, 'No Checker Selected')
        self.assertIn('existing', showerror.message)

    def test_good_edit(self):
        self.dialog.listbox.selection_set(END)
        self.dialog.edit_checker()
        checker_module.ConfigCheckerDialog.assert_called_with(self.dialog,
                                                              'three')

    def test_bad_remove(self):
        self.dialog.listbox.selection_clear(0, END)
        showerror.title = ''
        showerror.message = ''
        self.dialog.remove_checker()
        self.assertEqual(showerror.title, 'No Checker Selected')
        self.assertIn('existing', showerror.message)

    def test_dont_confirm_remove(self):
        self.dialog.listbox.selection_set(END)
        askyesno.result = False
        self.assertIsNone(self.dialog.remove_checker())
        self.assertIn('Confirm', askyesno.title)
        self.assertIn('three', askyesno.title)
        self.assertIn('three', askyesno.message)
        self.assertIn('remove', askyesno.message)

    def test_good_remove(self):
        self.dialog.listbox.selection_set(END)
        askyesno.result = True
        self.dialog.remove_checker()
        self.assertIn('Confirm', askyesno.title)
        self.assertIn('three', askyesno.title)
        self.assertIn('three', askyesno.message)
        self.assertIn('remove', askyesno.message)
        idleConf = checker_module.idleConf
        remove = idleConf.userCfg['checker'].remove_option
        remove.assert_has_calls([call('three', 'enabled'),
                                 call('three', 'command'),
                                 call('three', 'additional'),
                                 call('three', 'reload_source'),
                                 call('three', 'show_result'),
                                 ])
        save = idleConf.userCfg['checker'].Save
        save.assert_called_with()

    def test_update_listbox(self):
        self.dialog.listbox.delete(0, END)
        self.dialog.update_listbox()
        list_items = self.dialog.listbox.get(0, END)
        self.assertTupleEqual(list_items, ('one', 'two', 'three'))

    def test_selected_checker(self):
        self.dialog.listbox.selection_clear(0, END)
        self.assertIsNone(self.dialog._selected_checker())
        self.dialog.listbox.selection_set("end")
        self.assertEqual(self.dialog._selected_checker(), 'three')


class ConfigCheckerDialogTest(unittest.TestCase):
    dialog = Dummy_config_checker_dialog()

    def test_blank_name(self):
        for name in ('', '   '):
            self.dialog.name.set(name)
            self.assertFalse(self.dialog.name_ok())
            self.assertEqual(showerror.title, 'Name Error')
            self.assertIn('No', showerror.message)

        self.dialog.name.set('    ')
        self.assertFalse(self.dialog.name_ok())
        self.assertEqual(showerror.title, 'Name Error')
        self.assertIn('No', showerror.message)

    def test_good_name(self):
        self.dialog.name.set('pyflakes')
        self.assertTrue(self.dialog.name_ok())

    def test_blank_command(self):
        for command in ('', '   '):
            self.dialog.command.set(command)
            self.assertFalse(self.dialog.command_ok())
            self.assertEqual(showerror.title, 'Command Error')
            self.assertIn('No', showerror.message)

    def test_good_command(self):
        self.dialog.command.set('/bin/pyflakes')
        self.assertTrue(self.dialog.command_ok())

    def test_good_ok(self):
        self.dialog.name.set('bar')
        self.dialog.enabled.set(0)
        self.dialog.command.set('foo')
        self.dialog.additional.set('')
        self.dialog.reload_source.set(0)
        self.dialog.show_result.set(1)
        self.dialog.ok()
        idleConf = checker_module.idleConf
        set_option = idleConf.userCfg['checker'].SetOption
        set_option.assert_has_calls([call('bar', 'enabled', 0),
                                     call('bar', 'command', 'foo'),
                                     call('bar', 'additional', ''),
                                     call('bar', 'reload_source', 0),
                                     call('bar', 'show_result', 1),
                                     ])
        save = idleConf.userCfg['checker'].Save
        save.assert_called_with()

    def test_update_call_string(self):
        self.dialog.editwin.io.filename = '/foo/bar/filename.py'
        self.dialog.command.set('checker')
        self.dialog.additional.set('--help')
        self.dialog.update_call_string()
        self.assertEqual(self.dialog.call_string.get(),
                         'checker --help filename.py')

        self.dialog.editwin.io.filename = None
        self.dialog.update_call_string()
        self.assertEqual(self.dialog.call_string.get(),
                         'checker --help <filename>')

if __name__ == '__main__':
    unittest.main(verbosity=2)
