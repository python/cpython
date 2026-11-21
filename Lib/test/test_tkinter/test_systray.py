import logging
import sys
import unittest
import tkinter
from tkinter.systray import SysTrayIcon, notify, NotificationHandler
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

    @requires_tk(8, 7)
    def test_notification_handler(self):
        # All real notifications are sent from the same Tcl interpreter.
        # Sending a notification after the interpreter which sent
        # the previous one was deleted crashes Tk on X11 with libnotify.
        if self.root._windowingsystem != 'x11':
            self.skipTest('cannot safely send a native notification')
        self.create()
        handler = NotificationHandler('Python test', master=self.root)
        record = logging.LogRecord('test', logging.INFO, __file__, 0,
                                   'tkinter.systray test notification',
                                   None, None)
        # Sends a real desktop notification.
        handler.emit(record)


class FakeTk:
    # Records the calls of "tk sysnotify" instead of sending
    # real notifications.

    def __init__(self):
        self.calls = []

    def call(self, *args):
        if args[:2] != ('tk', 'sysnotify'):
            raise tkinter.TclError(f'unexpected call: {args}')
        self.calls.append(args[2:])


class FakeMaster:
    def __init__(self):
        self.tk = FakeTk()


class NotificationHandlerTest(unittest.TestCase):

    def setUp(self):
        self.master = FakeMaster()
        self.calls = self.master.tk.calls
        self.logger = logging.getLogger('test.tkinter.systray')
        self.logger.propagate = False
        self.logger.setLevel(logging.DEBUG)
        self.addCleanup(self.logger.setLevel, logging.NOTSET)
        self.addCleanup(setattr, self.logger, 'propagate', True)

    def add_handler(self, *args, **kwargs):
        handler = NotificationHandler(*args, master=self.master, **kwargs)
        self.logger.addHandler(handler)
        self.addCleanup(self.logger.removeHandler, handler)
        return handler

    def test_emit(self):
        self.add_handler()
        self.logger.warning('spam %s', 'eggs')
        self.logger.error('ham')
        self.assertEqual(self.calls, [('WARNING', 'spam eggs'),
                                      ('ERROR', 'ham')])

    def test_title(self):
        self.add_handler('Python')
        self.logger.warning('spam')
        self.assertEqual(self.calls, [('Python', 'spam')])

    def test_formatter(self):
        handler = self.add_handler()
        handler.setFormatter(logging.Formatter('%(name)s: %(message)s'))
        self.logger.info('spam')
        self.assertEqual(self.calls,
                         [('INFO', 'test.tkinter.systray: spam')])

    def test_level(self):
        handler = self.add_handler()
        handler.setLevel(logging.WARNING)
        self.logger.info('spam')
        self.logger.warning('eggs')
        self.assertEqual(self.calls, [('WARNING', 'eggs')])

    def test_error(self):
        # Errors in sending a notification are handled by handleError().
        handler = self.add_handler()
        errors = []
        handler.handleError = errors.append
        def call(*args):
            raise tkinter.TclError('no notifications')
        self.master.tk.call = call
        self.logger.warning('spam')
        self.assertEqual(len(errors), 1)
        self.assertEqual(errors[0].getMessage(), 'spam')


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

    def test_notification_handler(self):
        # The default root window is looked up when a record is emitted.
        handler = NotificationHandler()
        self.assertIsNone(handler.master)
        errors = []
        handler.handleError = lambda record: errors.append(sys.exception())
        record = logging.LogRecord('test', logging.INFO, __file__, 0,
                                   'message', None, None)
        tkinter.NoDefaultRoot()
        handler.emit(record)
        self.assertEqual(len(errors), 1)
        self.assertIsInstance(errors[0], RuntimeError)


if __name__ == "__main__":
    unittest.main()
