import sys
import unittest
import Tkinter
import ttk
from test.test_support import requires, run_unittest

import support

requires('gui')

class LabeledScaleTest(unittest.TestCase):

    def setUp(self):
        support.root_deiconify()

    def tearDown(self):
        support.root_withdraw()


    def test_widget_destroy(self):
        # automatically created variable
        x = ttk.LabeledScale()
        var = x._variable._name
        x.destroy()
        self.assertRaises(Tkinter.TclError, x.tk.globalgetvar, var)

        # manually created variable
        myvar = Tkinter.DoubleVar()
        name = myvar._name
        x = ttk.LabeledScale(variable=myvar)
        x.destroy()
        if x.tk.wantobjects():
            self.assertEqual(x.tk.globalgetvar(name), myvar.get())
        else:
            self.assertEqual(float(x.tk.globalgetvar(name)), myvar.get())
        del myvar
        self.assertRaises(Tkinter.TclError, x.tk.globalgetvar, name)

        # checking that the tracing callback is properly removed
        myvar = Tkinter.IntVar()
        # LabeledScale will start tracing myvar
        x = ttk.LabeledScale(variable=myvar)
        x.destroy()
        # Unless the tracing callback was removed, creating a new
        # LabeledScale with the same var will cause an error now. This
        # happens because the variable will be set to (possibly) a new
        # value which causes the tracing callback to be called and then
        # it tries calling instance attributes not yet defined.
        ttk.LabeledScale(variable=myvar)
        if hasattr(sys, 'last_type'):
            self.assertNotEqual(sys.last_type, Tkinter.TclError)


    def test_initialization(self):
        # master passing
        x = ttk.LabeledScale()
        self.assertEqual(x.master, Tkinter._default_root)
        x.destroy()
        master = Tkinter.Frame()
        x = ttk.LabeledScale(master)
        self.assertEqual(x.master, master)
        x.destroy()

        # variable initialization/passing
        passed_expected = (('0', 0), (0, 0), (10, 10),
            (-1, -1), (sys.maxint + 1, sys.maxint + 1))
        if x.tk.wantobjects():
            passed_expected += ((2.5, 2),)
        for pair in passed_expected:
            x = ttk.LabeledScale(from_=pair[0])
            self.assertEqual(x.value, pair[1])
            x.destroy()
        x = ttk.LabeledScale(from_='2.5')
        self.assertRaises(ValueError, x._variable.get)
        x.destroy()
        x = ttk.LabeledScale(from_=None)
        self.assertRaises(ValueError, x._variable.get)
        x.destroy()
        # variable should have its default value set to the from_ value
        myvar = Tkinter.DoubleVar(value=20)
        x = ttk.LabeledScale(variable=myvar)
        self.assertEqual(x.value, 0)
        x.destroy()
        # check that it is really using a DoubleVar
        x = ttk.LabeledScale(variable=myvar, from_=0.5)
        self.assertEqual(x.value, 0.5)
        self.assertEqual(x._variable._name, myvar._name)
        x.destroy()

        # widget positionment
        def check_positions(scale, scale_pos, label, label_pos):
            self.assertEqual(scale.pack_info()['side'], scale_pos)
            self.assertEqual(label.place_info()['anchor'], label_pos)
        x = ttk.LabeledScale(compound='top')
        check_positions(x.scale, 'bottom', x.label, 'n')
        x.destroy()
        x = ttk.LabeledScale(compound='bottom')
        check_positions(x.scale, 'top', x.label, 's')
        x.destroy()
        x = ttk.LabeledScale(compound='unknown') # invert default positions
        check_positions(x.scale, 'top', x.label, 's')
        x.destroy()
        x = ttk.LabeledScale() # take default positions
        check_positions(x.scale, 'bottom', x.label, 'n')
        x.destroy()

        # extra, and invalid, kwargs
        self.assertRaises(Tkinter.TclError, ttk.LabeledScale, a='b')


    def test_horizontal_range(self):
        lscale = ttk.LabeledScale(from_=0, to=10)
        lscale.pack()
        lscale.wait_visibility()
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
        self.assertEqual(lscale.label['text'], 0 if lscale.tk.wantobjects() else '0')
        self.assertEqual(curr_xcoord, int(linfo_2['x']))
        # change the range back
        lscale.scale.configure(from_=0, to=10)
        self.assertNotEqual(prev_xcoord, curr_xcoord)
        self.assertEqual(prev_xcoord, int(linfo_1['x']))

        lscale.destroy()


    def test_variable_change(self):
        x = ttk.LabeledScale()
        x.pack()
        x.wait_visibility()
        x.update()

        curr_xcoord = x.scale.coords()[0]
        newval = x.value + 1
        x.value = newval
        # The following update is needed since the test doesn't use mainloop,
        # at the same time this shouldn't affect test outcome
        x.update()
        self.assertEqual(x.label['text'],
                         newval if x.tk.wantobjects() else str(newval))
        self.assertGreater(x.scale.coords()[0], curr_xcoord)
        self.assertEqual(x.scale.coords()[0],
            int(x.label.place_info()['x']))

        # value outside range
        if x.tk.wantobjects():
            conv = lambda x: x
        else:
            conv = int
        x.value = conv(x.scale['to']) + 1 # no changes shouldn't happen
        x.update()
        self.assertEqual(conv(x.label['text']), newval)
        self.assertEqual(x.scale.coords()[0],
            int(x.label.place_info()['x']))

        x.destroy()


    def test_resize(self):
        x = ttk.LabeledScale()
        x.pack(expand=True, fill='both')
        x.wait_visibility()
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


class OptionMenuTest(unittest.TestCase):

    def setUp(self):
        support.root_deiconify()
        self.textvar = Tkinter.StringVar()

    def tearDown(self):
        del self.textvar
        support.root_withdraw()


    def test_widget_destroy(self):
        var = Tkinter.StringVar()
        optmenu = ttk.OptionMenu(None, var)
        name = var._name
        optmenu.update_idletasks()
        optmenu.destroy()
        self.assertEqual(optmenu.tk.globalgetvar(name), var.get())
        del var
        self.assertRaises(Tkinter.TclError, optmenu.tk.globalgetvar, name)


    def test_initialization(self):
        self.assertRaises(Tkinter.TclError,
            ttk.OptionMenu, None, self.textvar, invalid='thing')

        optmenu = ttk.OptionMenu(None, self.textvar, 'b', 'a', 'b')
        self.assertEqual(optmenu._variable.get(), 'b')

        self.assertTrue(optmenu['menu'])
        self.assertTrue(optmenu['textvariable'])

        optmenu.destroy()


    def test_menu(self):
        items = ('a', 'b', 'c')
        default = 'a'
        optmenu = ttk.OptionMenu(None, self.textvar, default, *items)
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
        optmenu = ttk.OptionMenu(None, self.textvar, default, *items)
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
        optmenu.wait_visibility()
        optmenu['menu'].invoke(0)
        self.assertEqual(optmenu._variable.get(), items[0])

        # changing to an invalid index shouldn't change the variable
        self.assertRaises(Tkinter.TclError, optmenu['menu'].invoke, -1)
        self.assertEqual(optmenu._variable.get(), items[0])

        optmenu.destroy()

        # specifying a callback
        success = []
        def cb_test(item):
            self.assertEqual(item, items[1])
            success.append(True)
        optmenu = ttk.OptionMenu(None, self.textvar, 'a', command=cb_test,
            *items)
        optmenu['menu'].invoke(1)
        if not success:
            self.fail("Menu callback not invoked")

        optmenu.destroy()


tests_gui = (LabeledScaleTest, OptionMenuTest)

if __name__ == "__main__":
    run_unittest(*tests_gui)
