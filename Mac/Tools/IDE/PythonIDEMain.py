# copyright 1997-2001 Just van Rossum, Letterror. just@letterror.com

import Splash

import FrameWork
import Wapplication
import W
import os
import sys
import MacOS
import EasyDialogs
from Carbon import File
from Carbon import Files

if MacOS.runtimemodel == 'macho':
    ELLIPSIS = '...'
else:
    ELLIPSIS = '\xc9'

def runningOnOSX():
    from gestalt import gestalt
    gestaltMenuMgrAquaLayoutBit = 1  # menus have the Aqua 1.0 layout
    gestaltMenuMgrAquaLayoutMask = (1L << gestaltMenuMgrAquaLayoutBit)
    value = gestalt("menu") & gestaltMenuMgrAquaLayoutMask
    return not not value

def getmodtime(file):
    file = File.FSRef(file)
    catinfo, d1, d2, d3 = file.FSGetCatalogInfo(Files.kFSCatInfoContentMod)
    return catinfo.contentModDate

class PythonIDE(Wapplication.Application):

    def __init__(self):
        if sys.platform == "darwin":
            if len(sys.argv) > 1 and sys.argv[1].startswith("-psn"):
                home = os.getenv("HOME")
                if home:
                    os.chdir(home)
        self.preffilepath = os.path.join("Python", "PythonIDE preferences")
        Wapplication.Application.__init__(self, 'Pide')
        from Carbon import AE
        from Carbon import AppleEvents

        AE.AEInstallEventHandler(AppleEvents.kCoreEventClass, AppleEvents.kAEOpenApplication,
                        self.ignoreevent)
        AE.AEInstallEventHandler(AppleEvents.kCoreEventClass, AppleEvents.kAEReopenApplication,
                        self.ignoreevent)
        AE.AEInstallEventHandler(AppleEvents.kCoreEventClass, AppleEvents.kAEPrintDocuments,
                        self.ignoreevent)
        AE.AEInstallEventHandler(AppleEvents.kCoreEventClass, AppleEvents.kAEOpenDocuments,
                        self.opendocsevent)
        AE.AEInstallEventHandler(AppleEvents.kCoreEventClass, AppleEvents.kAEQuitApplication,
                        self.quitevent)
        import PyConsole, PyEdit
        Splash.wait()
        # With -D option (OSX command line only) keep stderr, for debugging the IDE
        # itself.
        debug_stderr = None
        if len(sys.argv) >= 2 and sys.argv[1] == '-D':
            debug_stderr = sys.stderr
            del sys.argv[1]
        PyConsole.installoutput()
        PyConsole.installconsole()
        if debug_stderr:
            sys.stderr = debug_stderr
        for path in sys.argv[1:]:
            if path.startswith("-p"):
                # process number added by the OS
                continue
            self.opendoc(path)
        self.mainloop()

    def makeusermenus(self):
        m = Wapplication.Menu(self.menubar, "File")
        newitem = FrameWork.MenuItem(m, "New", "N", 'new')
        openitem = FrameWork.MenuItem(m, "Open"+ELLIPSIS, "O", 'open')
        openbynameitem = FrameWork.MenuItem(m, "Open File by Name"+ELLIPSIS, "D", 'openbyname')
        self.openrecentmenu = FrameWork.SubMenu(m, "Open Recent")
        self.makeopenrecentmenu()
        FrameWork.Separator(m)
        closeitem = FrameWork.MenuItem(m, "Close", "W", 'close')
        saveitem = FrameWork.MenuItem(m, "Save", "S", 'save')
        saveasitem = FrameWork.MenuItem(m, "Save as"+ELLIPSIS, None, 'save_as')
        FrameWork.Separator(m)
        saveasappletitem = FrameWork.MenuItem(m, "Save as Applet"+ELLIPSIS, None, 'save_as_applet')
        FrameWork.Separator(m)
        instmgritem = FrameWork.MenuItem(m, "Package Manager", None, 'openpackagemanager')
        gensuiteitem = FrameWork.MenuItem(m, "Generate OSA Suite...", None, 'gensuite')
        if not runningOnOSX():
            # On OSX there's a special "magic" quit menu, so we shouldn't add
            # it to the File menu.
            FrameWork.Separator(m)
            quititem = FrameWork.MenuItem(m, "Quit", "Q", 'quit')

        m = Wapplication.Menu(self.menubar, "Edit")
        undoitem = FrameWork.MenuItem(m, "Undo", 'Z', "undo")
        FrameWork.Separator(m)
        cutitem = FrameWork.MenuItem(m, "Cut", 'X', "cut")
        copyitem = FrameWork.MenuItem(m, "Copy", "C", "copy")
        pasteitem = FrameWork.MenuItem(m, "Paste", "V", "paste")
        FrameWork.MenuItem(m, "Clear", None,  "clear")
        FrameWork.Separator(m)
        selallitem = FrameWork.MenuItem(m, "Select all", "A", "selectall")
        sellineitem = FrameWork.MenuItem(m, "Select line", "L", "selectline")
        FrameWork.Separator(m)
        finditem = FrameWork.MenuItem(m, "Find"+ELLIPSIS, "F", "find")
        findagainitem = FrameWork.MenuItem(m, "Find again", 'G', "findnext")
        enterselitem = FrameWork.MenuItem(m, "Enter search string", "E", "entersearchstring")
        replaceitem = FrameWork.MenuItem(m, "Replace", None, "replace")
        replacefinditem = FrameWork.MenuItem(m, "Replace & find again", 'T', "replacefind")
        FrameWork.Separator(m)
        shiftleftitem = FrameWork.MenuItem(m, "Shift left", "[", "shiftleft")
        shiftrightitem = FrameWork.MenuItem(m, "Shift right", "]", "shiftright")

        m = Wapplication.Menu(self.menubar, "Python")
        runitem = FrameWork.MenuItem(m, "Run window", "R", 'run')
        runselitem = FrameWork.MenuItem(m, "Run selection", None, 'runselection')
        FrameWork.Separator(m)
        moditem = FrameWork.MenuItem(m, "Module browser"+ELLIPSIS, "M", self.domenu_modulebrowser)
        FrameWork.Separator(m)
        mm = FrameWork.SubMenu(m, "Preferences")
        FrameWork.MenuItem(mm, "Set Scripts folder"+ELLIPSIS, None, self.do_setscriptsfolder)
        FrameWork.MenuItem(mm, "Editor default settings"+ELLIPSIS, None, self.do_editorprefs)
        FrameWork.MenuItem(mm, "Set default window font"+ELLIPSIS, None, self.do_setwindowfont)

        self.openwindowsmenu = Wapplication.Menu(self.menubar, 'Windows')
        self.makeopenwindowsmenu()
        self._menustocheck = [closeitem, saveitem, saveasitem, saveasappletitem,
                        undoitem, cutitem, copyitem, pasteitem,
                        selallitem, sellineitem,
                        finditem, findagainitem, enterselitem, replaceitem, replacefinditem,
                        shiftleftitem, shiftrightitem,
                        runitem, runselitem]

        prefs = self.getprefs()
        try:
            fsr, d = File.Alias(rawdata=prefs.scriptsfolder).FSResolveAlias(None)
            self.scriptsfolder = fsr.FSNewAliasMinimal()
        except:
            path = os.path.join(os.getcwd(), "Mac", "IDE scripts")
            if not os.path.exists(path):
                if sys.platform == "darwin":
                    path = os.path.join(os.getenv("HOME"), "Library", "Python", "IDE-Scripts")
                else:
                    path = os.path.join(os.getcwd(), "Scripts")
                if not os.path.exists(path):
                    os.makedirs(path)
                    f = open(os.path.join(path, "Place your scripts here"+ELLIPSIS), "w")
                    f.close()
            fsr = File.FSRef(path)
            self.scriptsfolder = fsr.FSNewAliasMinimal()
            self.scriptsfoldermodtime = getmodtime(fsr)
        else:
            self.scriptsfoldermodtime = getmodtime(fsr)
        prefs.scriptsfolder = self.scriptsfolder.data
        self._scripts = {}
        self.scriptsmenu = None
        self.makescriptsmenu()
        self.makehelpmenu()

    def quitevent(self, theAppleEvent, theReply):
        self._quit()

    def suspendresume(self, onoff):
        if onoff:
            fsr, changed = self.scriptsfolder.FSResolveAlias(None)
            modtime = getmodtime(fsr)
            if self.scriptsfoldermodtime <> modtime or changed:
                self.scriptsfoldermodtime = modtime
                W.SetCursor('watch')
                self.makescriptsmenu()

    def ignoreevent(self, theAppleEvent, theReply):
        pass

    def opendocsevent(self, theAppleEvent, theReply):
        W.SetCursor('watch')
        import aetools
        parameters, args = aetools.unpackevent(theAppleEvent)
        docs = parameters['----']
        if type(docs) <> type([]):
            docs = [docs]
        for doc in docs:
            fsr, a = doc.FSResolveAlias(None)
            path = fsr.as_pathname()
            self.opendoc(path)

    def opendoc(self, path):
        fcreator, ftype = MacOS.GetCreatorAndType(path)
        if ftype == 'TEXT':
            self.openscript(path)
        elif ftype == '\0\0\0\0' and path[-3:] == '.py':
            self.openscript(path)
        else:
            W.Message("Can't open file of type '%s'." % ftype)

    def getabouttext(self):
        return "About Python IDE"+ELLIPSIS

    def do_about(self, id, item, window, event):
        Splash.about()

    def do_setscriptsfolder(self, *args):
        fsr = EasyDialogs.AskFolder(message="Select Scripts Folder",
                wanted=File.FSRef)
        if fsr:
            prefs = self.getprefs()
            alis = fsr.FSNewAliasMinimal()
            prefs.scriptsfolder = alis.data
            self.scriptsfolder = alis
            self.makescriptsmenu()
            prefs.save()

    def domenu_modulebrowser(self, *args):
        W.SetCursor('watch')
        import ModuleBrowser
        ModuleBrowser.ModuleBrowser()

    def domenu_open(self, *args):
        filename = EasyDialogs.AskFileForOpen(typeList=("TEXT",))
        if filename:
            self.openscript(filename)

    def domenu_openbyname(self, *args):
        # Open a file by name. If the clipboard contains a filename
        # use that as the default.
        from Carbon import Scrap
        try:
            sc = Scrap.GetCurrentScrap()
            dft = sc.GetScrapFlavorData("TEXT")
        except Scrap.Error:
            dft = ""
        else:
            if not os.path.exists(dft):
                dft = ""
        filename = EasyDialogs.AskString("Open File Named:", default=dft, ok="Open")
        if filename:
            self.openscript(filename)

    def domenu_new(self, *args):
        W.SetCursor('watch')
        import PyEdit
        return PyEdit.Editor()

    def makescriptsmenu(self):
        W.SetCursor('watch')
        if self._scripts:
            for id, item in self._scripts.keys():
                if self.menubar.menus.has_key(id):
                    m = self.menubar.menus[id]
                    m.delete()
            self._scripts = {}
        if self.scriptsmenu:
            if hasattr(self.scriptsmenu, 'id') and self.menubar.menus.has_key(self.scriptsmenu.id):
                self.scriptsmenu.delete()
        self.scriptsmenu = FrameWork.Menu(self.menubar, "Scripts")
        #FrameWork.MenuItem(self.scriptsmenu, "New script", None, self.domenu_new)
        #self.scriptsmenu.addseparator()
        fsr, d1 = self.scriptsfolder.FSResolveAlias(None)
        self.scriptswalk(fsr.as_pathname(), self.scriptsmenu)

    def makeopenwindowsmenu(self):
        for i in range(len(self.openwindowsmenu.items)):
            self.openwindowsmenu.menu.DeleteMenuItem(1)
            self.openwindowsmenu.items = []
        windows = []
        self._openwindows = {}
        for window in self._windows.keys():
            title = window.GetWTitle()
            if not title:
                title = "<no title>"
            windows.append((title, window))
        windows.sort()
        for title, window in windows:
            if title == "Python Interactive":       # ugly but useful hack by Joe Strout
                shortcut = '0'
            else:
                shortcut = None
            item = FrameWork.MenuItem(self.openwindowsmenu, title, shortcut, callback = self.domenu_openwindows)
            self._openwindows[item.item] = window
        self._openwindowscheckmark = 0
        self.checkopenwindowsmenu()

    def makeopenrecentmenu(self):
        for i in range(len(self.openrecentmenu.items)):
            self.openrecentmenu.menu.DeleteMenuItem(1)
            self.openrecentmenu.items = []
        prefs = self.getprefs()
        filelist = prefs.recentfiles
        if not filelist:
            self.openrecentmenu.enable(0)
            return
        self.openrecentmenu.enable(1)
        for filename in filelist:
            item = FrameWork.MenuItem(self.openrecentmenu, filename, None, callback = self.domenu_openrecent)

    def addrecentfile(self, file):
        prefs = self.getprefs()
        filelist = prefs.recentfiles
        if not filelist:
            filelist = []

        if file in filelist:
            if file == filelist[0]:
                return
            filelist.remove(file)
        filelist.insert(0, file)
        filelist = filelist[:10]
        prefs.recentfiles = filelist
        prefs.save()
        self.makeopenrecentmenu()

    def domenu_openwindows(self, id, item, window, event):
        w = self._openwindows[item]
        w.ShowWindow()
        w.SelectWindow()

    def domenu_openrecent(self, id, item, window, event):
        prefs = self.getprefs()
        filelist = prefs.recentfiles
        if not filelist:
            filelist = []
        item = item - 1
        filename = filelist[item]
        self.openscript(filename)

    def domenu_quit(self):
        self._quit()

    def domenu_save(self, *args):
        print "Save"

    def _quit(self):
        import PyConsole, PyEdit
        for window in self._windows.values():
            try:
                rv = window.close() # ignore any errors while quitting
            except:
                rv = 0   # (otherwise, we can get stuck!)
            if rv and rv > 0:
                return
        try:
            PyConsole.console.writeprefs()
            PyConsole.output.writeprefs()
            PyEdit.searchengine.writeprefs()
        except:
            # Write to __stderr__ so the msg end up in Console.app and has
            # at least _some_ chance of getting read...
            # But: this is a workaround for way more serious problems with
            # the Python 2.2 Jaguar addon.
            sys.__stderr__.write("*** PythonIDE: Can't write preferences ***\n")
        self.quitting = 1

    def domenu_openpackagemanager(self):
        import PackageManager
        PackageManager.PackageBrowser()

    def domenu_gensuite(self):
        import gensuitemodule
        gensuitemodule.main_interactive()

    def makehelpmenu(self):
        hashelp, hasdocs = self.installdocumentation()
        self.helpmenu = m = self.gethelpmenu()
        helpitem = FrameWork.MenuItem(m, "MacPython Help", None, self.domenu_localhelp)
        helpitem.enable(hashelp)
        docitem = FrameWork.MenuItem(m, "Python Documentation", None, self.domenu_localdocs)
        docitem.enable(hasdocs)
        finditem = FrameWork.MenuItem(m, "Lookup in Python Documentation", None, 'lookuppython')
        finditem.enable(hasdocs)
        if runningOnOSX():
            FrameWork.Separator(m)
            doc2item = FrameWork.MenuItem(m, "Apple Developer Documentation", None, self.domenu_appledocs)
            find2item = FrameWork.MenuItem(m, "Lookup in Carbon Documentation", None, 'lookupcarbon')
        FrameWork.Separator(m)
        webitem = FrameWork.MenuItem(m, "Python Documentation on the Web", None, self.domenu_webdocs)
        web2item = FrameWork.MenuItem(m, "Python on the Web", None, self.domenu_webpython)
        web3item = FrameWork.MenuItem(m, "MacPython on the Web", None, self.domenu_webmacpython)

    def domenu_localdocs(self, *args):
        from Carbon import AH
        AH.AHGotoPage("Python Documentation", None, None)

    def domenu_localhelp(self, *args):
        from Carbon import AH
        AH.AHGotoPage("MacPython Help", None, None)

    def domenu_appledocs(self, *args):
        from Carbon import AH, AppleHelp
        try:
            AH.AHGotoMainTOC(AppleHelp.kAHTOCTypeDeveloper)
        except AH.Error, arg:
            if arg[0] == -50:
                W.Message("Developer documentation not installed")
            else:
                W.Message("AppleHelp Error: %r" % (arg,))

    def domenu_lookuppython(self, *args):
        from Carbon import AH
        searchstring = self._getsearchstring()
        if not searchstring:
            return
        try:
            AH.AHSearch("Python Documentation", searchstring)
        except AH.Error, arg:
            W.Message("AppleHelp Error: %r" % (arg,))

    def domenu_lookupcarbon(self, *args):
        from Carbon import AH
        searchstring = self._getsearchstring()
        if not searchstring:
            return
        try:
            AH.AHSearch("Carbon", searchstring)
        except AH.Error, arg:
            W.Message("AppleHelp Error: %r" % (arg,))

    def _getsearchstring(self):
        # First we get the frontmost window
        front = self.getfrontwindow()
        if front and hasattr(front, 'getselectedtext'):
            text = front.getselectedtext()
            if text:
                return text
        # This is a cop-out. We should have disabled the menus
        # if there is no selection, but the can_ methods only seem
        # to work for Windows. Or not for the Help menu, maybe?
        text = EasyDialogs.AskString("Search documentation for", ok="Search")
        return text

    def domenu_webdocs(self, *args):
        import webbrowser
        major, minor, micro, state, nano = sys.version_info
        if state in ('alpha', 'beta'):
            docversion = 'dev/doc/devel'
        elif micro == 0:
            docversion = 'doc/%d.%d' % (major, minor)
        else:
            docversion = 'doc/%d.%d.%d' % (major, minor, micro)
        webbrowser.open("http://www.python.org/%s" % docversion)

    def domenu_webpython(self, *args):
        import webbrowser
        webbrowser.open("http://www.python.org/")

    def domenu_webmacpython(self, *args):
        import webbrowser
        webbrowser.open("http://www.cwi.nl/~jack/macpython.html")

    def installdocumentation(self):
        # This is rather much of a hack. Someone has to tell the Help Viewer
        # about the Python documentation, so why not us. The documentation
        # is located in the framework, but there's a symlink in Python.app.
        # And as AHRegisterHelpBook wants a bundle (with the right bits in
        # the plist file) we refer it to Python.app
        #
        # To make matters worse we have to look in two places: first in the IDE
        # itself, then in the Python application inside the framework.
        has_help = False
        has_doc = False
        ide_path_components = sys.argv[0].split("/")
        if ide_path_components[-3:] == ["Contents", "Resources", "PythonIDE.py"]:
            ide_app = "/".join(ide_path_components[:-3])
            help_source = os.path.join(ide_app, 'Contents/Resources/English.lproj/Documentation')
            doc_source = os.path.join(ide_app, 'Contents/Resources/English.lproj/PythonDocumentation')
            has_help = os.path.isdir(help_source)
            has_doc = os.path.isdir(doc_source)
            if has_help or has_doc:
                try:
                    from Carbon import AH
                    AH.AHRegisterHelpBook(ide_app)
                except (ImportError, MacOS.Error), arg:
                    pass # W.Message("Cannot register Python Documentation: %s" % str(arg))
        python_app = os.path.join(sys.prefix, 'Resources/Python.app')
        if not has_help:
            help_source = os.path.join(python_app, 'Contents/Resources/English.lproj/Documentation')
            has_help = os.path.isdir(help_source)
        if not has_doc:
            doc_source = os.path.join(python_app, 'Contents/Resources/English.lproj/PythonDocumentation')
            has_doc = os.path.isdir(doc_source)
        if has_help or has_doc:
            try:
                from Carbon import AH
                AH.AHRegisterHelpBook(python_app)
            except (ImportError, MacOS.Error), arg:
                pass # W.Message("Cannot register Python Documentation: %s" % str(arg))
        return has_help, has_doc
