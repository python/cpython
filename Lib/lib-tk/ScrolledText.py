# A ScrolledText widget feels like a text widget but also has a
# vertical scroll bar on its right.  (Later, options may be added to
# add a horizontal bar as well, to make the bars disappear
# automatically when not needed, to move them to the other side of the
# window, etc.)
#
# Configuration options are passed to the Text widget.
# A Frame widget is inserted between the master and the text, to hold
# the Scrollbar widget.
# Most methods calls are inherited from the Text widget; Pack methods
# are redirected to the Frame widget however.

from Tkinter import *
from Tkinter import _cnfmerge

class ScrolledText(Text):
    def __init__(self, master=None, cnf=None, **kw):
        if cnf is None:
            cnf = {}
        if kw:
            cnf = _cnfmerge((cnf, kw))
        fcnf = {}
        for k in cnf.keys():
            if type(k) == ClassType or k == 'name':
                fcnf[k] = cnf[k]
                del cnf[k]
        self.frame = Frame(master, **fcnf)
        self.vbar = Scrollbar(self.frame, name='vbar')
        self.vbar.pack(side=RIGHT, fill=Y)
        cnf['name'] = 'text'
        Text.__init__(self, self.frame, **cnf)
        self.pack(side=LEFT, fill=BOTH, expand=1)
        self['yscrollcommand'] = self.vbar.set
        self.vbar['command'] = self.yview

        # Copy geometry methods of self.frame -- hack!
        methods = Pack.__dict__.keys()
        methods = methods + Grid.__dict__.keys()
        methods = methods + Place.__dict__.keys()

        for m in methods:
            if m[0] != '_' and m != 'config' and m != 'configure':
                setattr(self, m, getattr(self.frame, m))
