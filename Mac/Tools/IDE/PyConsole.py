import W
import Wkeys
from Carbon import Fm
import WASTEconst
from types import *
from Carbon import Events
import string
import sys
import traceback
import MacOS
import MacPrefs
from Carbon import Qd
import EasyDialogs
import PyInteractive

if not hasattr(sys, 'ps1'):
    sys.ps1 = '>>> '
if not hasattr(sys, 'ps2'):
    sys.ps2 = '... '

def inspect(foo):                       # JJS 1/25/99
    "Launch the browser on the given object.  This is a general built-in function."
    import PyBrowser
    PyBrowser.Browser(foo)

class ConsoleTextWidget(W.EditText):

    def __init__(self, *args, **kwargs):
        apply(W.EditText.__init__, (self,) + args, kwargs)
        self._inputstart = 0
        self._buf = ''
        self.pyinteractive = PyInteractive.PyInteractive()

        import __main__
        self._namespace = __main__.__dict__
        self._namespace['inspect'] = inspect                    # JJS 1/25/99

    def insert(self, text):
        self.checkselection()
        self.ted.WEInsert(text, None, None)
        self.changed = 1
        self.selchanged = 1

    def set_namespace(self, dict):
        if type(dict) <> DictionaryType:
            raise TypeError, "The namespace needs to be a dictionary"
        if 'inspect' not in dict.keys(): dict['inspect'] = inspect                      # JJS 1/25/99
        self._namespace = dict

    def open(self):
        import __main__
        W.EditText.open(self)
        self.write('Python %s\n' % sys.version)
        self.write('Type "copyright", "credits" or "license" for more information.\n')
        self.write('MacPython IDE %s\n' % __main__.__version__)
        self.write(sys.ps1)
        self.flush()

    def key(self, char, event):
        (what, message, when, where, modifiers) = event
        if self._enabled and not modifiers & Events.cmdKey or char in Wkeys.arrowkeys:
            if char not in Wkeys.navigationkeys:
                self.checkselection()
            if char == Wkeys.enterkey:
                char = Wkeys.returnkey
            selstart, selend = self.getselection()
            if char == Wkeys.backspacekey:
                if selstart <= (self._inputstart - (selstart <> selend)):
                    return
            self.ted.WEKey(ord(char), modifiers)
            if char not in Wkeys.navigationkeys:
                self.changed = 1
            if char not in Wkeys.scrollkeys:
                self.selchanged = 1
            self.updatescrollbars()
            if char == Wkeys.returnkey:
                text = self.get()[self._inputstart:selstart]
                text = string.join(string.split(text, "\r"), "\n")
                if hasattr(MacOS, 'EnableAppswitch'):
                    saveyield = MacOS.EnableAppswitch(0)
                self._scriptDone = False
                if sys.platform == "darwin":
                    # see identical construct in PyEdit.py
                    from threading import Thread
                    t = Thread(target=self._userCancelledMonitor,
                                    name="UserCancelledMonitor")
                    t.start()
                try:
                    self.pyinteractive.executeline(text, self, self._namespace)
                finally:
                    self._scriptDone = True
                if hasattr(MacOS, 'EnableAppswitch'):
                    MacOS.EnableAppswitch(saveyield)
                selstart, selend = self.getselection()
                self._inputstart = selstart

    def _userCancelledMonitor(self):
        # XXX duplicate code from PyEdit.py
        import time, os
        from signal import SIGINT
        from Carbon import Evt
        while not self._scriptDone:
            if Evt.CheckEventQueueForUserCancel():
                # Send a SIGINT signal to ourselves.
                # This gets delivered to the main thread,
                # cancelling the running script.
                os.kill(os.getpid(), SIGINT)
                break
            time.sleep(0.25)

    def domenu_save_as(self, *args):
        filename = EasyDialogs.AskFileForSave(message='Save console text as:',
                savedFileName='console.txt')
        if not filename:
            return
        f = open(filename, 'wb')
        f.write(self.get())
        f.close()
        MacOS.SetCreatorAndType(filename, W._signature, 'TEXT')

    def write(self, text):
        self._buf = self._buf + text
        if '\n' in self._buf:
            self.flush()

    def flush(self):
        stuff = string.split(self._buf, '\n')
        stuff = string.join(stuff, '\r')
        self.setselection_at_end()
        try:
            self.ted.WEInsert(stuff, None, None)
        finally:
            self._buf = ""
        selstart, selend = self.getselection()
        self._inputstart = selstart
        self.ted.WEClearUndo()
        self.updatescrollbars()
        if self._parentwindow.wid.GetWindowPort().QDIsPortBuffered():
            self._parentwindow.wid.GetWindowPort().QDFlushPortBuffer(None)

    def selection_ok(self):
        selstart, selend = self.getselection()
        return not (selstart < self._inputstart or selend < self._inputstart)

    def checkselection(self):
        if not self.selection_ok():
            self.setselection_at_end()

    def setselection_at_end(self):
        end = self.ted.WEGetTextLength()
        self.setselection(end, end)
        self.updatescrollbars()

    def domenu_cut(self, *args):
        if not self.selection_ok():
            return
        W.EditText.domenu_cut(self)

    def domenu_paste(self, *args):
        if not self.selection_ok():
            self.setselection_at_end()
        W.EditText.domenu_paste(self)

    def domenu_clear(self, *args):
        if not self.selection_ok():
            return
        W.EditText.domenu_clear(self)


class PyConsole(W.Window):

    def __init__(self, bounds, show = 1, fontsettings = ("Monaco", 0, 9, (0, 0, 0)),
                    tabsettings = (32, 0), unclosable = 0):
        W.Window.__init__(self,
                                bounds,
                                "Python Interactive",
                                minsize = (200, 100),
                                tabbable = 0,
                                show = show)

        self._unclosable = unclosable
        consoletext = ConsoleTextWidget((-1, -1, -14, 1), inset = (6, 5),
                        fontsettings = fontsettings, tabsettings = tabsettings)
        self._bary = W.Scrollbar((-15, 14, 16, -14), consoletext.vscroll, max = 32767)
        self.consoletext = consoletext
        self.namespacemenu = W.PopupMenu((-15, -1, 16, 16), [], self.consoletext.set_namespace)
        self.namespacemenu.bind('<click>', self.makenamespacemenu)
        self.open()

    def makenamespacemenu(self, *args):
        W.SetCursor('watch')
        namespacelist = self.getnamespacelist()
        self.namespacemenu.set([("Clear window", self.clearbuffer), ("Font settings\xc9", self.dofontsettings),
                        ["Namespace"] + namespacelist, ("Browse namespace\xc9", self.browsenamespace)])
        currentname = self.consoletext._namespace["__name__"]
        for i in range(len(namespacelist)):
            if namespacelist[i][0] == currentname:
                break
        else:
            return
        # XXX this functionality should be generally available in Wmenus
        submenuid = self.namespacemenu.menu.menu.GetItemMark(3)
        menu = self.namespacemenu.menu.bar.menus[submenuid]
        menu.menu.CheckMenuItem(i + 1, 1)

    def browsenamespace(self):
        import PyBrowser, W
        W.SetCursor('watch')
        PyBrowser.Browser(self.consoletext._namespace, self.consoletext._namespace["__name__"])

    def clearbuffer(self):
        from Carbon import Res
        self.consoletext.ted.WEUseText(Res.Resource(''))
        self.consoletext.write(sys.ps1)
        self.consoletext.flush()

    def getnamespacelist(self):
        import os
        import __main__
        editors = filter(lambda x: x.__class__.__name__ == "Editor", self.parent._windows.values())

        namespaces = [ ("__main__",__main__.__dict__) ]
        for ed in editors:
            modname = os.path.splitext(ed.title)[0]
            if sys.modules.has_key(modname):
                module = sys.modules[modname]
                namespaces.append((modname, module.__dict__))
            else:
                if ed.title[-3:] == '.py':
                    modname = ed.title[:-3]
                else:
                    modname = ed.title
                ed.globals["__name__"] = modname
                namespaces.append((modname, ed.globals))
        return namespaces

    def dofontsettings(self):
        import FontSettings
        settings = FontSettings.FontDialog(self.consoletext.getfontsettings(),
                        self.consoletext.gettabsettings())
        if settings:
            fontsettings, tabsettings = settings
            self.consoletext.setfontsettings(fontsettings)
            self.consoletext.settabsettings(tabsettings)

    def show(self, onoff = 1):
        W.Window.show(self, onoff)
        if onoff:
            self.select()

    def close(self):
        if self._unclosable:
            self.show(0)
            return -1
        W.Window.close(self)

    def writeprefs(self):
        prefs = MacPrefs.GetPrefs(W.getapplication().preffilepath)
        prefs.console.show = self.isvisible()
        prefs.console.windowbounds = self.getbounds()
        prefs.console.fontsettings = self.consoletext.getfontsettings()
        prefs.console.tabsettings = self.consoletext.gettabsettings()
        prefs.save()

    def getselectedtext(self):
        return self.consoletext.getselectedtext()

class OutputTextWidget(W.EditText):

    def domenu_save_as(self, *args):
        title = self._parentwindow.gettitle()
        filename = EasyDialogs.AskFileForSave(message='Save %s text as:' % title,
                savedFileName=title + '.txt')
        if not filename:
            return
        f = open(filename, 'wb')
        f.write(self.get())
        f.close()
        MacOS.SetCreatorAndType(filename, W._signature, 'TEXT')

    def domenu_cut(self, *args):
        self.domenu_copy(*args)

    def domenu_clear(self, *args):
        self.set('')


class PyOutput:

    def __init__(self, bounds, show = 1, fontsettings = ("Monaco", 0, 9, (0, 0, 0)), tabsettings = (32, 0)):
        self.bounds = bounds
        self.fontsettings = fontsettings
        self.tabsettings = tabsettings
        self.w = None
        self.closed = 1
        self._buf = ''
        # should be able to set this
        self.savestdout, self.savestderr = sys.stdout, sys.stderr
        sys.stderr = sys.stdout = self
        if show:
            self.show()

    def setupwidgets(self):
        self.w = W.Window(self.bounds, "Output",
                        minsize = (200, 100),
                        tabbable = 0)
        self.w.outputtext = OutputTextWidget((-1, -1, -14, 1), inset = (6, 5),
                        fontsettings = self.fontsettings, tabsettings = self.tabsettings, readonly = 1)
        menuitems = [("Clear window", self.clearbuffer), ("Font settings\xc9", self.dofontsettings)]
        self.w.popupmenu = W.PopupMenu((-15, -1, 16, 16), menuitems)

        self.w._bary = W.Scrollbar((-15, 14, 16, -14), self.w.outputtext.vscroll, max = 32767)
        self.w.bind("<close>", self.close)
        self.w.bind("<activate>", self.activate)

    def write(self, text):
        if hasattr(MacOS, 'EnableAppswitch'):
            oldyield = MacOS.EnableAppswitch(-1)
        try:
            self._buf = self._buf + text
            if '\n' in self._buf:
                self.flush()
        finally:
            if hasattr(MacOS, 'EnableAppswitch'):
                MacOS.EnableAppswitch(oldyield)

    def flush(self):
        self.show()
        stuff = string.split(self._buf, '\n')
        stuff = string.join(stuff, '\r')
        end = self.w.outputtext.ted.WEGetTextLength()
        self.w.outputtext.setselection(end, end)
        self.w.outputtext.ted.WEFeatureFlag(WASTEconst.weFReadOnly, 0)
        try:
            self.w.outputtext.ted.WEInsert(stuff, None, None)
        finally:
            self._buf = ""
        self.w.outputtext.updatescrollbars()
        self.w.outputtext.ted.WEFeatureFlag(WASTEconst.weFReadOnly, 1)
        if self.w.wid.GetWindowPort().QDIsPortBuffered():
            self.w.wid.GetWindowPort().QDFlushPortBuffer(None)

    def show(self):
        if self.closed:
            if not self.w:
                self.setupwidgets()
                self.w.open()
                self.w.outputtext.updatescrollbars()
                self.closed = 0
            else:
                self.w.show(1)
                self.closed = 0
                self.w.select()

    def writeprefs(self):
        if self.w is not None:
            prefs = MacPrefs.GetPrefs(W.getapplication().preffilepath)
            prefs.output.show = self.w.isvisible()
            prefs.output.windowbounds = self.w.getbounds()
            prefs.output.fontsettings = self.w.outputtext.getfontsettings()
            prefs.output.tabsettings = self.w.outputtext.gettabsettings()
            prefs.save()

    def dofontsettings(self):
        import FontSettings
        settings = FontSettings.FontDialog(self.w.outputtext.getfontsettings(),
                        self.w.outputtext.gettabsettings())
        if settings:
            fontsettings, tabsettings = settings
            self.w.outputtext.setfontsettings(fontsettings)
            self.w.outputtext.settabsettings(tabsettings)

    def clearbuffer(self):
        from Carbon import Res
        self.w.outputtext.set('')

    def activate(self, onoff):
        if onoff:
            self.closed = 0

    def close(self):
        self.w.show(0)
        self.closed = 1
        return -1


class SimpleStdin:

    def readline(self):
        import EasyDialogs
        # A trick to make the input dialog box a bit more palatable
        if hasattr(sys.stdout, '_buf'):
            prompt = sys.stdout._buf
        else:
            prompt = ""
        if not prompt:
            prompt = "Stdin input:"
        sys.stdout.flush()
        rv = EasyDialogs.AskString(prompt)
        if rv is None:
            return ""
        rv = rv + "\n"  # readline should include line terminator
        sys.stdout.write(rv)  # echo user's reply
        return rv


def installconsole(defaultshow = 1):
    global console
    prefs = MacPrefs.GetPrefs(W.getapplication().preffilepath)
    if not prefs.console or not hasattr(prefs.console, 'show'):
        prefs.console.show = defaultshow
    if not hasattr(prefs.console, "windowbounds"):
        prefs.console.windowbounds = (450, 250)
    if not hasattr(prefs.console, "fontsettings"):
        prefs.console.fontsettings = ("Monaco", 0, 9, (0, 0, 0))
    if not hasattr(prefs.console, "tabsettings"):
        prefs.console.tabsettings = (32, 0)
    console = PyConsole(prefs.console.windowbounds, prefs.console.show,
                    prefs.console.fontsettings, prefs.console.tabsettings, 1)

def installoutput(defaultshow = 0, OutPutWindow = PyOutput):
    global output

    # quick 'n' dirty std in emulation
    sys.stdin = SimpleStdin()

    prefs = MacPrefs.GetPrefs(W.getapplication().preffilepath)
    if not prefs.output or not hasattr(prefs.output, 'show'):
        prefs.output.show = defaultshow
    if not hasattr(prefs.output, "windowbounds"):
        prefs.output.windowbounds = (450, 250)
    if not hasattr(prefs.output, "fontsettings"):
        prefs.output.fontsettings = ("Monaco", 0, 9, (0, 0, 0))
    if not hasattr(prefs.output, "tabsettings"):
        prefs.output.tabsettings = (32, 0)
    output = OutPutWindow(prefs.output.windowbounds, prefs.output.show,
                    prefs.output.fontsettings, prefs.output.tabsettings)
