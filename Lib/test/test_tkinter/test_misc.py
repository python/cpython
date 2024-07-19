import functools
import unittest
import tkinter
from tkinter import TclError
import enum
from test import support
from test.test_tkinter.support import AbstractTkTest, AbstractDefaultRootTest, requires_tk

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

    @requires_tk(8, 6, 6)
    def test_tk_busy(self):
        root = self.root
        f = tkinter.Frame(root, name='myframe')
        f2 = tkinter.Frame(root)
        f.pack()
        f2.pack()
        b = tkinter.Button(f)
        b.pack()
        f.tk_busy_hold()
        with self.assertRaisesRegex(TclError, 'unknown option "-spam"'):
            f.tk_busy_configure(spam='eggs')
        with self.assertRaisesRegex(TclError, 'unknown option "-spam"'):
            f.tk_busy_cget('spam')
        with self.assertRaisesRegex(TclError, 'unknown option "-spam"'):
            f.tk_busy_configure('spam')
        self.assertIsInstance(f.tk_busy_configure(), dict)

        self.assertTrue(f.tk_busy_status())
        self.assertFalse(root.tk_busy_status())
        self.assertFalse(f2.tk_busy_status())
        self.assertFalse(b.tk_busy_status())
        self.assertIn(f, f.tk_busy_current())
        self.assertIn(f, f.tk_busy_current('*.m?f*me'))
        self.assertNotIn(f, f.tk_busy_current('*spam'))

        f.tk_busy_forget()
        self.assertFalse(f.tk_busy_status())
        self.assertFalse(f.tk_busy_current())
        with self.assertRaisesRegex(TclError, "can't find busy window"):
            f.tk_busy_configure()
        with self.assertRaisesRegex(TclError, "can't find busy window"):
            f.tk_busy_forget()

    @requires_tk(8, 6, 6)
    def test_tk_busy_with_cursor(self):
        root = self.root
        if root._windowingsystem == 'aqua':
            self.skipTest('the cursor option is not supported on OSX/Aqua')
        f = tkinter.Frame(root, name='myframe')
        f.pack()
        f.tk_busy_hold(cursor='gumby')

        self.assertEqual(f.tk_busy_cget('cursor'), 'gumby')
        f.tk_busy_configure(cursor='heart')
        self.assertEqual(f.tk_busy_cget('cursor'), 'heart')
        self.assertEqual(f.tk_busy_configure()['cursor'][4], 'heart')
        self.assertEqual(f.tk_busy_configure('cursor')[4], 'heart')

        f.tk_busy_forget()
        with self.assertRaisesRegex(TclError, "can't find busy window"):
            f.tk_busy_cget('cursor')

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

    def test_after_info(self):
        root = self.root

        # No events.
        self.assertEqual(root.after_info(), ())

        # Add timer.
        timer = root.after(1, lambda: 'break')

        # With no parameter, it returns a tuple of the event handler ids.
        self.assertEqual(root.after_info(), (timer, ))
        root.after_cancel(timer)

        timer1 = root.after(5000, lambda: 'break')
        timer2 = root.after(5000, lambda: 'break')
        idle1 = root.after_idle(lambda: 'break')
        # Only contains new events and not 'timer'.
        self.assertEqual(root.after_info(), (idle1, timer2, timer1))

        # With a parameter returns a tuple of (script, type).
        timer1_info = root.after_info(timer1)
        self.assertEqual(len(timer1_info), 2)
        self.assertEqual(timer1_info[1], 'timer')
        idle1_info = root.after_info(idle1)
        self.assertEqual(len(idle1_info), 2)
        self.assertEqual(idle1_info[1], 'idle')

        root.after_cancel(timer1)
        with self.assertRaises(tkinter.TclError):
            root.after_info(timer1)
        root.after_cancel(timer2)
        with self.assertRaises(tkinter.TclError):
            root.after_info(timer2)
        root.after_cancel(idle1)
        with self.assertRaises(tkinter.TclError):
            root.after_info(idle1)

        # No events.
        self.assertEqual(root.after_info(), ())

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

    def test_winfo_pathname(self):
        t = tkinter.Toplevel(self.root)
        w = tkinter.Button(t)
        wid = w.winfo_id()
        self.assertIsInstance(wid, int)
        self.assertEqual(self.root.winfo_pathname(hex(wid)), str(w))
        self.assertEqual(self.root.winfo_pathname(hex(wid), displayof=None), str(w))
        self.assertEqual(self.root.winfo_pathname(hex(wid), displayof=t), str(w))
        self.assertEqual(self.root.winfo_pathname(wid), str(w))
        self.assertEqual(self.root.winfo_pathname(wid, displayof=None), str(w))
        self.assertEqual(self.root.winfo_pathname(wid, displayof=t), str(w))

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

    def test_info_patchlevel(self):
        vi = self.root.info_patchlevel()
        f = tkinter.Frame(self.root)
        self.assertEqual(f.info_patchlevel(), vi)
        # The following is almost a copy of tests for sys.version_info.
        self.assertIsInstance(vi[:], tuple)
        self.assertEqual(len(vi), 5)
        self.assertIsInstance(vi[0], int)
        self.assertIsInstance(vi[1], int)
        self.assertIsInstance(vi[2], int)
        self.assertIn(vi[3], ("alpha", "beta", "candidate", "final"))
        self.assertIsInstance(vi[4], int)
        self.assertIsInstance(vi.major, int)
        self.assertIsInstance(vi.minor, int)
        self.assertIsInstance(vi.micro, int)
        self.assertIn(vi.releaselevel, ("alpha", "beta", "final"))
        self.assertIsInstance(vi.serial, int)
        self.assertEqual(vi[0], vi.major)
        self.assertEqual(vi[1], vi.minor)
        self.assertEqual(vi[2], vi.micro)
        self.assertEqual(vi[3], vi.releaselevel)
        self.assertEqual(vi[4], vi.serial)
        self.assertTrue(vi > (1,0,0))
        if vi.releaselevel == 'final':
            self.assertEqual(vi.serial, 0)
        else:
            self.assertEqual(vi.micro, 0)
        self.assertTrue(str(vi).startswith(f'{vi.major}.{vi.minor}'))

    def test_embedded_null(self):
        widget = tkinter.Entry(self.root)
        widget.insert(0, 'abc\0def')  # ASCII-only
        widget.selection_range(0, 'end')
        self.assertEqual(widget.selection_get(), 'abc\x00def')
        widget.insert(0, '\u20ac\0')  # non-ASCII
        widget.selection_range(0, 'end')
        self.assertEqual(widget.selection_get(), '\u20ac\0abc\x00def')


class WmTest(AbstractTkTest, unittest.TestCase):

    def test_wm_attribute(self):
        w = self.root
        attributes = w.wm_attributes(return_python_dict=True)
        self.assertIsInstance(attributes, dict)
        attributes2 = w.wm_attributes()
        self.assertIsInstance(attributes2, tuple)
        self.assertEqual(attributes2[::2],
                         tuple('-' + k for k in attributes))
        self.assertEqual(attributes2[1::2], tuple(attributes.values()))
        # silently deprecated
        attributes3 = w.wm_attributes(None)
        if self.wantobjects:
            self.assertEqual(attributes3, attributes2)
        else:
            self.assertIsInstance(attributes3, str)

        for name in attributes:
            self.assertEqual(w.wm_attributes(name), attributes[name])
        # silently deprecated
        for name in attributes:
            self.assertEqual(w.wm_attributes('-' + name), attributes[name])

        self.assertIn('alpha', attributes)
        self.assertIn('fullscreen', attributes)
        self.assertIn('topmost', attributes)
        if w._windowingsystem == "win32":
            self.assertIn('disabled', attributes)
            self.assertIn('toolwindow', attributes)
            self.assertIn('transparentcolor', attributes)
        if w._windowingsystem == "aqua":
            self.assertIn('modified', attributes)
            self.assertIn('notify', attributes)
            self.assertIn('titlepath', attributes)
            self.assertIn('transparent', attributes)
        if w._windowingsystem == "x11":
            self.assertIn('type', attributes)
            self.assertIn('zoomed', attributes)

        w.wm_attributes(alpha=0.5)
        self.assertEqual(w.wm_attributes('alpha'),
                         0.5 if self.wantobjects else '0.5')
        w.wm_attributes(alpha=1.0)
        self.assertEqual(w.wm_attributes('alpha'),
                         1.0 if self.wantobjects else '1.0')
        # silently deprecated
        w.wm_attributes('-alpha', 0.5)
        self.assertEqual(w.wm_attributes('alpha'),
                         0.5 if self.wantobjects else '0.5')
        w.wm_attributes(alpha=1.0)
        self.assertEqual(w.wm_attributes('alpha'),
                         1.0 if self.wantobjects else '1.0')


class EventTest(AbstractTkTest, unittest.TestCase):

    def test_focus(self):
        f = tkinter.Frame(self.root, width=150, height=100)
        f.pack()
        self.root.wait_visibility()  # needed on Windows
        self.root.update_idletasks()

        events = []
        f.bind('<FocusIn>', events.append)

        f.focus_force()
        self.root.update()
        self.assertEqual(len(events), 1, events)
        e = events[0]
        self.assertIs(e.type, tkinter.EventType.FocusIn)
        self.assertIs(e.widget, f)
        self.assertIsInstance(e.serial, int)
        self.assertEqual(e.time, '??')
        self.assertIs(e.send_event, False)
        self.assertFalse(hasattr(e, 'focus'))
        self.assertEqual(e.num, '??')
        self.assertEqual(e.state, '??')
        self.assertEqual(e.char, '??')
        self.assertEqual(e.keycode, '??')
        self.assertEqual(e.keysym, '??')
        self.assertEqual(e.keysym_num, '??')
        self.assertEqual(e.width, '??')
        self.assertEqual(e.height, '??')
        self.assertEqual(e.x, '??')
        self.assertEqual(e.y, '??')
        self.assertEqual(e.x_root, '??')
        self.assertEqual(e.y_root, '??')
        self.assertEqual(e.delta, 0)
        self.assertEqual(repr(e), '<FocusIn event>')

    def test_configure(self):
        f = tkinter.Frame(self.root, width=150, height=100)
        f.pack()
        self.root.wait_visibility()  # needed on Windows
        self.root.update_idletasks()

        events = []
        f.bind('<Configure>', events.append)

        f.configure(height=120, borderwidth=10)
        self.assertEqual(len(events), 1, events)
        e = events[0]
        self.assertIs(e.type, tkinter.EventType.Configure)
        self.assertIs(e.widget, f)
        self.assertIsInstance(e.serial, int)
        self.assertEqual(e.time, '??')
        self.assertIs(e.send_event, False)
        self.assertFalse(hasattr(e, 'focus'))
        self.assertEqual(e.num, '??')
        self.assertEqual(e.state, '??')
        self.assertEqual(e.char, '??')
        self.assertEqual(e.keycode, '??')
        self.assertEqual(e.keysym, '??')
        self.assertEqual(e.keysym_num, '??')
        self.assertEqual(e.width, 150)
        self.assertEqual(e.height, 100)
        self.assertEqual(e.x, 0)
        self.assertEqual(e.y, 0)
        self.assertEqual(e.x_root, '??')
        self.assertEqual(e.y_root, '??')
        self.assertEqual(e.delta, 0)
        self.assertEqual(repr(e), '<Configure event x=0 y=0 width=150 height=100>')

    def test_event_generate_key_press(self):
        f = tkinter.Frame(self.root, width=150, height=100)
        f.pack()
        self.root.wait_visibility()  # needed on Windows
        self.root.update_idletasks()

        events = []
        f.bind('<KeyPress>', events.append)
        f.focus_force()

        f.event_generate('<Alt-z>')
        self.assertEqual(len(events), 1, events)
        e = events[0]
        self.assertIs(e.type, tkinter.EventType.KeyPress)
        self.assertIs(e.widget, f)
        self.assertIsInstance(e.serial, int)
        self.assertEqual(e.time, 0)
        self.assertIs(e.send_event, False)
        self.assertFalse(hasattr(e, 'focus'))
        self.assertEqual(e.num, '??')
        self.assertIsInstance(e.state, int)
        self.assertNotEqual(e.state, 0)
        self.assertEqual(e.char, 'z')
        self.assertIsInstance(e.keycode, int)
        self.assertNotEqual(e.keycode, 0)
        self.assertEqual(e.keysym, 'z')
        self.assertEqual(e.keysym_num, ord('z'))
        self.assertEqual(e.width, '??')
        self.assertEqual(e.height, '??')
        self.assertEqual(e.x, -1 - f.winfo_rootx())
        self.assertEqual(e.y, -1 - f.winfo_rooty())
        self.assertEqual(e.x_root, -1)
        self.assertEqual(e.y_root, -1)
        self.assertEqual(e.delta, 0)
        self.assertEqual(repr(e),
            f"<KeyPress event state={e.state:#x} "
            f"keysym=z keycode={e.keycode} char='z' x={e.x} y={e.y}>")

    def test_event_generate_enter(self):
        f = tkinter.Frame(self.root, width=150, height=100)
        f.pack()
        self.root.wait_visibility()  # needed on Windows
        self.root.update_idletasks()

        events = []
        f.bind('<Enter>', events.append)

        f.event_generate('<Enter>', x=100, y=50)
        self.assertEqual(len(events), 1, events)
        e = events[0]
        self.assertIs(e.type, tkinter.EventType.Enter)
        self.assertIs(e.widget, f)
        self.assertIsInstance(e.serial, int)
        self.assertEqual(e.time, 0)
        self.assertIs(e.send_event, False)
        self.assertIs(e.focus, False)
        self.assertEqual(e.num, '??')
        self.assertEqual(e.state, 0)
        self.assertEqual(e.char, '??')
        self.assertEqual(e.keycode, '??')
        self.assertEqual(e.keysym, '??')
        self.assertEqual(e.keysym_num, '??')
        self.assertEqual(e.width, '??')
        self.assertEqual(e.height, '??')
        self.assertEqual(e.x, 100)
        self.assertEqual(e.y, 50)
        self.assertEqual(e.x_root, 100 + f.winfo_rootx())
        self.assertEqual(e.y_root, 50 + f.winfo_rooty())
        self.assertEqual(e.delta, 0)
        self.assertEqual(repr(e), '<Enter event focus=False x=100 y=50>')

    def test_event_generate_button_press(self):
        f = tkinter.Frame(self.root, width=150, height=100)
        f.pack()
        self.root.wait_visibility()  # needed on Windows
        self.root.update_idletasks()

        events = []
        f.bind('<ButtonPress>', events.append)
        f.focus_force()

        f.event_generate('<Button-1>', x=100, y=50)
        self.assertEqual(len(events), 1, events)
        e = events[0]
        self.assertIs(e.type, tkinter.EventType.ButtonPress)
        self.assertIs(e.widget, f)
        self.assertIsInstance(e.serial, int)
        self.assertEqual(e.time, 0)
        self.assertIs(e.send_event, False)
        self.assertFalse(hasattr(e, 'focus'))
        self.assertEqual(e.num, 1)
        self.assertEqual(e.state, 0)
        self.assertEqual(e.char, '??')
        self.assertEqual(e.keycode, '??')
        self.assertEqual(e.keysym, '??')
        self.assertEqual(e.keysym_num, '??')
        self.assertEqual(e.width, '??')
        self.assertEqual(e.height, '??')
        self.assertEqual(e.x, 100)
        self.assertEqual(e.y, 50)
        self.assertEqual(e.x_root, f.winfo_rootx() + 100)
        self.assertEqual(e.y_root, f.winfo_rooty() + 50)
        self.assertEqual(e.delta, 0)
        self.assertEqual(repr(e), '<ButtonPress event num=1 x=100 y=50>')

    def test_event_generate_motion(self):
        f = tkinter.Frame(self.root, width=150, height=100)
        f.pack()
        self.root.wait_visibility()  # needed on Windows
        self.root.update_idletasks()

        events = []
        f.bind('<Motion>', events.append)
        f.focus_force()

        f.event_generate('<B1-Motion>', x=100, y=50)
        self.assertEqual(len(events), 1, events)
        e = events[0]
        self.assertIs(e.type, tkinter.EventType.Motion)
        self.assertIs(e.widget, f)
        self.assertIsInstance(e.serial, int)
        self.assertEqual(e.time, 0)
        self.assertIs(e.send_event, False)
        self.assertFalse(hasattr(e, 'focus'))
        self.assertEqual(e.num, '??')
        self.assertEqual(e.state, 0x100)
        self.assertEqual(e.char, '??')
        self.assertEqual(e.keycode, '??')
        self.assertEqual(e.keysym, '??')
        self.assertEqual(e.keysym_num, '??')
        self.assertEqual(e.width, '??')
        self.assertEqual(e.height, '??')
        self.assertEqual(e.x, 100)
        self.assertEqual(e.y, 50)
        self.assertEqual(e.x_root, f.winfo_rootx() + 100)
        self.assertEqual(e.y_root, f.winfo_rooty() + 50)
        self.assertEqual(e.delta, 0)
        self.assertEqual(repr(e), '<Motion event state=Button1 x=100 y=50>')

    def test_event_generate_mouse_wheel(self):
        f = tkinter.Frame(self.root, width=150, height=100)
        f.pack()
        self.root.wait_visibility()  # needed on Windows
        self.root.update_idletasks()

        events = []
        f.bind('<MouseWheel>', events.append)
        f.focus_force()

        f.event_generate('<MouseWheel>', x=100, y=50, delta=-5)
        self.assertEqual(len(events), 1, events)
        e = events[0]
        self.assertIs(e.type, tkinter.EventType.MouseWheel)
        self.assertIs(e.widget, f)
        self.assertIsInstance(e.serial, int)
        self.assertIs(e.send_event, False)
        self.assertFalse(hasattr(e, 'focus'))
        self.assertEqual(e.time, 0)
        self.assertEqual(e.num, '??')
        self.assertEqual(e.state, 0)
        self.assertEqual(e.char, '??')
        self.assertEqual(e.keycode, '??')
        self.assertEqual(e.keysym, '??')
        self.assertEqual(e.keysym_num, '??')
        self.assertEqual(e.width, '??')
        self.assertEqual(e.height, '??')
        self.assertEqual(e.x, 100)
        self.assertEqual(e.y, 50)
        self.assertEqual(e.x_root, f.winfo_rootx() + 100)
        self.assertEqual(e.y_root, f.winfo_rooty() + 50)
        self.assertEqual(e.delta, -5)
        self.assertEqual(repr(e), '<MouseWheel event delta=-5 x=100 y=50>')

    def test_generate_event_virtual_event(self):
        f = tkinter.Frame(self.root, width=150, height=100)
        f.pack()
        self.root.wait_visibility()  # needed on Windows
        self.root.update_idletasks()

        events = []
        f.bind('<<Spam>>', events.append)
        f.focus_force()

        f.event_generate('<<Spam>>', x=50)
        self.assertEqual(len(events), 1, events)
        e = events[0]
        self.assertIs(e.type, tkinter.EventType.VirtualEvent)
        self.assertIs(e.widget, f)
        self.assertIsInstance(e.serial, int)
        self.assertEqual(e.time, 0)
        self.assertIs(e.send_event, False)
        self.assertFalse(hasattr(e, 'focus'))
        self.assertEqual(e.num, '??')
        self.assertEqual(e.state, 0)
        self.assertEqual(e.char, '??')
        self.assertEqual(e.keycode, '??')
        self.assertEqual(e.keysym, '??')
        self.assertEqual(e.keysym_num, '??')
        self.assertEqual(e.width, '??')
        self.assertEqual(e.height, '??')
        self.assertEqual(e.x, 50)
        self.assertEqual(e.y, 0)
        self.assertEqual(e.x_root, f.winfo_rootx() + 50)
        self.assertEqual(e.y_root, -1)
        self.assertEqual(e.delta, 0)
        self.assertEqual(repr(e),
            f"<VirtualEvent event x=50 y=0>")


class BindTest(AbstractTkTest, unittest.TestCase):

    def setUp(self):
        super().setUp()
        root = self.root
        self.frame = tkinter.Frame(self.root, class_='Test',
                                   width=150, height=100)
        self.frame.pack()

    def assertCommandExist(self, funcid):
        self.assertEqual(_info_commands(self.root, funcid), (funcid,))

    def assertCommandNotExist(self, funcid):
        self.assertEqual(_info_commands(self.root, funcid), ())

    def test_bind(self):
        event = '<Control-Alt-Key-a>'
        f = self.frame
        self.assertEqual(f.bind(), ())
        self.assertEqual(f.bind(event), '')
        def test1(e): pass
        def test2(e): pass

        funcid = f.bind(event, test1)
        self.assertEqual(f.bind(), (event,))
        script = f.bind(event)
        self.assertIn(funcid, script)
        self.assertCommandExist(funcid)

        funcid2 = f.bind(event, test2, add=True)
        script = f.bind(event)
        self.assertIn(funcid, script)
        self.assertIn(funcid2, script)
        self.assertCommandExist(funcid)
        self.assertCommandExist(funcid2)

    def test_unbind(self):
        event = '<Control-Alt-Key-b>'
        f = self.frame
        self.assertEqual(f.bind(), ())
        self.assertEqual(f.bind(event), '')
        def test1(e): pass
        def test2(e): pass

        funcid = f.bind(event, test1)
        funcid2 = f.bind(event, test2, add=True)

        self.assertRaises(TypeError, f.unbind)
        f.unbind(event)
        self.assertEqual(f.bind(event), '')
        self.assertEqual(f.bind(), ())

    def test_unbind2(self):
        f = self.frame
        f.wait_visibility()
        f.focus_force()
        f.update_idletasks()
        event = '<Control-Alt-Key-c>'
        self.assertEqual(f.bind(), ())
        self.assertEqual(f.bind(event), '')
        def test1(e): events.append('a')
        def test2(e): events.append('b')
        def test3(e): events.append('c')

        funcid = f.bind(event, test1)
        funcid2 = f.bind(event, test2, add=True)
        funcid3 = f.bind(event, test3, add=True)
        events = []
        f.event_generate(event)
        self.assertEqual(events, ['a', 'b', 'c'])

        f.unbind(event, funcid2)
        script = f.bind(event)
        self.assertNotIn(funcid2, script)
        self.assertIn(funcid, script)
        self.assertIn(funcid3, script)
        self.assertEqual(f.bind(), (event,))
        self.assertCommandNotExist(funcid2)
        self.assertCommandExist(funcid)
        self.assertCommandExist(funcid3)
        events = []
        f.event_generate(event)
        self.assertEqual(events, ['a', 'c'])

        f.unbind(event, funcid)
        f.unbind(event, funcid3)
        self.assertEqual(f.bind(event), '')
        self.assertEqual(f.bind(), ())
        self.assertCommandNotExist(funcid)
        self.assertCommandNotExist(funcid2)
        self.assertCommandNotExist(funcid3)
        events = []
        f.event_generate(event)
        self.assertEqual(events, [])

        # non-idempotent
        self.assertRaises(tkinter.TclError, f.unbind, event, funcid2)

    def test_bind_rebind(self):
        event = '<Control-Alt-Key-d>'
        f = self.frame
        self.assertEqual(f.bind(), ())
        self.assertEqual(f.bind(event), '')
        def test1(e): pass
        def test2(e): pass
        def test3(e): pass

        funcid = f.bind(event, test1)
        funcid2 = f.bind(event, test2, add=True)
        script = f.bind(event)
        self.assertIn(funcid2, script)
        self.assertIn(funcid, script)
        self.assertCommandExist(funcid)
        self.assertCommandExist(funcid2)

        funcid3 = f.bind(event, test3)
        script = f.bind(event)
        self.assertNotIn(funcid, script)
        self.assertNotIn(funcid2, script)
        self.assertIn(funcid3, script)
        self.assertCommandExist(funcid3)

    def test_bind_class(self):
        event = '<Control-Alt-Key-e>'
        bind_class = self.root.bind_class
        unbind_class = self.root.unbind_class
        self.assertRaises(TypeError, bind_class)
        self.assertEqual(bind_class('Test'), ())
        self.assertEqual(bind_class('Test', event), '')
        self.addCleanup(unbind_class, 'Test', event)
        def test1(e): pass
        def test2(e): pass

        funcid = bind_class('Test', event, test1)
        self.assertEqual(bind_class('Test'), (event,))
        script = bind_class('Test', event)
        self.assertIn(funcid, script)
        self.assertCommandExist(funcid)

        funcid2 = bind_class('Test', event, test2, add=True)
        script = bind_class('Test', event)
        self.assertIn(funcid, script)
        self.assertIn(funcid2, script)
        self.assertCommandExist(funcid)
        self.assertCommandExist(funcid2)

    def test_unbind_class(self):
        event = '<Control-Alt-Key-f>'
        bind_class = self.root.bind_class
        unbind_class = self.root.unbind_class
        self.assertEqual(bind_class('Test'), ())
        self.assertEqual(bind_class('Test', event), '')
        self.addCleanup(unbind_class, 'Test', event)
        def test1(e): pass
        def test2(e): pass

        funcid = bind_class('Test', event, test1)
        funcid2 = bind_class('Test', event, test2, add=True)

        self.assertRaises(TypeError, unbind_class)
        self.assertRaises(TypeError, unbind_class, 'Test')
        unbind_class('Test', event)
        self.assertEqual(bind_class('Test', event), '')
        self.assertEqual(bind_class('Test'), ())
        self.assertCommandExist(funcid)
        self.assertCommandExist(funcid2)

        unbind_class('Test', event)  # idempotent

    def test_bind_class_rebind(self):
        event = '<Control-Alt-Key-g>'
        bind_class = self.root.bind_class
        unbind_class = self.root.unbind_class
        self.assertEqual(bind_class('Test'), ())
        self.assertEqual(bind_class('Test', event), '')
        self.addCleanup(unbind_class, 'Test', event)
        def test1(e): pass
        def test2(e): pass
        def test3(e): pass

        funcid = bind_class('Test', event, test1)
        funcid2 = bind_class('Test', event, test2, add=True)
        script = bind_class('Test', event)
        self.assertIn(funcid2, script)
        self.assertIn(funcid, script)
        self.assertCommandExist(funcid)
        self.assertCommandExist(funcid2)

        funcid3 = bind_class('Test', event, test3)
        script = bind_class('Test', event)
        self.assertNotIn(funcid, script)
        self.assertNotIn(funcid2, script)
        self.assertIn(funcid3, script)
        self.assertCommandExist(funcid)
        self.assertCommandExist(funcid2)
        self.assertCommandExist(funcid3)

    def test_bind_all(self):
        event = '<Control-Alt-Key-h>'
        bind_all = self.root.bind_all
        unbind_all = self.root.unbind_all
        self.assertNotIn(event, bind_all())
        self.assertEqual(bind_all(event), '')
        self.addCleanup(unbind_all, event)
        def test1(e): pass
        def test2(e): pass

        funcid = bind_all(event, test1)
        self.assertIn(event, bind_all())
        script = bind_all(event)
        self.assertIn(funcid, script)
        self.assertCommandExist(funcid)

        funcid2 = bind_all(event, test2, add=True)
        script = bind_all(event)
        self.assertIn(funcid, script)
        self.assertIn(funcid2, script)
        self.assertCommandExist(funcid)
        self.assertCommandExist(funcid2)

    def test_unbind_all(self):
        event = '<Control-Alt-Key-i>'
        bind_all = self.root.bind_all
        unbind_all = self.root.unbind_all
        self.assertNotIn(event, bind_all())
        self.assertEqual(bind_all(event), '')
        self.addCleanup(unbind_all, event)
        def test1(e): pass
        def test2(e): pass

        funcid = bind_all(event, test1)
        funcid2 = bind_all(event, test2, add=True)

        unbind_all(event)
        self.assertEqual(bind_all(event), '')
        self.assertNotIn(event, bind_all())
        self.assertCommandExist(funcid)
        self.assertCommandExist(funcid2)

        unbind_all(event)  # idempotent

    def test_bind_all_rebind(self):
        event = '<Control-Alt-Key-j>'
        bind_all = self.root.bind_all
        unbind_all = self.root.unbind_all
        self.assertNotIn(event, bind_all())
        self.assertEqual(bind_all(event), '')
        self.addCleanup(unbind_all, event)
        def test1(e): pass
        def test2(e): pass
        def test3(e): pass

        funcid = bind_all(event, test1)
        funcid2 = bind_all(event, test2, add=True)
        script = bind_all(event)
        self.assertIn(funcid2, script)
        self.assertIn(funcid, script)
        self.assertCommandExist(funcid)
        self.assertCommandExist(funcid2)

        funcid3 = bind_all(event, test3)
        script = bind_all(event)
        self.assertNotIn(funcid, script)
        self.assertNotIn(funcid2, script)
        self.assertIn(funcid3, script)
        self.assertCommandExist(funcid)
        self.assertCommandExist(funcid2)
        self.assertCommandExist(funcid3)

    def _test_tag_bind(self, w):
        tag = 'sel'
        event = '<Control-Alt-Key-a>'
        w.pack()
        self.assertRaises(TypeError, w.tag_bind)
        tag_bind = w._tag_bind if isinstance(w, tkinter.Text) else w.tag_bind
        if isinstance(w, tkinter.Text):
            self.assertRaises(TypeError, w.tag_bind, tag)
            self.assertRaises(TypeError, w.tag_bind, tag, event)
        self.assertEqual(tag_bind(tag), ())
        self.assertEqual(tag_bind(tag, event), '')
        def test1(e): pass
        def test2(e): pass

        funcid = w.tag_bind(tag, event, test1)
        self.assertEqual(tag_bind(tag), (event,))
        script = tag_bind(tag, event)
        self.assertIn(funcid, script)
        self.assertCommandExist(funcid)

        funcid2 = w.tag_bind(tag, event, test2, add=True)
        script = tag_bind(tag, event)
        self.assertIn(funcid, script)
        self.assertIn(funcid2, script)
        self.assertCommandExist(funcid)
        self.assertCommandExist(funcid2)

    def _test_tag_unbind(self, w):
        tag = 'sel'
        event = '<Control-Alt-Key-b>'
        w.pack()
        tag_bind = w._tag_bind if isinstance(w, tkinter.Text) else w.tag_bind
        self.assertEqual(tag_bind(tag), ())
        self.assertEqual(tag_bind(tag, event), '')
        def test1(e): pass
        def test2(e): pass

        funcid = w.tag_bind(tag, event, test1)
        funcid2 = w.tag_bind(tag, event, test2, add=True)

        self.assertRaises(TypeError, w.tag_unbind, tag)
        w.tag_unbind(tag, event)
        self.assertEqual(tag_bind(tag, event), '')
        self.assertEqual(tag_bind(tag), ())

    def _test_tag_bind_rebind(self, w):
        tag = 'sel'
        event = '<Control-Alt-Key-d>'
        w.pack()
        tag_bind = w._tag_bind if isinstance(w, tkinter.Text) else w.tag_bind
        self.assertEqual(tag_bind(tag), ())
        self.assertEqual(tag_bind(tag, event), '')
        def test1(e): pass
        def test2(e): pass
        def test3(e): pass

        funcid = w.tag_bind(tag, event, test1)
        funcid2 = w.tag_bind(tag, event, test2, add=True)
        script = tag_bind(tag, event)
        self.assertIn(funcid2, script)
        self.assertIn(funcid, script)
        self.assertCommandExist(funcid)
        self.assertCommandExist(funcid2)

        funcid3 = w.tag_bind(tag, event, test3)
        script = tag_bind(tag, event)
        self.assertNotIn(funcid, script)
        self.assertNotIn(funcid2, script)
        self.assertIn(funcid3, script)
        self.assertCommandExist(funcid3)

    def test_canvas_tag_bind(self):
        c = tkinter.Canvas(self.frame)
        self._test_tag_bind(c)

    def test_canvas_tag_unbind(self):
        c = tkinter.Canvas(self.frame)
        self._test_tag_unbind(c)

    def test_canvas_tag_bind_rebind(self):
        c = tkinter.Canvas(self.frame)
        self._test_tag_bind_rebind(c)

    def test_text_tag_bind(self):
        t = tkinter.Text(self.frame)
        self._test_tag_bind(t)

    def test_text_tag_unbind(self):
        t = tkinter.Text(self.frame)
        self._test_tag_unbind(t)

    def test_text_tag_bind_rebind(self):
        t = tkinter.Text(self.frame)
        self._test_tag_bind_rebind(t)

    def test_bindtags(self):
        f = self.frame
        self.assertEqual(self.root.bindtags(), ('.', 'Tk', 'all'))
        self.assertEqual(f.bindtags(), (str(f), 'Test', '.', 'all'))
        f.bindtags(('a', 'b c'))
        self.assertEqual(f.bindtags(), ('a', 'b c'))

    def test_bind_events(self):
        event = '<Enter>'
        root = self.root
        t = tkinter.Toplevel(root)
        f = tkinter.Frame(t, class_='Test', width=150, height=100)
        f.pack()
        root.wait_visibility()  # needed on Windows
        root.update_idletasks()
        self.addCleanup(root.unbind_class, 'Test', event)
        self.addCleanup(root.unbind_class, 'Toplevel', event)
        self.addCleanup(root.unbind_class, 'tag', event)
        self.addCleanup(root.unbind_class, 'tag2', event)
        self.addCleanup(root.unbind_all, event)
        def test(what):
            return lambda e: events.append((what, e.widget))

        root.bind_all(event, test('all'))
        root.bind_class('Test', event, test('frame class'))
        root.bind_class('Toplevel', event, test('toplevel class'))
        root.bind_class('tag', event, test('tag'))
        root.bind_class('tag2', event, test('tag2'))
        f.bind(event, test('frame'))
        t.bind(event, test('toplevel'))

        events = []
        f.event_generate(event)
        self.assertEqual(events, [
            ('frame', f),
            ('frame class', f),
            ('toplevel', f),
            ('all', f),
        ])

        events = []
        t.event_generate(event)
        self.assertEqual(events, [
            ('toplevel', t),
            ('toplevel class', t),
            ('all', t),
        ])

        f.bindtags(('tag', 'tag3'))
        events = []
        f.event_generate(event)
        self.assertEqual(events, [('tag', f)])


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


def _info_commands(widget, pattern=None):
    return widget.tk.splitlist(widget.tk.call('info', 'commands', pattern))


if __name__ == "__main__":
    unittest.main()
