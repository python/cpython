import traceback
import sys
import W
import os
import types
from Carbon import List


class TraceBack:

    def __init__(self, title = "Traceback"):
        app = W.getapplication()  # checks if W is properly initialized
        self.title = title
        self.w = None
        self.closed = 1
        self.start = 0
        self.lastwindowtitle = ""
        self.bounds = (360, 298)

    def traceback(self, start = 0, lastwindowtitle = ""):
        try:
            self.lastwindowtitle = lastwindowtitle
            self.start = start
            self.type, self.value, self.tb = sys.exc_info()
            if self.type is not SyntaxError:
                self.show()
                if type(self.type) == types.ClassType:
                    errortext = self.type.__name__
                else:
                    errortext = str(self.type)
                value = str(self.value)
                if self.value and value:
                    errortext = errortext + ": " + value
                self.w.text.set(errortext)
                self.buildtblist()
                self.w.list.set(self.textlist)
                self.w.list.setselection([len(self.textlist) - 1])
                self.w.wid.SelectWindow()
                self.closed = 0
            else:
                self.syntaxerror()
        except:
            traceback.print_exc()

    def syntaxerror(self):
        try:
            value, (filename, lineno, charno, line) = self.value
        except:
            filename = ""
            lineno = None
            value = self.value
        if not filename and self.lastwindowtitle:
            filename = self.lastwindowtitle
        elif not filename:
            filename = "<unknown>"
        if filename and os.path.exists(filename):
            filename = os.path.split(filename)[1]
        if lineno and charno is not None:
            charno = charno - 1
            text = str(value) + '\rFile: "' + str(filename) + '", line ' + str(lineno) + '\r\r' + line[:charno] + "\xa5" + line[charno:-1]
        else:
            text = str(value) + '\rFile: "' + str(filename) + '"'
        self.syntaxdialog = W.ModalDialog((360, 120), "Syntax Error")
        self.syntaxdialog.text = W.TextBox((10, 10, -10, -40), text)
        self.syntaxdialog.cancel = W.Button((-190, -32, 80, 16), "Cancel", self.syntaxclose)
        self.syntaxdialog.edit = W.Button((-100, -32, 80, 16), "Edit", self.syntaxedit)
        self.syntaxdialog.setdefaultbutton(self.syntaxdialog.edit)
        self.syntaxdialog.bind("cmd.", self.syntaxdialog.cancel.push)
        self.syntaxdialog.open()

    def syntaxclose(self):
        self.syntaxdialog.close()
        del self.syntaxdialog

    def syntaxedit(self):
        try:
            value, (filename, lineno, charno, line) = self.value
        except:
            filename = ""
            lineno = None
        if not filename and self.lastwindowtitle:
            filename = self.lastwindowtitle
        elif not filename:
            filename = "<unknown>"
        self.syntaxclose()
        if lineno:
            if charno is None:
                charno = 1
            W.getapplication().openscript(filename, lineno, charno - 1)
        else:
            W.getapplication().openscript(filename)

    def show(self):
        if self.closed:
            self.setupwidgets()
            self.w.open()
        else:
            self.w.wid.ShowWindow()
            self.w.wid.SelectWindow()

    def hide(self):
        if self.closed:
            return
        self.w.close()

    def close(self):
        self.bounds = self.w.getbounds()
        self.closed = 1
        self.type, self.value, self.tb = None, None, None
        self.tblist = None

    def activate(self, onoff):
        if onoff:
            if self.closed:
                self.traceback()
            self.closed = 0
            self.checkbuttons()

    def setupwidgets(self):
        self.w = W.Window(self.bounds, self.title, minsize = (316, 168))
        self.w.text = W.TextBox((10, 10, -10, 30))
        self.w.tbtitle = W.TextBox((10, 40, -10, 10), "Traceback (innermost last):")
        self.w.list = W.TwoLineList((10, 60, -10, -40), callback = self.listhit)

        self.w.editbutton = W.Button((10, -30, 60, 16), "Edit", self.edit)
        self.w.editbutton.enable(0)

        self.w.browselocalsbutton = W.Button((80, -30, 100, 16), "Browse locals\xc9", self.browselocals)
        self.w.browselocalsbutton.enable(0)

        self.w.postmortembutton = W.Button((190, -30, 100, 16), "Post mortem\xc9", self.postmortem)

        self.w.setdefaultbutton(self.w.editbutton)
        self.w.bind("cmdb", self.w.browselocalsbutton.push)
        self.w.bind("<close>", self.close)
        self.w.bind("<activate>", self.activate)

    def buildtblist(self):
        tb = self.tb
        for i in range(self.start):
            if tb.tb_next is None:
                break
            tb = tb.tb_next
        self.tblist = traceback.extract_tb(tb)
        self.textlist = []
        for filename, lineno, func, line in self.tblist:
            tbline = ""
            if os.path.exists(filename):
                filename = os.path.split(filename)[1]
            tbline = 'File "%s", line %r, in %r' % (filename, lineno, func)
            if line:
                tbline = tbline + '\r      ' + line
            self.textlist.append(tbline[:255])

    def edit(self):
        sel = self.w.list.getselection()
        for i in sel:
            filename, lineno, func, line = self.tblist[i]
            W.getapplication().openscript(filename, lineno)

    def browselocals(self):
        sel = self.w.list.getselection()
        for i in sel:
            tb = self.tb
            for j in range(i + self.start):
                tb = tb.tb_next
            self.browse(tb.tb_frame.f_locals)

    def browse(self, object):
        import PyBrowser
        PyBrowser.Browser(object)

    def postmortem(self):
        import PyDebugger
        PyDebugger.postmortem(self.type, self.value, self.tb)

    def listhit(self, isdbl):
        if isdbl:
            self.w.editbutton.push()
        else:
            self.checkbuttons()

    def checkbuttons(self):
        havefile = len(self.w.list.getselection()) > 0
        self.w.editbutton.enable(havefile)
        self.w.browselocalsbutton.enable(havefile)
        self.w.setdefaultbutton(havefile and self.w.editbutton or self.w.postmortembutton)
