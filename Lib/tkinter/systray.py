"""Interface to the system tray icon and desktop notifications.

The SysTrayIcon class gives access to the "tk systray" command and
the notify() function gives access to the "tk sysnotify" command.
They require Tk 8.7/9.0 or newer.

Only one system tray icon is supported per Tcl interpreter.

On Windows, sending a notification requires that the system tray icon
has been created first; the icon is also displayed in the
notification.
"""

import tkinter

__all__ = ["SysTrayIcon", "notify"]


class SysTrayIcon:
    """The system tray icon.

    Only one system tray icon is supported per Tcl interpreter.
    With exists false (the default) a new icon is created, and creating
    a second one raises TclError.  With exists true this refers to the
    already-existing icon instead of creating one.

    Supported configuration options are:

        image: the image displayed in the system tray (required when
               creating an icon; it must be a photo image on Windows)
        text: the text displayed in the tooltip of the icon
        button1: a callback that is called when the icon is clicked
                 with the left mouse button
        button3: a callback that is called when the icon is clicked
                 with the right mouse button
    """

    def __init__(self, master=None, *, exists=False, **options):
        if master is None:
            master = tkinter._get_default_root('use the system tray icon')
        self.master = master
        self._command_names = {}
        if exists:
            # Refer to the already-existing icon, reconfiguring it if
            # any options are given.
            if options:
                self._call('configure', options)
        else:
            if options.get('image') is None:
                raise TypeError(
                    "the 'image' argument is required to create an icon")
            self._call('create', options)

    def _call(self, subcommand, cnf):
        # Call "tk systray" with the given options, registering and
        # unregistering the callback commands as needed.
        master = self.master
        cnf = dict(cnf)
        new_names = {}
        for key in ('button1', 'button3'):
            if key in cnf:
                command = cnf[key]
                if command is None:
                    cnf[key] = ''
                elif callable(command):
                    new_names[key] = cnf[key] = master._register(command)
        try:
            master.tk.call('tk', 'systray', subcommand,
                           *master._options(cnf))
        except tkinter.TclError:
            for name in new_names.values():
                master.deletecommand(name)
            raise
        for key in ('button1', 'button3'):
            if key in cnf:
                old_name = self._command_names.pop(key, None)
                if old_name is not None:
                    master.deletecommand(old_name)
                if key in new_names:
                    self._command_names[key] = new_names[key]

    def configure(self, cnf=None, **kw):
        """Query or modify the options of the system tray icon.

        With no arguments, return a dict of all option values.  With a
        string argument, return the value of that option.  Otherwise,
        set the given options.
        """
        if kw:
            cnf = tkinter._cnfmerge((cnf, kw))
        elif cnf:
            cnf = tkinter._cnfmerge(cnf)
        tk = self.master.tk
        if cnf is None:
            items = tk.splitlist(tk.call('tk', 'systray', 'configure'))
            return {items[i][1:]: items[i+1] for i in range(0, len(items), 2)}
        if isinstance(cnf, str):
            return tk.call('tk', 'systray', 'configure', '-' + cnf)
        self._call('configure', cnf)
    config = configure

    def cget(self, option):
        """Return the value of the given option of the system tray icon."""
        return self.master.tk.call('tk', 'systray', 'configure', '-' + option)

    def exists(self):
        """Return whether the system tray icon exists."""
        tk = self.master.tk
        return tk.getboolean(tk.call('tk', 'systray', 'exists'))

    def destroy(self):
        """Destroy the system tray icon."""
        self.master.tk.call('tk', 'systray', 'destroy')
        for name in self._command_names.values():
            self.master.deletecommand(name)
        self._command_names.clear()

    def notify(self, title, message):
        """Send a desktop notification with the given title and message."""
        self.master.tk.call('tk', 'sysnotify', title, message)


def notify(title, message, *, master=None):
    """Send a desktop notification with the given title and message.

    On Windows, the system tray icon must have been created first.
    """
    if master is None:
        master = tkinter._get_default_root('send a notification')
    master.tk.call('tk', 'sysnotify', title, message)
