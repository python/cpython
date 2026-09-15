import unittest
import tkinter
from tkinter.systray import SysTrayIcon, notify
from test.support import requires
from test.test_tkinter.support import (AbstractTkTest,
                                       AbstractDefaultRootTest,
                                       requires_tk,
                                       setUpModule)  # noqa: F401

requires('gui')


class SysTrayIconTest(AbstractTkTest, unittest.TestCase):

    def setUp(self):
        super().setUp()
        self.image = tkinter.PhotoImage(master=self.root,
                                        width=16, height=16)

    def create(self, **kwargs):
        try:
            icon = SysTrayIcon(self.root, image=self.image, **kwargs)
        except tkinter.TclError as e:
            self.skipTest(f'cannot create a system tray icon: {e}')
        self.addCleanup(self._destroy, icon)
        return icon

    def _destroy(self, icon):
        try:
            icon.destroy()
        except tkinter.TclError:
            pass

    @requires_tk(8, 7)
    def test_create(self):
        icon = self.create(text='tooltip')
        self.assertTrue(icon.exists())
        self.assertEqual(str(icon.cget('image')), str(self.image))
        self.assertEqual(icon.cget('text'), 'tooltip')

    @requires_tk(8, 7)
    def test_create_requires_image(self):
        with self.assertRaises(TypeError):
            SysTrayIcon(self.root)

    @requires_tk(8, 7)
    def test_exists_argument(self):
        icon = self.create(text='tooltip')
        # exists=True refers to the already-created icon without creating
        # a new one (which would raise the singleton error).
        icon2 = SysTrayIcon(self.root, exists=True)
        self.assertTrue(icon2.exists())
        self.assertEqual(icon2.cget('text'), 'tooltip')
        # It can reconfigure the existing icon.
        SysTrayIcon(self.root, exists=True, text='new')
        self.assertEqual(icon.cget('text'), 'new')

    @requires_tk(8, 7)
    def test_singleton(self):
        self.create()
        with self.assertRaisesRegex(tkinter.TclError,
                                    'only one system tray icon'):
            SysTrayIcon(self.root, image=self.image)

    @requires_tk(8, 7)
    def test_configure(self):
        icon = self.create(text='old')
        icon.configure(text='new')
        self.assertEqual(icon.cget('text'), 'new')
        options = icon.configure()
        self.assertIsInstance(options, dict)
        self.assertLessEqual({'image', 'text', 'button1', 'button3'},
                             options.keys())

    @requires_tk(8, 7)
    def test_callbacks(self):
        clicks = []
        icon = self.create(button1=lambda: clicks.append(1))
        name = icon._command_names['button1']
        # The registered callback is called without arguments.
        self.root.tk.call(name)
        self.assertEqual(clicks, [1])
        # Replacing the callback deletes the old Tcl command.
        icon.configure(button1=lambda: clicks.append(2))
        self.assertFalse(self.root.tk.call('info', 'commands', name))
        self.root.tk.call(icon._command_names['button1'])
        self.assertEqual(clicks, [1, 2])
        # Removing the callback deletes the Tcl command.
        name = icon._command_names['button1']
        icon.configure(button1=None)
        self.assertNotIn('button1', icon._command_names)
        self.assertFalse(self.root.tk.call('info', 'commands', name))

    @requires_tk(8, 7)
    def test_destroy(self):
        icon = self.create(button1=lambda: None)
        name = icon._command_names['button1']
        icon.destroy()
        self.assertFalse(icon.exists())
        self.assertFalse(self.root.tk.call('info', 'commands', name))
        # A new icon can be created after the old one was destroyed.
        icon2 = self.create()
        self.assertTrue(icon2.exists())

    @requires_tk(8, 7)
    def test_notify(self):
        if self.root._windowingsystem != 'x11':
            self.skipTest('cannot safely send a native notification')
        icon = self.create()
        # Sends a real desktop notification.
        icon.notify('Python test', 'tkinter.systray test notification')


class DefaultRootTest(AbstractDefaultRootTest, unittest.TestCase):

    @requires_tk(8, 7)
    def test_systray(self):
        root = tkinter.Tk()
        image = tkinter.PhotoImage(master=root, width=16, height=16)
        try:
            icon = SysTrayIcon(image=image)
        except tkinter.TclError as e:
            root.destroy()
            self.skipTest(f'cannot create a system tray icon: {e}')
        self.assertIs(icon.master, root)
        icon.destroy()
        root.destroy()
        tkinter.NoDefaultRoot()
        self.assertRaises(RuntimeError, SysTrayIcon, image='none')
        self.assertRaises(RuntimeError, notify, 'title', 'message')


if __name__ == "__main__":
    unittest.main()
