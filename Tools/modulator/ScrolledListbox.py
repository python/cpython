# A ScrolledList widget feels like a list widget but also has a
# vertical scroll bar on its right.  (Later, options may be added to
# add a horizontal bar as well, to make the bars disappear
# automatically when not needed, to move them to the other side of the
# window, etc.)
#
# Configuration options are passed to the List widget.
# A Frame widget is inserted between the master and the list, to hold
# the Scrollbar widget.
# Most methods calls are inherited from the List widget; Pack methods
# are redirected to the Frame widget however.

from Tkinter import *
from Tkinter import _cnfmerge

class ScrolledListbox(Listbox):
    def __init__(self, master=None, cnf={}):
        cnf = _cnfmerge(cnf)
        fcnf = {}
        vcnf = {'name': 'vbar',
            Pack: {'side': 'right', 'fill': 'y'},}
        for k in cnf.keys():
            if type(k) == ClassType or k == 'name':
                fcnf[k] = cnf[k]
                del cnf[k]
        self.frame = Frame(master, fcnf)
        self.vbar = Scrollbar(self.frame, vcnf)
        cnf[Pack] = {'side': 'left', 'fill': 'both', 'expand': 'yes'}
        cnf['name'] = 'list'
        Listbox.__init__(self, self.frame, cnf)
        self['yscrollcommand'] = (self.vbar, 'set')
        self.vbar['command'] = (self, 'yview')

        # Copy Pack methods of self.frame -- hack!
        for m in Pack.__dict__.keys():
            if m[0] != '_' and m != 'config':
                setattr(self, m, getattr(self.frame, m))
