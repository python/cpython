"""Class browser.

XXX TO DO:

- reparse when source changed (maybe just a button would be OK?)
    (or recheck on window popup)
- add popup menu with more options (e.g. doc strings, base classes, imports)
- show function argument list? (have to do pattern matching on source)
- should the classes and methods lists also be in the module's menu bar?
- add base classes to class browser tree
- make methodless classes inexpandable
- make classless modules inexpandable


"""

import os
import sys
import string
import pyclbr

import PyShell
from WindowList import ListedToplevel
from TreeWidget import TreeNode, TreeItem, ScrolledCanvas

class ClassBrowser:

    def __init__(self, flist, name, path):
        self.name = name
        self.file = os.path.join(path[0], self.name + ".py")
        self.init(flist)

    def close(self, event=None):
        self.top.destroy()

    def init(self, flist):
        self.flist = flist
        # reset pyclbr
        pyclbr._modules.clear()
        # create top
        self.top = top = ListedToplevel(flist.root)
        top.protocol("WM_DELETE_WINDOW", self.close)
        top.bind("<Escape>", self.close)
        self.settitle()
        top.focus_set()
        # create scrolled canvas
        sc = ScrolledCanvas(top, bg="white", highlightthickness=0, takefocus=1)
        sc.frame.pack(expand=1, fill="both")
        item = self.rootnode()
        node = TreeNode(sc.canvas, None, item)
        node.update()
        node.expand()

    def settitle(self):
        self.top.wm_title("Class Browser - " + self.name)
        self.top.wm_iconname("Class Browser")

    def rootnode(self):
        return ModuleBrowserTreeItem(self.file)

class ModuleBrowserTreeItem(TreeItem):

    def __init__(self, file):
        self.file = file

    def GetText(self):
        return os.path.basename(self.file)

    def GetIconName(self):
        return "python"

    def GetSubList(self):
        sublist = []
        for name in self.listclasses():
            item = ClassBrowserTreeItem(name, self.classes, self.file)
            sublist.append(item)
        return sublist

    def OnDoubleClick(self):
        if os.path.normcase(self.file[-3:]) != ".py":
            return
        if not os.path.exists(self.file):
            return
        PyShell.flist.open(self.file)

    def IsExpandable(self):
        return os.path.normcase(self.file[-3:]) == ".py"
    
    def listclasses(self):
        dir, file = os.path.split(self.file)
        name, ext = os.path.splitext(file)
        if os.path.normcase(ext) != ".py":
            return []
        try:
            dict = pyclbr.readmodule(name, [dir] + sys.path)
        except ImportError, msg:
            return []
        items = []
        self.classes = {}
        for key, cl in dict.items():
            if cl.module == name:
                s = key
                if cl.super:
                    supers = []
                    for sup in cl.super:
                        if type(sup) is type(''):
                            sname = sup
                        else:
                            sname = sup.name
                            if sup.module != cl.module:
                                sname = "%s.%s" % (sup.module, sname)
                        supers.append(sname)
                    s = s + "(%s)" % string.join(supers, ", ")
                items.append((cl.lineno, s))
                self.classes[s] = cl
        items.sort()
        list = []
        for item, s in items:
            list.append(s)
        return list

class ClassBrowserTreeItem(TreeItem):

    def __init__(self, name, classes, file):
        self.name = name
        self.classes = classes
        self.file = file

    def GetText(self):
        return "class " + self.name

    def IsExpandable(self):
        try:
            cl = self.classes[self.name]
        except (IndexError, KeyError):
            return 0
        else:
            return not not cl.methods

    def GetSubList(self):
        sublist = []
        for name in self.listmethods():
            item = MethodBrowserTreeItem(
                name, self.classes[self.name], self.file)
            sublist.append(item)
        return sublist

    def OnDoubleClick(self):
        if not os.path.exists(self.file):
            return
        edit = PyShell.flist.open(self.file)
        if self.classes.has_key(self.name):
            cl = self.classes[self.name]
        else:
            name = self.name
            i = string.find(name, '(')
            if i < 0:
                return
            name = name[:i]
            if not self.classes.has_key(name):
                return
            cl = self.classes[name]
        if not hasattr(cl, 'lineno'):
            return
        lineno = cl.lineno
        edit.gotoline(lineno)

    def listmethods(self):
        try:
            cl = self.classes[self.name]
        except (IndexError, KeyError):
            return []
        items = []
        for name, lineno in cl.methods.items():
            items.append((lineno, name))
        items.sort()
        list = []
        for item, name in items:
            list.append(name)
        return list

class MethodBrowserTreeItem(TreeItem):

    def __init__(self, name, cl, file):
        self.name = name
        self.cl = cl
        self.file = file

    def GetText(self):
        return "def " + self.name + "(...)"

    def GetIconName(self):
        return "python" # XXX

    def IsExpandable(self):
        return 0

    def OnDoubleClick(self):
        if not os.path.exists(self.file):
            return
        edit = PyShell.flist.open(self.file)
        edit.gotoline(self.cl.methods[self.name])

def main():
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
    ClassBrowser(PyShell.flist, name, [dir])
    if sys.stdin is sys.__stdin__:
        mainloop()

if __name__ == "__main__":
    main()
