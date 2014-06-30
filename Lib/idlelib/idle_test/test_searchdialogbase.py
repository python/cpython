'''Unittests for idlelib/SearchDialogBase.py

Coverage: 99%. The only thing not covered is inconsequential --
testing skipping of suite when self.needwrapbutton is false.

'''
import unittest
from test.test_support import requires
from Tkinter import Tk, Toplevel, Frame, Label, BooleanVar, StringVar
from idlelib import SearchEngine as se
from idlelib import SearchDialogBase as sdb
from idlelib.idle_test.mock_idle import Func
from idlelib.idle_test.mock_tk import Var, Mbox

# The following could help make some tests gui-free.
# However, they currently make radiobutton tests fail.
##def setUpModule():
##    # Replace tk objects used to initialize se.SearchEngine.
##    se.BooleanVar = Var
##    se.StringVar = Var
##
##def tearDownModule():
##    se.BooleanVar = BooleanVar
##    se.StringVar = StringVar

class SearchDialogBaseTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()

    @classmethod
    def tearDownClass(cls):
        cls.root.destroy()
        del cls.root

    def setUp(self):
        self.engine = se.SearchEngine(self.root)  # None also seems to work
        self.dialog = sdb.SearchDialogBase(root=self.root, engine=self.engine)

    def tearDown(self):
        self.dialog.close()

    def test_open_and_close(self):
        # open calls create_widgets, which needs default_command
        self.dialog.default_command = None  

        # Since text parameter of .open is not used in base class,
        # pass dummy 'text' instead of tk.Text().
        self.dialog.open('text')
        self.assertEqual(self.dialog.top.state(), 'normal')
        self.dialog.close()
        self.assertEqual(self.dialog.top.state(), 'withdrawn')

        self.dialog.open('text', searchphrase="hello")
        self.assertEqual(self.dialog.ent.get(), 'hello')
        self.dialog.close()

    def test_create_widgets(self):
        self.dialog.create_entries = Func()
        self.dialog.create_option_buttons = Func()
        self.dialog.create_other_buttons = Func()
        self.dialog.create_command_buttons = Func()

        self.dialog.default_command = None
        self.dialog.create_widgets()

        self.assertTrue(self.dialog.create_entries.called)
        self.assertTrue(self.dialog.create_option_buttons.called)
        self.assertTrue(self.dialog.create_other_buttons.called)
        self.assertTrue(self.dialog.create_command_buttons.called)

    def test_make_entry(self):
        equal = self.assertEqual
        self.dialog.row = 0
        self.dialog.top = Toplevel(self.root)
        label, entry = self.dialog.make_entry("Test:", 'hello')
        equal(label.cget('text'), 'Test:')

        self.assertIn(entry.get(), 'hello')
        egi = entry.grid_info()
        equal(egi['row'], '0')
        equal(egi['column'], '1')
        equal(egi['rowspan'], '1')
        equal(egi['columnspan'], '1')
        equal(self.dialog.row, 1)

    def test_create_entries(self):
        self.dialog.row = 0
        self.engine.setpat('hello')
        self.dialog.create_entries()
        self.assertIn(self.dialog.ent.get(), 'hello')

    def test_make_frame(self):
        self.dialog.row = 0
        self.dialog.top = Toplevel(self.root)
        label, frame = self.dialog.make_frame()
        self.assertEqual(label, '')
        self.assertIsInstance(frame, Frame)

        label, labelledframe = self.dialog.make_frame('testlabel')
        self.assertEqual(label.cget('text'), 'testlabel')
        self.assertIsInstance(labelledframe, Frame)

    def btn_test_setup(self, which):
        self.dialog.row = 0
        self.dialog.top = Toplevel(self.root)
        if which == 'option':
            self.dialog.create_option_buttons()
        elif which == 'other':
            self.dialog.create_other_buttons()
        else:
            raise ValueError('bad which arg %s' % which)

    def test_create_option_buttons(self):
        self.btn_test_setup('option')
        self.checkboxtests()

    def test_create_option_buttons_flipped(self):
        for var in ('revar', 'casevar', 'wordvar', 'wrapvar'):
            Var = getattr(self.engine, var)
            Var.set(not Var.get())
        self.btn_test_setup('option')
        self.checkboxtests(flip=1)

    def checkboxtests(self, flip=0):
        """Tests the four checkboxes in the search dialog window."""
        engine = self.engine
        for child in self.dialog.top.winfo_children():
            for grandchild in child.winfo_children():
                text = grandchild.config()['text'][-1]
                if text == ('Regular', 'expression'):
                    self.btnstatetest(grandchild, engine.revar, flip)
                elif text == ('Match', 'case'):
                    self.btnstatetest(grandchild, engine.casevar, flip)
                elif text == ('Whole', 'word'):
                    self.btnstatetest(grandchild, engine.wordvar, flip)
                elif text == ('Wrap', 'around'):
                    self.btnstatetest(grandchild, engine.wrapvar, not flip)

    def btnstatetest(self, button, var, defaultstate):
        self.assertEqual(var.get(), defaultstate)
        if defaultstate == 1:
            button.deselect()
        else:
            button.select()
        self.assertEqual(var.get(), 1 - defaultstate)

    def test_create_other_buttons(self):
        self.btn_test_setup('other')
        self.radiobuttontests()

    def test_create_other_buttons_flipped(self):
        self.engine.backvar.set(1)
        self.btn_test_setup('other')
        self.radiobuttontests(back=1)

    def radiobuttontests(self, back=0):
        searchupbtn = None
        searchdownbtn = None

        for child in self.dialog.top.winfo_children():
            for grandchild in child.children.values():
                text = grandchild.config()['text'][-1]
                if text == 'Up':
                    searchupbtn = grandchild
                elif text == 'Down':
                    searchdownbtn = grandchild

        # Defaults to searching downward
        self.assertEqual(self.engine.backvar.get(), back)
        if back:
            searchdownbtn.select()
        else:
            searchupbtn.select()
        self.assertEqual(self.engine.backvar.get(), not back)
        searchdownbtn.select()

    def test_make_button(self):
        self.dialog.top = Toplevel(self.root)
        self.dialog.buttonframe = Frame(self.dialog.top)
        btn = self.dialog.make_button('Test', self.dialog.close)
        self.assertEqual(btn.cget('text'), 'Test')

    def test_create_command_buttons(self):
        self.dialog.create_command_buttons()
        # Look for close button command in buttonframe
        closebuttoncommand = ''
        for child in self.dialog.buttonframe.winfo_children():
            if child.config()['text'][-1] == 'close':
                closebuttoncommand = child.config()['command'][-1]
        self.assertIn('close', closebuttoncommand)



if __name__ == '__main__':
    unittest.main(verbosity=2, exit=2)
