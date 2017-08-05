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


def _traverse_node(node, name=None):
    """Return the immediate children for a node.

    Node is the current node being traversed.  The return value is
    a tuple with the first value being a dictionary of the
    Class/Function instances of the node and the second is
    a list of tuples with (lineno, name).
    """
    items = []
    children = {}
    for key, cl in node.items():
        if name is None or cl.module == name:
            s = key
            if hasattr(cl, 'super') and cl.super:
                supers = []
                for sup in cl.super:
                    if type(sup) is type(''):
                        sname = sup
                    else:
                        sname = sup.name
                        if sup.module != cl.module:
                            sname = f'{sup.module}.{sname}'
                    supers.append(sname)
                s += '({})'.format(', '.join(supers))
            items.append((cl.lineno, s))
            children[s] = cl
    return children, items


class ClassBrowser:
    """Browse module classes and functions in IDLE.
    """

    def __init__(self, flist, name, path, _htest=False, _utest=False):
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
        if not (_htest or _utest):
            file_open = pyshell.flist.open
        self.name = name
        self.file = os.path.join(path[0], self.name + ".py")
        self._htest = _htest
        self._utest = _utest
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
        if not self._utest:
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
        """Return the list of ChildBrowserTreeItem items.

        Each item returned from listclasses is the first level of
        classes/functions within the module.
        """
        sublist = []
        for name in self.listchildren():
            item = ChildBrowserTreeItem(name, self.classes, self.file)
            sublist.append(item)
        return sublist

    def OnDoubleClick(self):
        "Open a module in an editor window when double clicked."
        if os.path.normcase(self.file[-3:]) != ".py":
            return
        if not os.path.exists(self.file):
            return
        file_open(self.file)

    def IsExpandable(self):
        "Return True if Python (.py) file."
        return os.path.normcase(self.file[-3:]) == ".py"

    def listchildren(self):
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
            tree = pyclbr.readmodule_ex(name, [dir] + sys.path)
        except ImportError:
            return []
        self.classes, items = _traverse_node(tree, name)
        items.sort()
        return [s for item, s in items]


class ChildBrowserTreeItem(TreeItem):
    """Browser tree for child nodes within the module.

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
                return not not self.cl.children
            except AttributeError:
                return False
        return None

    def GetSubList(self):
        "Return recursive list of ChildBrowserTreeItem items."
        if not self.cl:
            return []
        sublist = []
        for obj in self.listchildren():
            classes, item_name = obj
            item = ChildBrowserTreeItem(item_name, classes, self.file)
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

    def listchildren(self):
        "Return list of nested classes/functions sorted by lineno."
        if not self.cl:
            return []
        result = []
        for name, ob in self.cl.children.items():
            classes, items = _traverse_node({name: ob})
            result.append((ob.lineno, classes, items[0][1]))
        result.sort()
        return [item[1:] for item in result]


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
    from unittest import main
    main('idlelib.idle_test.test_browser', verbosity=2, exit=False)
    from idlelib.idle_test.htest import run
    run(_class_browser)
