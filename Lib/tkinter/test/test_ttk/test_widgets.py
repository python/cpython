import unittest
import tkinter
from tkinter import ttk
from test.support import requires, run_unittest

import tkinter.test.support as support
from tkinter.test.test_ttk.test_functions import MockTclObj, MockStateSpec

requires('gui')

class WidgetTest(unittest.TestCase):
    """Tests methods available in every ttk widget."""

    def setUp(self):
        support.root_deiconify()
        self.widget = ttk.Button()
        self.widget.pack()
        self.widget.wait_visibility()

    def tearDown(self):
        self.widget.destroy()
        support.root_withdraw()


    def test_identify(self):
        self.widget.update_idletasks()
        self.failUnlessEqual(self.widget.identify(5, 5), "label")
        self.failUnlessEqual(self.widget.identify(-1, -1), "")

        self.failUnlessRaises(tkinter.TclError, self.widget.identify, None, 5)
        self.failUnlessRaises(tkinter.TclError, self.widget.identify, 5, None)
        self.failUnlessRaises(tkinter.TclError, self.widget.identify, 5, '')


    def test_widget_state(self):
        # XXX not sure about the portability of all these tests
        self.failUnlessEqual(self.widget.state(), ())
        self.failUnlessEqual(self.widget.instate(['!disabled']), True)

        # changing from !disabled to disabled
        self.failUnlessEqual(self.widget.state(['disabled']), ('!disabled', ))
        # no state change
        self.failUnlessEqual(self.widget.state(['disabled']), ())
        # change back to !disable but also active
        self.failUnlessEqual(self.widget.state(['!disabled', 'active']),
            ('!active', 'disabled'))
        # no state changes, again
        self.failUnlessEqual(self.widget.state(['!disabled', 'active']), ())
        self.failUnlessEqual(self.widget.state(['active', '!disabled']), ())

        def test_cb(arg1, **kw):
            return arg1, kw
        self.failUnlessEqual(self.widget.instate(['!disabled'],
            test_cb, "hi", **{"msg": "there"}),
            ('hi', {'msg': 'there'}))

        # attempt to set invalid statespec
        currstate = self.widget.state()
        self.failUnlessRaises(tkinter.TclError, self.widget.instate,
            ['badstate'])
        self.failUnlessRaises(tkinter.TclError, self.widget.instate,
            ['disabled', 'badstate'])
        # verify that widget didn't change its state
        self.failUnlessEqual(currstate, self.widget.state())

        # ensuring that passing None as state doesn't modify current state
        self.widget.state(['active', '!disabled'])
        self.failUnlessEqual(self.widget.state(), ('active', ))


class ButtonTest(unittest.TestCase):

    def test_invoke(self):
        success = []
        btn = ttk.Button(command=lambda: success.append(1))
        btn.invoke()
        self.failUnless(success)


class CheckbuttonTest(unittest.TestCase):

    def test_invoke(self):
        success = []
        def cb_test():
            success.append(1)
            return "cb test called"

        cbtn = ttk.Checkbutton(command=cb_test)
        # the variable automatically created by ttk.Checkbutton is actually
        # undefined till we invoke the Checkbutton
        self.failUnlessEqual(cbtn.state(), ('alternate', ))
        self.failUnlessRaises(tkinter.TclError, cbtn.tk.globalgetvar,
            cbtn['variable'])

        res = cbtn.invoke()
        self.failUnlessEqual(res, "cb test called")
        self.failUnlessEqual(cbtn['onvalue'],
            cbtn.tk.globalgetvar(cbtn['variable']))
        self.failUnless(success)

        cbtn['command'] = ''
        res = cbtn.invoke()
        self.failUnlessEqual(res, '')
        self.failIf(len(success) > 1)
        self.failUnlessEqual(cbtn['offvalue'],
            cbtn.tk.globalgetvar(cbtn['variable']))


class ComboboxTest(unittest.TestCase):

    def setUp(self):
        support.root_deiconify()
        self.combo = ttk.Combobox()

    def tearDown(self):
        self.combo.destroy()
        support.root_withdraw()

    def _show_drop_down_listbox(self):
        width = self.combo.winfo_width()
        self.combo.event_generate('<ButtonPress-1>', x=width - 5, y=5)
        self.combo.event_generate('<ButtonRelease-1>', x=width - 5, y=5)
        self.combo.update_idletasks()


    def test_virtual_event(self):
        success = []

        self.combo['values'] = [1]
        self.combo.bind('<<ComboboxSelected>>',
            lambda evt: success.append(True))
        self.combo.pack()
        self.combo.wait_visibility()

        height = self.combo.winfo_height()
        self._show_drop_down_listbox()
        self.combo.update()
        self.combo.event_generate('<Return>')
        self.combo.update()

        self.failUnless(success)


    def test_postcommand(self):
        success = []

        self.combo['postcommand'] = lambda: success.append(True)
        self.combo.pack()
        self.combo.wait_visibility()

        self._show_drop_down_listbox()
        self.failUnless(success)

        # testing postcommand removal
        self.combo['postcommand'] = ''
        self._show_drop_down_listbox()
        self.failUnlessEqual(len(success), 1)


    def test_values(self):
        def check_get_current(getval, currval):
            self.failUnlessEqual(self.combo.get(), getval)
            self.failUnlessEqual(self.combo.current(), currval)

        check_get_current('', -1)

        self.combo['values'] = ['a', 1, 'c']

        self.combo.set('c')
        check_get_current('c', 2)

        self.combo.current(0)
        check_get_current('a', 0)

        self.combo.set('d')
        check_get_current('d', -1)

        # testing values with empty string
        self.combo.set('')
        self.combo['values'] = (1, 2, '', 3)
        check_get_current('', 2)

        # testing values with empty string set through configure
        self.combo.configure(values=[1, '', 2])
        self.failUnlessEqual(self.combo['values'], ('1', '', '2'))

        # out of range
        self.failUnlessRaises(tkinter.TclError, self.combo.current,
            len(self.combo['values']))
        # it expects an integer (or something that can be converted to int)
        self.failUnlessRaises(tkinter.TclError, self.combo.current, '')

        # testing creating combobox with empty string in values
        combo2 = ttk.Combobox(values=[1, 2, ''])
        self.failUnlessEqual(combo2['values'], ('1', '2', ''))
        combo2.destroy()


class EntryTest(unittest.TestCase):

    def setUp(self):
        support.root_deiconify()
        self.entry = ttk.Entry()

    def tearDown(self):
        self.entry.destroy()
        support.root_withdraw()


    def test_bbox(self):
        self.failUnlessEqual(len(self.entry.bbox(0)), 4)
        for item in self.entry.bbox(0):
            self.failUnless(isinstance(item, int))

        self.failUnlessRaises(tkinter.TclError, self.entry.bbox, 'noindex')
        self.failUnlessRaises(tkinter.TclError, self.entry.bbox, None)


    def test_identify(self):
        self.entry.pack()
        self.entry.wait_visibility()
        self.entry.update_idletasks()

        self.failUnlessEqual(self.entry.identify(5, 5), "textarea")
        self.failUnlessEqual(self.entry.identify(-1, -1), "")

        self.failUnlessRaises(tkinter.TclError, self.entry.identify, None, 5)
        self.failUnlessRaises(tkinter.TclError, self.entry.identify, 5, None)
        self.failUnlessRaises(tkinter.TclError, self.entry.identify, 5, '')


    def test_validation_options(self):
        success = []
        test_invalid = lambda: success.append(True)

        self.entry['validate'] = 'none'
        self.entry['validatecommand'] = lambda: False

        self.entry['invalidcommand'] = test_invalid
        self.entry.validate()
        self.failUnless(success)

        self.entry['invalidcommand'] = ''
        self.entry.validate()
        self.failUnlessEqual(len(success), 1)

        self.entry['invalidcommand'] = test_invalid
        self.entry['validatecommand'] = lambda: True
        self.entry.validate()
        self.failUnlessEqual(len(success), 1)

        self.entry['validatecommand'] = ''
        self.entry.validate()
        self.failUnlessEqual(len(success), 1)

        self.entry['validatecommand'] = True
        self.failUnlessRaises(tkinter.TclError, self.entry.validate)


    def test_validation(self):
        validation = []
        def validate(to_insert):
            if not 'a' <= to_insert.lower() <= 'z':
                validation.append(False)
                return False
            validation.append(True)
            return True

        self.entry['validate'] = 'key'
        self.entry['validatecommand'] = self.entry.register(validate), '%S'

        self.entry.insert('end', 1)
        self.entry.insert('end', 'a')
        self.failUnlessEqual(validation, [False, True])
        self.failUnlessEqual(self.entry.get(), 'a')


    def test_revalidation(self):
        def validate(content):
            for letter in content:
                if not 'a' <= letter.lower() <= 'z':
                    return False
            return True

        self.entry['validatecommand'] = self.entry.register(validate), '%P'

        self.entry.insert('end', 'avocado')
        self.failUnlessEqual(self.entry.validate(), True)
        self.failUnlessEqual(self.entry.state(), ())

        self.entry.delete(0, 'end')
        self.failUnlessEqual(self.entry.get(), '')

        self.entry.insert('end', 'a1b')
        self.failUnlessEqual(self.entry.validate(), False)
        self.failUnlessEqual(self.entry.state(), ('invalid', ))

        self.entry.delete(1)
        self.failUnlessEqual(self.entry.validate(), True)
        self.failUnlessEqual(self.entry.state(), ())


class PanedwindowTest(unittest.TestCase):

    def setUp(self):
        support.root_deiconify()
        self.paned = ttk.Panedwindow()

    def tearDown(self):
        self.paned.destroy()
        support.root_withdraw()


    def test_add(self):
        # attempt to add a child that is not a direct child of the paned window
        label = ttk.Label(self.paned)
        child = ttk.Label(label)
        self.failUnlessRaises(tkinter.TclError, self.paned.add, child)
        label.destroy()
        child.destroy()
        # another attempt
        label = ttk.Label()
        child = ttk.Label(label)
        self.failUnlessRaises(tkinter.TclError, self.paned.add, child)
        child.destroy()
        label.destroy()

        good_child = ttk.Label()
        self.paned.add(good_child)
        # re-adding a child is not accepted
        self.failUnlessRaises(tkinter.TclError, self.paned.add, good_child)

        other_child = ttk.Label(self.paned)
        self.paned.add(other_child)
        self.failUnlessEqual(self.paned.pane(0), self.paned.pane(1))
        self.failUnlessRaises(tkinter.TclError, self.paned.pane, 2)
        good_child.destroy()
        other_child.destroy()
        self.failUnlessRaises(tkinter.TclError, self.paned.pane, 0)


    def test_forget(self):
        self.failUnlessRaises(tkinter.TclError, self.paned.forget, None)
        self.failUnlessRaises(tkinter.TclError, self.paned.forget, 0)

        self.paned.add(ttk.Label())
        self.paned.forget(0)
        self.failUnlessRaises(tkinter.TclError, self.paned.forget, 0)


    def test_insert(self):
        self.failUnlessRaises(tkinter.TclError, self.paned.insert, None, 0)
        self.failUnlessRaises(tkinter.TclError, self.paned.insert, 0, None)
        self.failUnlessRaises(tkinter.TclError, self.paned.insert, 0, 0)

        child = ttk.Label()
        child2 = ttk.Label()
        child3 = ttk.Label()

        self.failUnlessRaises(tkinter.TclError, self.paned.insert, 0, child)

        self.paned.insert('end', child2)
        self.paned.insert(0, child)
        self.failUnlessEqual(self.paned.panes(), (str(child), str(child2)))

        self.paned.insert(0, child2)
        self.failUnlessEqual(self.paned.panes(), (str(child2), str(child)))

        self.paned.insert('end', child3)
        self.failUnlessEqual(self.paned.panes(),
            (str(child2), str(child), str(child3)))

        # reinserting a child should move it to its current position
        panes = self.paned.panes()
        self.paned.insert('end', child3)
        self.failUnlessEqual(panes, self.paned.panes())

        # moving child3 to child2 position should result in child2 ending up
        # in previous child position and child ending up in previous child3
        # position
        self.paned.insert(child2, child3)
        self.failUnlessEqual(self.paned.panes(),
            (str(child3), str(child2), str(child)))


    def test_pane(self):
        self.failUnlessRaises(tkinter.TclError, self.paned.pane, 0)

        child = ttk.Label()
        self.paned.add(child)
        self.failUnless(isinstance(self.paned.pane(0), dict))
        self.failUnlessEqual(self.paned.pane(0, weight=None), 0)
        # newer form for querying a single option
        self.failUnlessEqual(self.paned.pane(0, 'weight'), 0)
        self.failUnlessEqual(self.paned.pane(0), self.paned.pane(str(child)))

        self.failUnlessRaises(tkinter.TclError, self.paned.pane, 0,
            badoption='somevalue')


    def test_sashpos(self):
        self.failUnlessRaises(tkinter.TclError, self.paned.sashpos, None)
        self.failUnlessRaises(tkinter.TclError, self.paned.sashpos, '')
        self.failUnlessRaises(tkinter.TclError, self.paned.sashpos, 0)

        child = ttk.Label(self.paned, text='a')
        self.paned.add(child, weight=1)
        self.failUnlessRaises(tkinter.TclError, self.paned.sashpos, 0)
        child2 = ttk.Label(self.paned, text='b')
        self.paned.add(child2)
        self.failUnlessRaises(tkinter.TclError, self.paned.sashpos, 1)

        self.paned.pack(expand=True, fill='both')
        self.paned.wait_visibility()

        curr_pos = self.paned.sashpos(0)
        self.paned.sashpos(0, 1000)
        self.failUnless(curr_pos != self.paned.sashpos(0))
        self.failUnless(isinstance(self.paned.sashpos(0), int))


class RadiobuttonTest(unittest.TestCase):

    def test_invoke(self):
        success = []
        def cb_test():
            success.append(1)
            return "cb test called"

        myvar = tkinter.IntVar()
        cbtn = ttk.Radiobutton(command=cb_test, variable=myvar, value=0)
        cbtn2 = ttk.Radiobutton(command=cb_test, variable=myvar, value=1)

        res = cbtn.invoke()
        self.failUnlessEqual(res, "cb test called")
        self.failUnlessEqual(cbtn['value'], myvar.get())
        self.failUnlessEqual(myvar.get(),
            cbtn.tk.globalgetvar(cbtn['variable']))
        self.failUnless(success)

        cbtn2['command'] = ''
        res = cbtn2.invoke()
        self.failUnlessEqual(res, '')
        self.failIf(len(success) > 1)
        self.failUnlessEqual(cbtn2['value'], myvar.get())
        self.failUnlessEqual(myvar.get(),
            cbtn.tk.globalgetvar(cbtn['variable']))

        self.failUnlessEqual(str(cbtn['variable']), str(cbtn2['variable']))



class ScaleTest(unittest.TestCase):

    def setUp(self):
        support.root_deiconify()
        self.scale = ttk.Scale()
        self.scale.pack()
        self.scale.update()

    def tearDown(self):
        self.scale.destroy()
        support.root_withdraw()


    def test_custom_event(self):
        failure = [1, 1, 1] # will need to be empty

        funcid = self.scale.bind('<<RangeChanged>>', lambda evt: failure.pop())

        self.scale['from'] = 10
        self.scale['from_'] = 10
        self.scale['to'] = 3

        self.failIf(failure)

        failure = [1, 1, 1]
        self.scale.configure(from_=2, to=5)
        self.scale.configure(from_=0, to=-2)
        self.scale.configure(to=10)

        self.failIf(failure)


    def test_get(self):
        scale_width = self.scale.winfo_width()
        self.failUnlessEqual(self.scale.get(scale_width, 0), self.scale['to'])

        self.failUnlessEqual(self.scale.get(0, 0), self.scale['from'])
        self.failUnlessEqual(self.scale.get(), self.scale['value'])
        self.scale['value'] = 30
        self.failUnlessEqual(self.scale.get(), self.scale['value'])

        self.failUnlessRaises(tkinter.TclError, self.scale.get, '', 0)
        self.failUnlessRaises(tkinter.TclError, self.scale.get, 0, '')


    def test_set(self):
        # set restricts the max/min values according to the current range
        max = self.scale['to']
        new_max = max + 10
        self.scale.set(new_max)
        self.failUnlessEqual(self.scale.get(), max)
        min = self.scale['from']
        self.scale.set(min - 1)
        self.failUnlessEqual(self.scale.get(), min)

        # changing directly the variable doesn't impose this limitation tho
        var = tkinter.DoubleVar()
        self.scale['variable'] = var
        var.set(max + 5)
        self.failUnlessEqual(self.scale.get(), var.get())
        self.failUnlessEqual(self.scale.get(), max + 5)
        del var

        # the same happens with the value option
        self.scale['value'] = max + 10
        self.failUnlessEqual(self.scale.get(), max + 10)
        self.failUnlessEqual(self.scale.get(), self.scale['value'])

        # nevertheless, note that the max/min values we can get specifying
        # x, y coords are the ones according to the current range
        self.failUnlessEqual(self.scale.get(0, 0), min)
        self.failUnlessEqual(self.scale.get(self.scale.winfo_width(), 0), max)

        self.failUnlessRaises(tkinter.TclError, self.scale.set, None)


class NotebookTest(unittest.TestCase):

    def setUp(self):
        support.root_deiconify()
        self.nb = ttk.Notebook()
        self.child1 = ttk.Label()
        self.child2 = ttk.Label()
        self.nb.add(self.child1, text='a')
        self.nb.add(self.child2, text='b')

    def tearDown(self):
        self.child1.destroy()
        self.child2.destroy()
        self.nb.destroy()
        support.root_withdraw()


    def test_tab_identifiers(self):
        self.nb.forget(0)
        self.nb.hide(self.child2)
        self.failUnlessRaises(tkinter.TclError, self.nb.tab, self.child1)
        self.failUnlessEqual(self.nb.index('end'), 1)
        self.nb.add(self.child2)
        self.failUnlessEqual(self.nb.index('end'), 1)
        self.nb.select(self.child2)

        self.failUnless(self.nb.tab('current'))
        self.nb.add(self.child1, text='a')

        self.nb.pack()
        self.nb.wait_visibility()
        self.failUnlessEqual(self.nb.tab('@5,5'), self.nb.tab('current'))

        for i in range(5, 100, 5):
            if self.nb.tab('@%d, 5' % i, text=None) == 'a':
                break
        else:
            self.fail("Tab with text 'a' not found")


    def test_add_and_hidden(self):
        self.failUnlessRaises(tkinter.TclError, self.nb.hide, -1)
        self.failUnlessRaises(tkinter.TclError, self.nb.hide, 'hi')
        self.failUnlessRaises(tkinter.TclError, self.nb.hide, None)
        self.failUnlessRaises(tkinter.TclError, self.nb.add, None)
        self.failUnlessRaises(tkinter.TclError, self.nb.add, ttk.Label(),
            unknown='option')

        tabs = self.nb.tabs()
        self.nb.hide(self.child1)
        self.nb.add(self.child1)
        self.failUnlessEqual(self.nb.tabs(), tabs)

        child = ttk.Label()
        self.nb.add(child, text='c')
        tabs = self.nb.tabs()

        curr = self.nb.index('current')
        # verify that the tab gets readded at its previous position
        child2_index = self.nb.index(self.child2)
        self.nb.hide(self.child2)
        self.nb.add(self.child2)
        self.failUnlessEqual(self.nb.tabs(), tabs)
        self.failUnlessEqual(self.nb.index(self.child2), child2_index)
        self.failUnless(str(self.child2) == self.nb.tabs()[child2_index])
        # but the tab next to it (not hidden) is the one selected now
        self.failUnlessEqual(self.nb.index('current'), curr + 1)


    def test_forget(self):
        self.failUnlessRaises(tkinter.TclError, self.nb.forget, -1)
        self.failUnlessRaises(tkinter.TclError, self.nb.forget, 'hi')
        self.failUnlessRaises(tkinter.TclError, self.nb.forget, None)

        tabs = self.nb.tabs()
        child1_index = self.nb.index(self.child1)
        self.nb.forget(self.child1)
        self.failIf(str(self.child1) in self.nb.tabs())
        self.failUnlessEqual(len(tabs) - 1, len(self.nb.tabs()))

        self.nb.add(self.child1)
        self.failUnlessEqual(self.nb.index(self.child1), 1)
        self.failIf(child1_index == self.nb.index(self.child1))


    def test_index(self):
        self.failUnlessRaises(tkinter.TclError, self.nb.index, -1)
        self.failUnlessRaises(tkinter.TclError, self.nb.index, None)

        self.failUnless(isinstance(self.nb.index('end'), int))
        self.failUnlessEqual(self.nb.index(self.child1), 0)
        self.failUnlessEqual(self.nb.index(self.child2), 1)
        self.failUnlessEqual(self.nb.index('end'), 2)


    def test_insert(self):
        # moving tabs
        tabs = self.nb.tabs()
        self.nb.insert(1, tabs[0])
        self.failUnlessEqual(self.nb.tabs(), (tabs[1], tabs[0]))
        self.nb.insert(self.child1, self.child2)
        self.failUnlessEqual(self.nb.tabs(), tabs)
        self.nb.insert('end', self.child1)
        self.failUnlessEqual(self.nb.tabs(), (tabs[1], tabs[0]))
        self.nb.insert('end', 0)
        self.failUnlessEqual(self.nb.tabs(), tabs)
        # bad moves
        self.failUnlessRaises(tkinter.TclError, self.nb.insert, 2, tabs[0])
        self.failUnlessRaises(tkinter.TclError, self.nb.insert, -1, tabs[0])

        # new tab
        child3 = ttk.Label()
        self.nb.insert(1, child3)
        self.failUnlessEqual(self.nb.tabs(), (tabs[0], str(child3), tabs[1]))
        self.nb.forget(child3)
        self.failUnlessEqual(self.nb.tabs(), tabs)
        self.nb.insert(self.child1, child3)
        self.failUnlessEqual(self.nb.tabs(), (str(child3), ) + tabs)
        self.nb.forget(child3)
        self.failUnlessRaises(tkinter.TclError, self.nb.insert, 2, child3)
        self.failUnlessRaises(tkinter.TclError, self.nb.insert, -1, child3)

        # bad inserts
        self.failUnlessRaises(tkinter.TclError, self.nb.insert, 'end', None)
        self.failUnlessRaises(tkinter.TclError, self.nb.insert, None, 0)
        self.failUnlessRaises(tkinter.TclError, self.nb.insert, None, None)


    def test_select(self):
        self.nb.pack()
        self.nb.wait_visibility()

        success = []
        tab_changed = []

        self.child1.bind('<Unmap>', lambda evt: success.append(True))
        self.nb.bind('<<NotebookTabChanged>>',
            lambda evt: tab_changed.append(True))

        self.failUnlessEqual(self.nb.select(), str(self.child1))
        self.nb.select(self.child2)
        self.failUnless(success)
        self.failUnlessEqual(self.nb.select(), str(self.child2))

        self.nb.update()
        self.failUnless(tab_changed)


    def test_tab(self):
        self.failUnlessRaises(tkinter.TclError, self.nb.tab, -1)
        self.failUnlessRaises(tkinter.TclError, self.nb.tab, 'notab')
        self.failUnlessRaises(tkinter.TclError, self.nb.tab, None)

        self.failUnless(isinstance(self.nb.tab(self.child1), dict))
        self.failUnlessEqual(self.nb.tab(self.child1, text=None), 'a')
        # newer form for querying a single option
        self.failUnlessEqual(self.nb.tab(self.child1, 'text'), 'a')
        self.nb.tab(self.child1, text='abc')
        self.failUnlessEqual(self.nb.tab(self.child1, text=None), 'abc')
        self.failUnlessEqual(self.nb.tab(self.child1, 'text'), 'abc')


    def test_tabs(self):
        self.failUnlessEqual(len(self.nb.tabs()), 2)

        self.nb.forget(self.child1)
        self.nb.forget(self.child2)

        self.failUnlessEqual(self.nb.tabs(), ())


    def test_traversal(self):
        self.nb.pack()
        self.nb.wait_visibility()

        self.nb.select(0)

        support.simulate_mouse_click(self.nb, 5, 5)
        self.nb.event_generate('<Control-Tab>')
        self.failUnlessEqual(self.nb.select(), str(self.child2))
        self.nb.event_generate('<Shift-Control-Tab>')
        self.failUnlessEqual(self.nb.select(), str(self.child1))
        self.nb.event_generate('<Shift-Control-Tab>')
        self.failUnlessEqual(self.nb.select(), str(self.child2))

        self.nb.tab(self.child1, text='a', underline=0)
        self.nb.enable_traversal()
        self.nb.event_generate('<Alt-a>')
        self.failUnlessEqual(self.nb.select(), str(self.child1))


class TreeviewTest(unittest.TestCase):

    def setUp(self):
        support.root_deiconify()
        self.tv = ttk.Treeview()

    def tearDown(self):
        self.tv.destroy()
        support.root_withdraw()


    def test_bbox(self):
        self.tv.pack()
        self.failUnlessEqual(self.tv.bbox(''), '')
        self.tv.wait_visibility()
        self.tv.update()

        item_id = self.tv.insert('', 'end')
        children = self.tv.get_children()
        self.failUnless(children)

        bbox = self.tv.bbox(children[0])
        self.failUnlessEqual(len(bbox), 4)
        self.failUnless(isinstance(bbox, tuple))
        for item in bbox:
            if not isinstance(item, int):
                self.fail("Invalid bounding box: %s" % bbox)
                break

        # compare width in bboxes
        self.tv['columns'] = ['test']
        self.tv.column('test', width=50)
        bbox_column0 = self.tv.bbox(children[0], 0)
        root_width = self.tv.column('#0', width=None)
        self.failUnlessEqual(bbox_column0[0], bbox[0] + root_width)

        # verify that bbox of a closed item is the empty string
        child1 = self.tv.insert(item_id, 'end')
        self.failUnlessEqual(self.tv.bbox(child1), '')


    def test_children(self):
        # no children yet, should get an empty tuple
        self.failUnlessEqual(self.tv.get_children(), ())

        item_id = self.tv.insert('', 'end')
        self.failUnless(isinstance(self.tv.get_children(), tuple))
        self.failUnlessEqual(self.tv.get_children()[0], item_id)

        # add item_id and child3 as children of child2
        child2 = self.tv.insert('', 'end')
        child3 = self.tv.insert('', 'end')
        self.tv.set_children(child2, item_id, child3)
        self.failUnlessEqual(self.tv.get_children(child2), (item_id, child3))

        # child3 has child2 as parent, thus trying to set child2 as a children
        # of child3 should result in an error
        self.failUnlessRaises(tkinter.TclError,
            self.tv.set_children, child3, child2)

        # remove child2 children
        self.tv.set_children(child2)
        self.failUnlessEqual(self.tv.get_children(child2), ())

        # remove root's children
        self.tv.set_children('')
        self.failUnlessEqual(self.tv.get_children(), ())


    def test_column(self):
        # return a dict with all options/values
        self.failUnless(isinstance(self.tv.column('#0'), dict))
        # return a single value of the given option
        self.failUnless(isinstance(self.tv.column('#0', width=None), int))
        # set a new value for an option
        self.tv.column('#0', width=10)
        # testing new way to get option value
        self.failUnlessEqual(self.tv.column('#0', 'width'), 10)
        self.failUnlessEqual(self.tv.column('#0', width=None), 10)
        # check read-only option
        self.failUnlessRaises(tkinter.TclError, self.tv.column, '#0', id='X')

        self.failUnlessRaises(tkinter.TclError, self.tv.column, 'invalid')
        invalid_kws = [
            {'unknown_option': 'some value'},  {'stretch': 'wrong'},
            {'anchor': 'wrong'}, {'width': 'wrong'}, {'minwidth': 'wrong'}
        ]
        for kw in invalid_kws:
            self.failUnlessRaises(tkinter.TclError, self.tv.column, '#0',
                **kw)


    def test_delete(self):
        self.failUnlessRaises(tkinter.TclError, self.tv.delete, '#0')

        item_id = self.tv.insert('', 'end')
        item2 = self.tv.insert(item_id, 'end')
        self.failUnlessEqual(self.tv.get_children(), (item_id, ))
        self.failUnlessEqual(self.tv.get_children(item_id), (item2, ))

        self.tv.delete(item_id)
        self.failIf(self.tv.get_children())

        # reattach should fail
        self.failUnlessRaises(tkinter.TclError,
            self.tv.reattach, item_id, '', 'end')

        # test multiple item delete
        item1 = self.tv.insert('', 'end')
        item2 = self.tv.insert('', 'end')
        self.failUnlessEqual(self.tv.get_children(), (item1, item2))

        self.tv.delete(item1, item2)
        self.failIf(self.tv.get_children())


    def test_detach_reattach(self):
        item_id = self.tv.insert('', 'end')
        item2 = self.tv.insert(item_id, 'end')

        # calling detach without items is valid, although it does nothing
        prev = self.tv.get_children()
        self.tv.detach() # this should do nothing
        self.failUnlessEqual(prev, self.tv.get_children())

        self.failUnlessEqual(self.tv.get_children(), (item_id, ))
        self.failUnlessEqual(self.tv.get_children(item_id), (item2, ))

        # detach item with children
        self.tv.detach(item_id)
        self.failIf(self.tv.get_children())

        # reattach item with children
        self.tv.reattach(item_id, '', 'end')
        self.failUnlessEqual(self.tv.get_children(), (item_id, ))
        self.failUnlessEqual(self.tv.get_children(item_id), (item2, ))

        # move a children to the root
        self.tv.move(item2, '', 'end')
        self.failUnlessEqual(self.tv.get_children(), (item_id, item2))
        self.failUnlessEqual(self.tv.get_children(item_id), ())

        # bad values
        self.failUnlessRaises(tkinter.TclError,
            self.tv.reattach, 'nonexistent', '', 'end')
        self.failUnlessRaises(tkinter.TclError,
            self.tv.detach, 'nonexistent')
        self.failUnlessRaises(tkinter.TclError,
            self.tv.reattach, item2, 'otherparent', 'end')
        self.failUnlessRaises(tkinter.TclError,
            self.tv.reattach, item2, '', 'invalid')

        # multiple detach
        self.tv.detach(item_id, item2)
        self.failUnlessEqual(self.tv.get_children(), ())
        self.failUnlessEqual(self.tv.get_children(item_id), ())


    def test_exists(self):
        self.failUnlessEqual(self.tv.exists('something'), False)
        self.failUnlessEqual(self.tv.exists(''), True)
        self.failUnlessEqual(self.tv.exists({}), False)

        # the following will make a tk.call equivalent to
        # tk.call(treeview, "exists") which should result in an error
        # in the tcl interpreter since tk requires an item.
        self.failUnlessRaises(tkinter.TclError, self.tv.exists, None)


    def test_focus(self):
        # nothing is focused right now
        self.failUnlessEqual(self.tv.focus(), '')

        item1 = self.tv.insert('', 'end')
        self.tv.focus(item1)
        self.failUnlessEqual(self.tv.focus(), item1)

        self.tv.delete(item1)
        self.failUnlessEqual(self.tv.focus(), '')

        # try focusing inexistent item
        self.failUnlessRaises(tkinter.TclError, self.tv.focus, 'hi')


    def test_heading(self):
        # check a dict is returned
        self.failUnless(isinstance(self.tv.heading('#0'), dict))

        # check a value is returned
        self.tv.heading('#0', text='hi')
        self.failUnlessEqual(self.tv.heading('#0', 'text'), 'hi')
        self.failUnlessEqual(self.tv.heading('#0', text=None), 'hi')

        # invalid option
        self.failUnlessRaises(tkinter.TclError, self.tv.heading, '#0',
            background=None)
        # invalid value
        self.failUnlessRaises(tkinter.TclError, self.tv.heading, '#0',
            anchor=1)


    def test_heading_callback(self):
        def simulate_heading_click(x, y):
            support.simulate_mouse_click(self.tv, x, y)
            self.tv.update_idletasks()

        success = [] # no success for now

        self.tv.pack()
        self.tv.wait_visibility()
        self.tv.heading('#0', command=lambda: success.append(True))
        self.tv.column('#0', width=100)
        self.tv.update()

        # assuming that the coords (5, 5) fall into heading #0
        simulate_heading_click(5, 5)
        if not success:
            self.fail("The command associated to the treeview heading wasn't "
                "invoked.")

        success = []
        commands = self.tv.master._tclCommands
        self.tv.heading('#0', command=str(self.tv.heading('#0', command=None)))
        self.failUnlessEqual(commands, self.tv.master._tclCommands)
        simulate_heading_click(5, 5)
        if not success:
            self.fail("The command associated to the treeview heading wasn't "
                "invoked.")

        # XXX The following raises an error in a tcl interpreter, but not in
        # Python
        #self.tv.heading('#0', command='I dont exist')
        #simulate_heading_click(5, 5)


    def test_index(self):
        # item 'what' doesn't exist
        self.failUnlessRaises(tkinter.TclError, self.tv.index, 'what')

        self.failUnlessEqual(self.tv.index(''), 0)

        item1 = self.tv.insert('', 'end')
        item2 = self.tv.insert('', 'end')
        c1 = self.tv.insert(item1, 'end')
        c2 = self.tv.insert(item1, 'end')
        self.failUnlessEqual(self.tv.index(item1), 0)
        self.failUnlessEqual(self.tv.index(c1), 0)
        self.failUnlessEqual(self.tv.index(c2), 1)
        self.failUnlessEqual(self.tv.index(item2), 1)

        self.tv.move(item2, '', 0)
        self.failUnlessEqual(self.tv.index(item2), 0)
        self.failUnlessEqual(self.tv.index(item1), 1)

        # check that index still works even after its parent and siblings
        # have been detached
        self.tv.detach(item1)
        self.failUnlessEqual(self.tv.index(c2), 1)
        self.tv.detach(c1)
        self.failUnlessEqual(self.tv.index(c2), 0)

        # but it fails after item has been deleted
        self.tv.delete(item1)
        self.failUnlessRaises(tkinter.TclError, self.tv.index, c2)


    def test_insert_item(self):
        # parent 'none' doesn't exist
        self.failUnlessRaises(tkinter.TclError, self.tv.insert, 'none', 'end')

        # open values
        self.failUnlessRaises(tkinter.TclError, self.tv.insert, '', 'end',
            open='')
        self.failUnlessRaises(tkinter.TclError, self.tv.insert, '', 'end',
            open='please')
        self.failIf(self.tv.delete(self.tv.insert('', 'end', open=True)))
        self.failIf(self.tv.delete(self.tv.insert('', 'end', open=False)))

        # invalid index
        self.failUnlessRaises(tkinter.TclError, self.tv.insert, '', 'middle')

        # trying to duplicate item id is invalid
        itemid = self.tv.insert('', 'end', 'first-item')
        self.failUnlessEqual(itemid, 'first-item')
        self.failUnlessRaises(tkinter.TclError, self.tv.insert, '', 'end',
            'first-item')
        self.failUnlessRaises(tkinter.TclError, self.tv.insert, '', 'end',
            MockTclObj('first-item'))

        # unicode values
        value = '\xe1ba'
        item = self.tv.insert('', 'end', values=(value, ))
        self.failUnlessEqual(self.tv.item(item, 'values'), (value, ))
        self.failUnlessEqual(self.tv.item(item, values=None), (value, ))

        self.tv.item(item, values=list(self.tv.item(item, values=None)))
        self.failUnlessEqual(self.tv.item(item, values=None), (value, ))

        self.failUnless(isinstance(self.tv.item(item), dict))

        # erase item values
        self.tv.item(item, values='')
        self.failIf(self.tv.item(item, values=None))

        # item tags
        item = self.tv.insert('', 'end', tags=[1, 2, value])
        self.failUnlessEqual(self.tv.item(item, tags=None), ('1', '2', value))
        self.tv.item(item, tags=[])
        self.failIf(self.tv.item(item, tags=None))
        self.tv.item(item, tags=(1, 2))
        self.failUnlessEqual(self.tv.item(item, tags=None), ('1', '2'))

        # values with spaces
        item = self.tv.insert('', 'end', values=('a b c',
            '%s %s' % (value, value)))
        self.failUnlessEqual(self.tv.item(item, values=None),
            ('a b c', '%s %s' % (value, value)))

        # text
        self.failUnlessEqual(self.tv.item(
            self.tv.insert('', 'end', text="Label here"), text=None),
            "Label here")
        self.failUnlessEqual(self.tv.item(
            self.tv.insert('', 'end', text=value), text=None),
            value)


    def test_set(self):
        self.tv['columns'] = ['A', 'B']
        item = self.tv.insert('', 'end', values=['a', 'b'])
        self.failUnlessEqual(self.tv.set(item), {'A': 'a', 'B': 'b'})

        self.tv.set(item, 'B', 'a')
        self.failUnlessEqual(self.tv.item(item, values=None), ('a', 'a'))

        self.tv['columns'] = ['B']
        self.failUnlessEqual(self.tv.set(item), {'B': 'a'})

        self.tv.set(item, 'B', 'b')
        self.failUnlessEqual(self.tv.set(item, column='B'), 'b')
        self.failUnlessEqual(self.tv.item(item, values=None), ('b', 'a'))

        self.tv.set(item, 'B', 123)
        self.failUnlessEqual(self.tv.set(item, 'B'), 123)
        self.failUnlessEqual(self.tv.item(item, values=None), (123, 'a'))
        self.failUnlessEqual(self.tv.set(item), {'B': 123})

        # inexistent column
        self.failUnlessRaises(tkinter.TclError, self.tv.set, item, 'A')
        self.failUnlessRaises(tkinter.TclError, self.tv.set, item, 'A', 'b')

        # inexistent item
        self.failUnlessRaises(tkinter.TclError, self.tv.set, 'notme')


    def test_tag_bind(self):
        events = []
        item1 = self.tv.insert('', 'end', tags=['call'])
        item2 = self.tv.insert('', 'end', tags=['call'])
        self.tv.tag_bind('call', '<ButtonPress-1>',
            lambda evt: events.append(1))
        self.tv.tag_bind('call', '<ButtonRelease-1>',
            lambda evt: events.append(2))

        self.tv.pack()
        self.tv.wait_visibility()
        self.tv.update()

        pos_y = set()
        found = set()
        for i in range(0, 100, 10):
            if len(found) == 2: # item1 and item2 already found
                break
            item_id = self.tv.identify_row(i)
            if item_id and item_id not in found:
                pos_y.add(i)
                found.add(item_id)

        self.failUnlessEqual(len(pos_y), 2) # item1 and item2 y pos
        for y in pos_y:
            support.simulate_mouse_click(self.tv, 0, y)

        # by now there should be 4 things in the events list, since each
        # item had a bind for two events that were simulated above
        self.failUnlessEqual(len(events), 4)
        for evt in zip(events[::2], events[1::2]):
            self.failUnlessEqual(evt, (1, 2))


    def test_tag_configure(self):
        # Just testing parameter passing for now
        self.failUnlessRaises(TypeError, self.tv.tag_configure)
        self.failUnlessRaises(tkinter.TclError, self.tv.tag_configure,
            'test', sky='blue')
        self.tv.tag_configure('test', foreground='blue')
        self.failUnlessEqual(self.tv.tag_configure('test', 'foreground'),
            'blue')
        self.failUnlessEqual(self.tv.tag_configure('test', foreground=None),
            'blue')
        self.failUnless(isinstance(self.tv.tag_configure('test'), dict))


tests_gui = (
        WidgetTest, ButtonTest, CheckbuttonTest, RadiobuttonTest,
        ComboboxTest, EntryTest, PanedwindowTest, ScaleTest, NotebookTest,
        TreeviewTest
        )

if __name__ == "__main__":
    run_unittest(*tests_gui)
