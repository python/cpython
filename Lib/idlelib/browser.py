"""Class browser.

XXX TO DO:

- reparse when source changed (maybe just a button would be OK?)
    (or recheck on window popup)
- add popup menu with more options (e.g. doc strings, base classes, imports)
- show function argument list? (have to do pattern matching on source)
- should the classes and methods lists also be in the module's menu bar?
- add base classes to class browser tree
"""

import os
import pyclbr
import sys

from idlelib.config import idleConf
from idlelib import pyshell
from idlelib.tree import TreeNode, TreeItem, ScrolledCanvas
from idlelib.windows import ListedToplevel

file_open = None  # Method...Item and Class...Item use this.
# Normally pyshell.flist.open, but there is no pyshell.flist for htest.

class ClassBrowser:
    """Browse module classes and functions in IDLE.
    """

    def __init__(self, flist, name, path, _htest=False):
        # XXX This API should change, if the file doesn't end in ".py"
        # XXX the code here is bogus!
        """Create a window for browsing a module's structure.

        Args:
            flist: filelist.FileList instance used as the root for the window.
            name: Python module to parse.
            path: Module search path.
            _htest - bool, change box when location running htest.

        Global variables:
            file_open: Function used for opening a file.

        Instance variables:
            name: Module name.
            file: Full path and module with .py extension.  Used in
                creating ModuleBrowserTreeItem as the rootnode for
                the tree and subsequently in the children.
        """
        global file_open
        if not _htest:
            file_open = pyshell.flist.open
        self.name = name
        self.file = os.path.join(path[0], self.name + ".py")
        self._htest = _htest
        self.init(flist)

    def close(self, event=None):
        "Dismiss the window and the tree nodes."
        self.top.destroy()
        self.node.destroy()

    def init(self, flist):
        "Create browser tkinter widgets, including the tree."
        self.flist = flist
        # reset pyclbr
        pyclbr._modules.clear()
        # create top
        self.top = top = ListedToplevel(flist.root)
        top.protocol("WM_DELETE_WINDOW", self.close)
        top.bind("<Escape>", self.close)
        if self._htest: # place dialog below parent if running htest
            top.geometry("+%d+%d" %
                (flist.root.winfo_rootx(), flist.root.winfo_rooty() + 200))
        self.settitle()
        top.focus_set()
        # create scrolled canvas
        theme = idleConf.CurrentTheme()
        background = idleConf.GetHighlight(theme, 'normal')['background']
        sc = ScrolledCanvas(top, bg=background, highlightthickness=0, takefocus=1)
        sc.frame.pack(expand=1, fill="both")
        item = self.rootnode()
        self.node = node = TreeNode(sc.canvas, None, item)
        node.update()
        node.expand()

    def settitle(self):
        "Set the window title."
        self.top.wm_title("Class Browser - " + self.name)
        self.top.wm_iconname("Class Browser")

    def rootnode(self):
        "Return a ModuleBrowserTreeItem as the root of the tree."
        return ModuleBrowserTreeItem(self.file)

class ModuleBrowserTreeItem(TreeItem):
    """Browser tree for Python module.

    Uses TreeItem as the basis for the structure of the tree.
    """

    def __init__(self, file):
        """Create a TreeItem for the file.

        Args:
            file: Full path and module name.
        """
        self.file = file

    def GetText(self):
        "Return the module name as the text string to display."
        return os.path.basename(self.file)

    def GetIconName(self):
        "Return the name of the icon to display."
        return "python"

    def GetSubList(self):
        """Return the list of ClassBrowserTreeItem items.

        Each item returned from listclasses is the first level of
        classes/functions within the module.
        """
        sublist = []
        for name in self.listclasses():
            item = ClassBrowserTreeItem(name, self.classes, self.file)
            sublist.append(item)
        return sublist

    def OnDoubleClick(self):
        "Open a module in an editor window when double clicked."
        if os.path.normcase(self.file[-3:]) != ".py":
            return
        if not os.path.exists(self.file):
            return
        pyshell.flist.open(self.file)

    def IsExpandable(self):
        "Return True if Python (.py) file."
        return os.path.normcase(self.file[-3:]) == ".py"

    def listclasses(self):
        """Return list of classes and functions in the module.

        The dictionary output from pyclbr is re-written as a
        list of tuples in the form (lineno, name) and
        then sorted so that the classes and functions are
        processed in line number order.  The returned list only
        contains the name and not the line number.  An instance
        variable self.classes contains the pyclbr dictionary values,
        which are instances of Class and Function.
        """
        dir, file = os.path.split(self.file)
        name, ext = os.path.splitext(file)
        if os.path.normcase(ext) != ".py":
            return []
        try:
            dict = pyclbr.readmodule_ex(name, [dir] + sys.path)
        except ImportError:
            return []
        items = []
        self.classes = {}
        for key, cl in dict.items():
            if cl.module == name:
                s = key
                if hasattr(cl, 'super') and cl.super:
                    supers = []
                    for sup in cl.super:
                        if type(sup) is type(''):
                            sname = sup
                        else:
                            sname = sup.name
                            if sup.module != cl.module:
                                sname = "%s.%s" % (sup.module, sname)
                        supers.append(sname)
                    s = s + "(%s)" % ", ".join(supers)
                items.append((cl.lineno, s))
                self.classes[s] = cl
        items.sort()
        list = []
        for item, s in items:
            list.append(s)
        return list

class ClassBrowserTreeItem(TreeItem):
    """Browser tree for classes within a module.

    Uses TreeItem as the basis for the structure of the tree.
    """

    def __init__(self, name, classes, file):
        """Create a TreeItem for the class/function.

        Args:
            name: Name of the class/function.
            classes: Dictonary of Class/Function instances from pyclbr.
            file: Full path and module name.

        Instance variables:
            self.cl: Class/Function instance for the class/function name.
            self.isfunction: True if self.cl is a Function.
        """
        self.name = name
        # XXX - Does classes need to be an instance variable?
        self.classes = classes
        self.file = file
        try:
            self.cl = self.classes[self.name]
        except (IndexError, KeyError):
            self.cl = None
        self.isfunction = isinstance(self.cl, pyclbr.Function)

    def GetText(self):
        "Return the name of the function/class to display."
        if self.isfunction:
            return "def " + self.name + "(...)"
        else:
            return "class " + self.name

    def GetIconName(self):
        "Return the name of the icon to display."
        if self.isfunction:
            return "python"
        else:
            return "folder"

    def IsExpandable(self):
        "Return True if this class has methods."
        if self.cl:
            try:
                return not not self.cl.methods
            except AttributeError:
                return False
        return None

    def GetSubList(self):
        """Return Class methods as a list of MethodBrowserTreeItem items.

        Each item is a method within the class.
        """
        if not self.cl:
            return []
        sublist = []
        for name in self.listmethods():
            item = MethodBrowserTreeItem(name, self.cl, self.file)
            sublist.append(item)
        return sublist

    def OnDoubleClick(self):
        "Open module with file_open and position to lineno, if it exists."
        if not os.path.exists(self.file):
            return
        edit = file_open(self.file)
        if hasattr(self.cl, 'lineno'):
            lineno = self.cl.lineno
            edit.gotoline(lineno)

    def listmethods(self):
        "Return list of methods within a class sorted by lineno."
        if not self.cl:
            return []
        items = []
        for name, lineno in self.cl.methods.items():
            items.append((lineno, name))
        items.sort()
        list = []
        for item, name in items:
            list.append(name)
        return list

class MethodBrowserTreeItem(TreeItem):
    """Browser tree for methods within a class.

    Uses TreeItem as the basis for the structure of the tree.
    """

    def __init__(self, name, cl, file):
        """Create a TreeItem for the methods.

        Args:
            name: Name of the class/function.
            cl: pyclbr.Class instance for name.
            file: Full path and module name.
        """
        self.name = name
        self.cl = cl
        self.file = file

    def GetText(self):
        "Return the method name to display."
        return "def " + self.name + "(...)"

    def GetIconName(self):
        "Return the name of the icon to display."
        return "python"

    def IsExpandable(self):
        "Return False as there are no tree items after methods."
        return False

    def OnDoubleClick(self):
        "Open module with file_open and position at the method start."
        if not os.path.exists(self.file):
            return
        edit = file_open(self.file)
        edit.gotoline(self.cl.methods[self.name])

def _class_browser(parent): # htest #
    try:
        file = __file__
    except NameError:
        file = sys.argv[0]
        if sys.argv[1:]:
            file = sys.argv[1]
        else:
            file = sys.argv[0]
    dir, file = os.path.split(file)
    name = os.path.splitext(file)[0]
    flist = pyshell.PyShellFileList(parent)
    global file_open
    file_open = flist.open
    ClassBrowser(flist, name, [dir], _htest=True)

if __name__ == "__main__":
    from idlelib.idle_test.htest import run
    run(_class_browser)
