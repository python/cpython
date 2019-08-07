# base class for tk common dialogues
#
# this module provides a base class for accessing the common
# dialogues available in Tk 4.2 and newer.  use filedialog,
# colorchooser, and messagebox to access the individual
# dialogs.
#
# written by Fredrik Lundh, May 1997
#

__all__ = ["Dialog"]

from tkinter import Frame


class Dialog:

    command = None

    def __init__(self, master=None, **options):
        self.master = master
        self.options = options
        if not master and options.get('parent'):
            self.master = options['parent']

    def _fixoptions(self):
        pass # hook

    def _fixresult(self, widget, result):
        return result # hook

    def show(self, **options):

        # update instance options
        for k, v in options.items():
            self.options[k] = v

        self._fixoptions()

        # we need a dummy widget to properly process the options
        # (at least as long as we use Tkinter 1.63)
        w = Frame(self.master)

        try:

            s = w.tk.call(self.command, *w._options(self.options))

            s = self._fixresult(w, s)

        finally:

            try:
                # get rid of the widget
                w.destroy()
            except:
                pass

        return s
