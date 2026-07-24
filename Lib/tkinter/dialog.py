# dialog.py -- Tkinter interface to the tk_dialog script.

"""Classic Tk dialog box, wrapping the tk_dialog script."""

from tkinter import _cnfmerge, Widget, TclError, Button, Pack

__all__ = ["Dialog"]

DIALOG_ICON = 'questhead'


class Dialog(Widget):
    """A modal dialog box built from the classic (non-themed) Tk widgets."""

    def __init__(self, master=None, cnf={}, **kw):
        cnf = _cnfmerge((cnf, kw))
        self.widgetName = '__dialog__'
        self._setup(master, cnf)
        self.num = self.tk.getint(
                self.tk.call(
                      'tk_dialog', self._w,
                      cnf['title'], cnf['text'],
                      cnf['bitmap'], cnf['default'],
                      *cnf['strings']))
        try: Widget.destroy(self)
        except TclError: pass

    def destroy(self):
        """Do nothing; the dialog window is already destroyed."""


def _test():
    d = Dialog(None, {'title': 'File Modified',
                      'text':
                      'File "Python.h" has been modified'
                      ' since the last time it was saved.'
                      ' Do you want to save it before'
                      ' exiting the application.',
                      'bitmap': DIALOG_ICON,
                      'default': 0,
                      'strings': ('Save File',
                                  'Discard Changes',
                                  'Return to Editor')})
    print(d.num)


if __name__ == '__main__':
    t = Button(None, {'text': 'Test',
                      'command': _test,
                      Pack: {}})
    q = Button(None, {'text': 'Quit',
                      'command': t.quit,
                      Pack: {}})
    t.mainloop()
