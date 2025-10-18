import sys
import unittest
import tkinter
from tkinter import ttk
from test.support import requires, gc_collect
from test.test_tkinter.support import AbstractTkTest, AbstractDefaultRootTest

requires('gui')

class LabeledScaleTest(AbstractTkTest, unittest.TestCase):

    def tearDown(self):
        self.root.update_idletasks()
        super().tearDown()

    def test_widget_destroy(self):
        # automatically created variable
        x = ttk.LabeledScale(self.root)
        var = x._variable._name
        x.destroy()
        gc_collect()  # For PyPy or other GCs.
        self.assertRaises(tkinter.TclError, x.tk.globalgetvar, var)

        # manually created variable
        myvar = tkinter.DoubleVar(self.root)
        name = myvar._name
        x = ttk.LabeledScale(self.root, variable=myvar)
        x.destroy()
        if self.wantobjects:
            self.assertEqual(x.tk.globalgetvar(name), myvar.get())
        else:
            self.assertEqual(float(x.tk.globalgetvar(name)), myvar.get())
        del myvar
        gc_collect()  # For PyPy or other GCs.
        self.assertRaises(tkinter.TclError, x.tk.globalgetvar, name)

        # checking that the tracing callback is properly removed
        myvar = tkinter.IntVar(self.root)
        # LabeledScale will start tracing myvar
        x = ttk.LabeledScale(self.root, variable=myvar)
        x.destroy()
        # Unless the tracing callback was removed, creating a new
        # LabeledScale with the same var will cause an error now. This
        # happens because the variable will be set to (possibly) a new
        # value which causes the tracing callback to be called and then
        # it tries calling instance attributes not yet defined.
        ttk.LabeledScale(self.root, variable=myvar)
        if hasattr(sys, 'last_exc'):
            self.assertNotEqual(type(sys.last_exc), tkinter.TclError)
        elif hasattr(sys, 'last_type'):
            self.assertNotEqual(sys.last_type, tkinter.TclError)

    def test_initialization(self):
        # master passing
        master = tkinter.Frame(self.root)
        x = ttk.LabeledScale(master)
        self.assertEqual(x.master, master)
        x.destroy()

        # variable initialization/passing
        passed_expected = (('0', 0), (0, 0), (10, 10),
            (-1, -1), (sys.maxsize + 1, sys.maxsize + 1),
            (2.5, 2), ('2.5', 2))
        for pair in passed_expected:
            x = ttk.LabeledScale(self.root, from_=pair[0])
            self.assertEqual(x.value, pair[1])
            x.destroy()
        x = ttk.LabeledScale(self.root, from_=None)
        self.assertRaises((ValueError, tkinter.TclError), x._variable.get)
        x.destroy()
        # variable should have its default value set to the from_ value
        myvar = tkinter.DoubleVar(self.root, value=20)
        x = ttk.LabeledScale(self.root, variable=myvar)
        self.assertEqual(x.value, 0)
        x.destroy()
        # check that it is really using a DoubleVar
        x = ttk.LabeledScale(self.root, variable=myvar, from_=0.5)
        self.assertEqual(x.value, 0.5)
        self.assertEqual(x._variable._name, myvar._name)
        x.destroy()

        # widget positionment
        def check_positions(scale, scale_pos, label, label_pos):
            self.assertEqual(scale.pack_info()['side'], scale_pos)
            self.assertEqual(label.place_info()['anchor'], label_pos)
        x = ttk.LabeledScale(self.root, compound='top')
        check_positions(x.scale, 'bottom', x.label, 'n')
        x.destroy()
        x = ttk.LabeledScale(self.root, compound='bottom')
        check_positions(x.scale, 'top', x.label, 's')
        x.destroy()
        # invert default positions
        x = ttk.LabeledScale(self.root, compound='unknown')
        check_positions(x.scale, 'top', x.label, 's')
        x.destroy()
        x = ttk.LabeledScale(self.root) # take default positions
        check_positions(x.scale, 'bottom', x.label, 'n')
        x.destroy()

        # extra, and invalid, kwargs
        self.assertRaises(tkinter.TclError, ttk.LabeledScale, master, a='b')


    def test_horizontal_range(self):
        lscale = ttk.LabeledScale(self.root, from_=0, to=10)
        lscale.pack()
        lscale.update()

        linfo_1 = lscale.label.place_info()
        prev_xcoord = lscale.scale.coords()[0]
        self.assertEqual(prev_xcoord, int(linfo_1['x']))
        # change range to: from -5 to 5. This should change the x coord of
        # the scale widget, since 0 is at the middle of the new
        # range.
        lscale.scale.configure(from_=-5, to=5)
        # The following update is needed since the test doesn't use mainloop,
        # at the same time this shouldn't affect test outcome
        lscale.update()
        curr_xcoord = lscale.scale.coords()[0]
        self.assertNotEqual(prev_xcoord, curr_xcoord)
        # the label widget should have been repositioned too
        linfo_2 = lscale.label.place_info()
        self.assertEqual(lscale.label['text'], 0 if self.wantobjects else '0')
        self.assertEqual(curr_xcoord, int(linfo_2['x']))
        # change the range back
        lscale.scale.configure(from_=0, to=10)
        self.assertNotEqual(prev_xcoord, curr_xcoord)
        self.assertEqual(prev_xcoord, int(linfo_1['x']))

        lscale.destroy()


    def test_variable_change(self):
        x = ttk.LabeledScale(self.root)
        x.pack()
        x.update()

        curr_xcoord = x.scale.coords()[0]
        newval = x.value + 1
        x.value = newval
        # The following update is needed since the test doesn't use mainloop,
        # at the same time this shouldn't affect test outcome
        x.update()
        self.assertEqual(x.value, newval)
        self.assertEqual(x.label['text'],
                         newval if self.wantobjects else str(newval))
        self.assertEqual(float(x.scale.get()), newval)
        self.assertGreater(x.scale.coords()[0], curr_xcoord)
        self.assertEqual(x.scale.coords()[0],
            int(x.label.place_info()['x']))

        # value outside range
        if self.wantobjects:
            conv = lambda x: x
        else:
            conv = int
        x.value = conv(x.scale['to']) + 1 # no changes shouldn't happen
        x.update()
        self.assertEqual(x.value, newval)
        self.assertEqual(conv(x.label['text']), newval)
        self.assertEqual(float(x.scale.get()), newval)
        self.assertEqual(x.scale.coords()[0],
            int(x.label.place_info()['x']))

        # non-integer value
        x.value = newval = newval + 1.5
        x.update()
        self.assertEqual(x.value, int(newval))
        self.assertEqual(conv(x.label['text']), int(newval))
        self.assertEqual(float(x.scale.get()), newval)

        x.destroy()


    def test_resize(self):
        x = ttk.LabeledScale(self.root)
        x.pack(expand=True, fill='both')
        gc_collect()  # For PyPy or other GCs.
        x.update()

        width, height = x.master.winfo_width(), x.master.winfo_height()
        width_new, height_new = width * 2, height * 2

        x.value = 3
        x.update()
        x.master.wm_geometry("%dx%d" % (width_new, height_new))
        self.assertEqual(int(x.label.place_info()['x']),
            x.scale.coords()[0])

        # Reset geometry
        x.master.wm_geometry("%dx%d" % (width, height))
        x.destroy()


class OptionMenuTest(AbstractTkTest, unittest.TestCase):

    def setUp(self):
        super().setUp()
        self.textvar = tkinter.StringVar(self.root)

    def tearDown(self):
        del self.textvar
        super().tearDown()


    def test_widget_destroy(self):
        var = tkinter.StringVar(self.root)
        optmenu = ttk.OptionMenu(self.root, var)
        name = var._name
        optmenu.update_idletasks()
        optmenu.destroy()
        self.assertEqual(optmenu.tk.globalgetvar(name), var.get())
        del var
        gc_collect()  # For PyPy or other GCs.
        self.assertRaises(tkinter.TclError, optmenu.tk.globalgetvar, name)


    def test_initialization(self):
        self.assertRaises(tkinter.TclError,
            ttk.OptionMenu, self.root, self.textvar, invalid='thing')

        optmenu = ttk.OptionMenu(self.root, self.textvar, 'b', 'a', 'b')
        self.assertEqual(optmenu._variable.get(), 'b')

        self.assertTrue(optmenu['menu'])
        self.assertTrue(optmenu['textvariable'])

        optmenu.destroy()


    def test_menu(self):
        items = ('a', 'b', 'c')
        default = 'a'
        optmenu = ttk.OptionMenu(self.root, self.textvar, default, *items)
        found_default = False
        for i in range(len(items)):
            value = optmenu['menu'].entrycget(i, 'value')
            self.assertEqual(value, items[i])
            if value == default:
                found_default = True
        self.assertTrue(found_default)
        optmenu.destroy()

        # default shouldn't be in menu if it is not part of values
        default = 'd'
        optmenu = ttk.OptionMenu(self.root, self.textvar, default, *items)
        curr = None
        i = 0
        while True:
            last, curr = curr, optmenu['menu'].entryconfigure(i, 'value')
            if last == curr:
                # no more menu entries
                break
            self.assertNotEqual(curr, default)
            i += 1
        self.assertEqual(i, len(items))

        # check that variable is updated correctly
        optmenu.pack()
        gc_collect()  # For PyPy or other GCs.
        optmenu['menu'].invoke(0)
        self.assertEqual(optmenu._variable.get(), items[0])

        # changing to an invalid index shouldn't change the variable
        self.assertRaises(tkinter.TclError, optmenu['menu'].invoke, -1)
        self.assertEqual(optmenu._variable.get(), items[0])

        optmenu.destroy()

        # specifying a callback
        success = []
        def cb_test(item):
            self.assertEqual(item, items[1])
            success.append(True)
        optmenu = ttk.OptionMenu(self.root, self.textvar, 'a', command=cb_test,
            *items)
        optmenu['menu'].invoke(1)
        if not success:
            self.fail("Menu callback not invoked")

        optmenu.destroy()

    def test_unique_radiobuttons(self):
        # check that radiobuttons are unique across instances (bpo25684)
        items = ('a', 'b', 'c')
        default = 'a'
        optmenu = ttk.OptionMenu(self.root, self.textvar, default, *items)
        textvar2 = tkinter.StringVar(self.root)
        optmenu2 = ttk.OptionMenu(self.root, textvar2, default, *items)
        optmenu.pack()
        optmenu2.pack()
        optmenu['menu'].invoke(1)
        optmenu2['menu'].invoke(2)
        optmenu_stringvar_name = optmenu['menu'].entrycget(0, 'variable')
        optmenu2_stringvar_name = optmenu2['menu'].entrycget(0, 'variable')
        self.assertNotEqual(optmenu_stringvar_name,
                            optmenu2_stringvar_name)
        self.assertEqual(self.root.tk.globalgetvar(optmenu_stringvar_name),
                         items[1])
        self.assertEqual(self.root.tk.globalgetvar(optmenu2_stringvar_name),
                         items[2])

        optmenu.destroy()
        optmenu2.destroy()

    def test_trace_variable(self):
        # prior to bpo45160, tracing a variable would cause the callback to be made twice
        success = []
        items = ('a', 'b', 'c')
        textvar = tkinter.StringVar(self.root)
        def cb_test(*args):
            success.append(textvar.get())
        optmenu = ttk.OptionMenu(self.root, textvar, "a", *items)
        optmenu.pack()
        cb_name = textvar.trace_add("write", cb_test)
        optmenu['menu'].invoke(1)
        self.assertEqual(success, ['b'])
        self.assertEqual(textvar.get(), 'b')
        textvar.trace_remove("write", cb_name)
        optmenu.destroy()

    def test_specify_name(self):
        textvar = tkinter.StringVar(self.root)
        widget = ttk.OptionMenu(self.root, textvar, ":)", name="option_menu_ex")
        self.assertEqual(str(widget), ".option_menu_ex")
        self.assertIs(self.root.children["option_menu_ex"], widget)


class DefaultRootTest(AbstractDefaultRootTest, unittest.TestCase):

    def test_labeledscale(self):
        self._test_widget(ttk.LabeledScale)


if __name__ == "__main__":
    unittest.main()
