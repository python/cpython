import functools
import unittest
import tkinter
import enum
from test import support
from tkinter.test.support import AbstractTkTest, AbstractDefaultRootTest

support.requires('gui')

class MiscTest(AbstractTkTest, unittest.TestCase):

    def test_all(self):
        self.assertIn("Widget", tkinter.__all__)
        # Check that variables from tkinter.constants are also in tkinter.__all__
        self.assertIn("CASCADE", tkinter.__all__)
        self.assertIsNotNone(tkinter.CASCADE)
        # Check that sys, re, and constants are not in tkinter.__all__
        self.assertNotIn("re", tkinter.__all__)
        self.assertNotIn("sys", tkinter.__all__)
        self.assertNotIn("constants", tkinter.__all__)
        # Check that an underscored functions is not in tkinter.__all__
        self.assertNotIn("_tkerror", tkinter.__all__)
        # Check that wantobjects is not in tkinter.__all__
        self.assertNotIn("wantobjects", tkinter.__all__)

    def test_repr(self):
        t = tkinter.Toplevel(self.root, name='top')
        f = tkinter.Frame(t, name='child')
        self.assertEqual(repr(f), '<tkinter.Frame object .top.child>')

    def test_generated_names(self):
        t = tkinter.Toplevel(self.root)
        f = tkinter.Frame(t)
        f2 = tkinter.Frame(t)
        b = tkinter.Button(f2)
        for name in str(b).split('.'):
            self.assertFalse(name.isidentifier(), msg=repr(name))

    def test_tk_setPalette(self):
        root = self.root
        root.tk_setPalette('black')
        self.assertEqual(root['background'], 'black')
        root.tk_setPalette('white')
        self.assertEqual(root['background'], 'white')
        self.assertRaisesRegex(tkinter.TclError,
                '^unknown color name "spam"$',
                root.tk_setPalette, 'spam')

        root.tk_setPalette(background='black')
        self.assertEqual(root['background'], 'black')
        root.tk_setPalette(background='blue', highlightColor='yellow')
        self.assertEqual(root['background'], 'blue')
        self.assertEqual(root['highlightcolor'], 'yellow')
        root.tk_setPalette(background='yellow', highlightColor='blue')
        self.assertEqual(root['background'], 'yellow')
        self.assertEqual(root['highlightcolor'], 'blue')
        self.assertRaisesRegex(tkinter.TclError,
                '^unknown color name "spam"$',
                root.tk_setPalette, background='spam')
        self.assertRaisesRegex(tkinter.TclError,
                '^must specify a background color$',
                root.tk_setPalette, spam='white')
        self.assertRaisesRegex(tkinter.TclError,
                '^must specify a background color$',
                root.tk_setPalette, highlightColor='blue')

    def test_after(self):
        root = self.root

        def callback(start=0, step=1):
            nonlocal count
            count = start + step

        # Without function, sleeps for ms.
        self.assertIsNone(root.after(1))

        # Set up with callback with no args.
        count = 0
        timer1 = root.after(0, callback)
        self.assertIn(timer1, root.tk.call('after', 'info'))
        (script, _) = root.tk.splitlist(root.tk.call('after', 'info', timer1))
        root.update()  # Process all pending events.
        self.assertEqual(count, 1)
        with self.assertRaises(tkinter.TclError):
            root.tk.call(script)

        # Set up with callback with args.
        count = 0
        timer1 = root.after(0, callback, 42, 11)
        root.update()  # Process all pending events.
        self.assertEqual(count, 53)

        # Cancel before called.
        timer1 = root.after(1000, callback)
        self.assertIn(timer1, root.tk.call('after', 'info'))
        (script, _) = root.tk.splitlist(root.tk.call('after', 'info', timer1))
        root.after_cancel(timer1)  # Cancel this event.
        self.assertEqual(count, 53)
        with self.assertRaises(tkinter.TclError):
            root.tk.call(script)

        # Call with a callable class
        count = 0
        timer1 = root.after(0, functools.partial(callback, 42, 11))
        root.update()  # Process all pending events.
        self.assertEqual(count, 53)

    def test_after_idle(self):
        root = self.root

        def callback(start=0, step=1):
            nonlocal count
            count = start + step

        # Set up with callback with no args.
        count = 0
        idle1 = root.after_idle(callback)
        self.assertIn(idle1, root.tk.call('after', 'info'))
        (script, _) = root.tk.splitlist(root.tk.call('after', 'info', idle1))
        root.update_idletasks()  # Process all pending events.
        self.assertEqual(count, 1)
        with self.assertRaises(tkinter.TclError):
            root.tk.call(script)

        # Set up with callback with args.
        count = 0
        idle1 = root.after_idle(callback, 42, 11)
        root.update_idletasks()  # Process all pending events.
        self.assertEqual(count, 53)

        # Cancel before called.
        idle1 = root.after_idle(callback)
        self.assertIn(idle1, root.tk.call('after', 'info'))
        (script, _) = root.tk.splitlist(root.tk.call('after', 'info', idle1))
        root.after_cancel(idle1)  # Cancel this event.
        self.assertEqual(count, 53)
        with self.assertRaises(tkinter.TclError):
            root.tk.call(script)

    def test_after_cancel(self):
        root = self.root

        def callback():
            nonlocal count
            count += 1

        timer1 = root.after(5000, callback)
        idle1 = root.after_idle(callback)

        # No value for id raises a ValueError.
        with self.assertRaises(ValueError):
            root.after_cancel(None)

        # Cancel timer event.
        count = 0
        (script, _) = root.tk.splitlist(root.tk.call('after', 'info', timer1))
        root.tk.call(script)
        self.assertEqual(count, 1)
        root.after_cancel(timer1)
        with self.assertRaises(tkinter.TclError):
            root.tk.call(script)
        self.assertEqual(count, 1)
        with self.assertRaises(tkinter.TclError):
            root.tk.call('after', 'info', timer1)

        # Cancel same event - nothing happens.
        root.after_cancel(timer1)

        # Cancel idle event.
        count = 0
        (script, _) = root.tk.splitlist(root.tk.call('after', 'info', idle1))
        root.tk.call(script)
        self.assertEqual(count, 1)
        root.after_cancel(idle1)
        with self.assertRaises(tkinter.TclError):
            root.tk.call(script)
        self.assertEqual(count, 1)
        with self.assertRaises(tkinter.TclError):
            root.tk.call('after', 'info', idle1)

    def test_clipboard(self):
        root = self.root
        root.clipboard_clear()
        root.clipboard_append('Ùñî')
        self.assertEqual(root.clipboard_get(), 'Ùñî')
        root.clipboard_append('çōđě')
        self.assertEqual(root.clipboard_get(), 'Ùñîçōđě')
        root.clipboard_clear()
        with self.assertRaises(tkinter.TclError):
            root.clipboard_get()

    def test_clipboard_astral(self):
        root = self.root
        root.clipboard_clear()
        root.clipboard_append('𝔘𝔫𝔦')
        self.assertEqual(root.clipboard_get(), '𝔘𝔫𝔦')
        root.clipboard_append('𝔠𝔬𝔡𝔢')
        self.assertEqual(root.clipboard_get(), '𝔘𝔫𝔦𝔠𝔬𝔡𝔢')
        root.clipboard_clear()
        with self.assertRaises(tkinter.TclError):
            root.clipboard_get()

    def test_winfo_rgb(self):

        def assertApprox(col1, col2):
            # A small amount of flexibility is required (bpo-45496)
            # 33 is ~0.05% of 65535, which is a reasonable margin
            for col1_channel, col2_channel in zip(col1, col2):
                self.assertAlmostEqual(col1_channel, col2_channel, delta=33)

        root = self.root
        rgb = root.winfo_rgb

        # Color name.
        self.assertEqual(rgb('red'), (65535, 0, 0))
        self.assertEqual(rgb('dark slate blue'), (18504, 15677, 35723))
        # #RGB - extends each 4-bit hex value to be 16-bit.
        self.assertEqual(rgb('#F0F'), (0xFFFF, 0x0000, 0xFFFF))
        # #RRGGBB - extends each 8-bit hex value to be 16-bit.
        assertApprox(rgb('#4a3c8c'), (0x4a4a, 0x3c3c, 0x8c8c))
        # #RRRRGGGGBBBB
        assertApprox(rgb('#dede14143939'), (0xdede, 0x1414, 0x3939))
        # Invalid string.
        with self.assertRaises(tkinter.TclError):
            rgb('#123456789a')
        # RGB triplet is invalid input.
        with self.assertRaises(tkinter.TclError):
            rgb((111, 78, 55))

    def test_event_repr_defaults(self):
        e = tkinter.Event()
        e.serial = 12345
        e.num = '??'
        e.height = '??'
        e.keycode = '??'
        e.state = 0
        e.time = 123456789
        e.width = '??'
        e.x = '??'
        e.y = '??'
        e.char = ''
        e.keysym = '??'
        e.keysym_num = '??'
        e.type = '100'
        e.widget = '??'
        e.x_root = '??'
        e.y_root = '??'
        e.delta = 0
        self.assertEqual(repr(e), '<100 event>')

    def test_event_repr(self):
        e = tkinter.Event()
        e.serial = 12345
        e.num = 3
        e.focus = True
        e.height = 200
        e.keycode = 65
        e.state = 0x30405
        e.time = 123456789
        e.width = 300
        e.x = 10
        e.y = 20
        e.char = 'A'
        e.send_event = True
        e.keysym = 'Key-A'
        e.keysym_num = ord('A')
        e.type = tkinter.EventType.Configure
        e.widget = '.text'
        e.x_root = 1010
        e.y_root = 1020
        e.delta = -1
        self.assertEqual(repr(e),
                         "<Configure event send_event=True"
                         " state=Shift|Control|Button3|0x30000"
                         " keysym=Key-A keycode=65 char='A'"
                         " num=3 delta=-1 focus=True"
                         " x=10 y=20 width=300 height=200>")

    def test_eventtype_enum(self):
        class CheckedEventType(enum.StrEnum):
            KeyPress = '2'
            Key = KeyPress
            KeyRelease = '3'
            ButtonPress = '4'
            Button = ButtonPress
            ButtonRelease = '5'
            Motion = '6'
            Enter = '7'
            Leave = '8'
            FocusIn = '9'
            FocusOut = '10'
            Keymap = '11'           # undocumented
            Expose = '12'
            GraphicsExpose = '13'   # undocumented
            NoExpose = '14'         # undocumented
            Visibility = '15'
            Create = '16'
            Destroy = '17'
            Unmap = '18'
            Map = '19'
            MapRequest = '20'
            Reparent = '21'
            Configure = '22'
            ConfigureRequest = '23'
            Gravity = '24'
            ResizeRequest = '25'
            Circulate = '26'
            CirculateRequest = '27'
            Property = '28'
            SelectionClear = '29'   # undocumented
            SelectionRequest = '30' # undocumented
            Selection = '31'        # undocumented
            Colormap = '32'
            ClientMessage = '33'    # undocumented
            Mapping = '34'          # undocumented
            VirtualEvent = '35'     # undocumented
            Activate = '36'
            Deactivate = '37'
            MouseWheel = '38'
        enum._test_simple_enum(CheckedEventType, tkinter.EventType)

    def test_getboolean(self):
        for v in 'true', 'yes', 'on', '1', 't', 'y', 1, True:
            self.assertIs(self.root.getboolean(v), True)
        for v in 'false', 'no', 'off', '0', 'f', 'n', 0, False:
            self.assertIs(self.root.getboolean(v), False)
        self.assertRaises(ValueError, self.root.getboolean, 'yea')
        self.assertRaises(ValueError, self.root.getboolean, '')
        self.assertRaises(TypeError, self.root.getboolean, None)
        self.assertRaises(TypeError, self.root.getboolean, ())

    def test_mainloop(self):
        log = []
        def callback():
            log.append(1)
            self.root.after(100, self.root.quit)
        self.root.after(100, callback)
        self.root.mainloop(1)
        self.assertEqual(log, [])
        self.root.mainloop(0)
        self.assertEqual(log, [1])
        self.assertTrue(self.root.winfo_exists())


class DefaultRootTest(AbstractDefaultRootTest, unittest.TestCase):

    def test_default_root(self):
        self.assertIs(tkinter._support_default_root, True)
        self.assertIsNone(tkinter._default_root)
        root = tkinter.Tk()
        root2 = tkinter.Tk()
        root3 = tkinter.Tk()
        self.assertIs(tkinter._default_root, root)
        root2.destroy()
        self.assertIs(tkinter._default_root, root)
        root.destroy()
        self.assertIsNone(tkinter._default_root)
        root3.destroy()
        self.assertIsNone(tkinter._default_root)

    def test_no_default_root(self):
        self.assertIs(tkinter._support_default_root, True)
        self.assertIsNone(tkinter._default_root)
        root = tkinter.Tk()
        self.assertIs(tkinter._default_root, root)
        tkinter.NoDefaultRoot()
        self.assertIs(tkinter._support_default_root, False)
        self.assertFalse(hasattr(tkinter, '_default_root'))
        # repeated call is no-op
        tkinter.NoDefaultRoot()
        self.assertIs(tkinter._support_default_root, False)
        self.assertFalse(hasattr(tkinter, '_default_root'))
        root.destroy()
        self.assertIs(tkinter._support_default_root, False)
        self.assertFalse(hasattr(tkinter, '_default_root'))
        root = tkinter.Tk()
        self.assertIs(tkinter._support_default_root, False)
        self.assertFalse(hasattr(tkinter, '_default_root'))
        root.destroy()

    def test_getboolean(self):
        self.assertRaises(RuntimeError, tkinter.getboolean, '1')
        root = tkinter.Tk()
        self.assertIs(tkinter.getboolean('1'), True)
        self.assertRaises(ValueError, tkinter.getboolean, 'yea')
        root.destroy()
        tkinter.NoDefaultRoot()
        self.assertRaises(RuntimeError, tkinter.getboolean, '1')

    def test_mainloop(self):
        self.assertRaises(RuntimeError, tkinter.mainloop)
        root = tkinter.Tk()
        root.after_idle(root.quit)
        tkinter.mainloop()
        root.destroy()
        tkinter.NoDefaultRoot()
        self.assertRaises(RuntimeError, tkinter.mainloop)


if __name__ == "__main__":
    unittest.main()
