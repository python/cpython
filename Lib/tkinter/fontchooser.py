"""Interface to the native font selection dialog.

The FontChooser class gives access to the "tk fontchooser" command.
The dialog is application-global: all instances configure the single
dialog of the Tcl interpreter.

The dialog may be modeless, so show() may return immediately.
"""

import tkinter
from tkinter.font import Font

__all__ = ["FontChooser"]


class FontChooser:
    """The font selection dialog.

    Supported configuration options are:

        parent: the window to which the dialog and its virtual
                events are related (the main window by default)
        title: the title of the dialog
        font: the font that is currently selected in the dialog
        command: a callback that is called with a tkinter.font.Font
                 object wrapping the selected font when the user
                 selects a font
        visible: whether the dialog is currently displayed
                 (read-only)

    The "font" option accepts the forms supported by
    tkinter.font.Font(font=...).

    The "command" callback is the only reliable way to obtain the
    selected font.  On some platforms the "font" option is not updated
    to the user's choice.
    """

    def __init__(self, master=None, **options):
        if master is None:
            master = options.get('parent')
        if master is None:
            master = tkinter._get_default_root('create a font chooser')
        self.master = master
        self._command_name = None
        if options:
            self.configure(options)

    def configure(self, cnf=None, **kw):
        """Query or modify the options of the font dialog.

        With no arguments, return a dict of all option values.  With a
        string argument, return the value of that option.  Otherwise,
        set the given options.  The "visible" option is read-only.
        """
        if kw:
            cnf = tkinter._cnfmerge((cnf, kw))
        elif cnf:
            cnf = tkinter._cnfmerge(cnf)
        master = self.master
        if cnf is None:
            items = master.tk.splitlist(master.tk.call(
                    'tk', 'fontchooser', 'configure'))
            return {items[i][1:]: items[i+1] for i in range(0, len(items), 2)}
        if isinstance(cnf, str):
            return master.tk.call('tk', 'fontchooser', 'configure', '-' + cnf)
        cnf = dict(cnf)
        new_name = None
        if 'command' in cnf:
            command = cnf['command']
            if command is None:
                cnf['command'] = ''
            elif callable(command):
                # Pass the selected font to the callback as a Font object.
                def callback(description):
                    command(Font(self.master, font=description, exists=True))
                # Name the Tcl command after the callback.
                callback.__func__ = command
                new_name = cnf['command'] = master._register(callback)
        try:
            master.tk.call('tk', 'fontchooser', 'configure',
                           *master._options(cnf))
        except tkinter.TclError:
            if new_name is not None:
                master.deletecommand(new_name)
            raise
        if 'command' in cnf:
            if self._command_name is not None:
                master.deletecommand(self._command_name)
            self._command_name = new_name
    config = configure

    def cget(self, option):
        """Return the value of the given option of the font dialog."""
        return self.master.tk.call('tk', 'fontchooser', 'configure',
                                   '-' + option)

    def show(self):
        """Display the font dialog.

        Depending on the platform, may return immediately or only
        once the dialog has been withdrawn.
        """
        self.master.tk.call('tk', 'fontchooser', 'show')

    def hide(self):
        """Hide the font dialog if it is displayed."""
        self.master.tk.call('tk', 'fontchooser', 'hide')
