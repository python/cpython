import string
from Tkinter import *
import linecache

from TreeWidget import TreeNode, TreeItem, ScrolledCanvas
from ObjectBrowser import ObjectTreeItem, make_objecttreeitem
from OldStackViewer import StackViewer, NamespaceViewer

def StackBrowser(root, flist=None, stack=None):
    top = Toplevel(root)
    sc = ScrolledCanvas(top, bg="white", highlightthickness=0)
    sc.frame.pack(expand=1, fill="both")
    item = StackTreeItem(flist)
    node = TreeNode(sc.canvas, None, item)
    node.expand()

class StackTreeItem(TreeItem):

    def __init__(self, flist=None):
        self.flist = flist
        self.stack = get_stack()
        self.text = get_exception()

    def GetText(self):
        return self.text

    def GetSubList(self):
        sublist = []
        for info in self.stack:
            item = FrameTreeItem(info, self.flist)
            sublist.append(item)
        return sublist

class FrameTreeItem(TreeItem):

    def __init__(self, info, flist):
        self.info = info
        self.flist = flist

    def GetText(self):
        frame, lineno = self.info
        try:
            modname = frame.f_globals["__name__"]
        except:
            modname = "?"
        code = frame.f_code
        filename = code.co_filename
        funcname = code.co_name
        sourceline = linecache.getline(filename, lineno)
        sourceline = string.strip(sourceline)
        if funcname in ("?", "", None):
            item = "%s, line %d: %s" % (modname, lineno, sourceline)
        else:
            item = "%s.%s(...), line %d: %s" % (modname, funcname,
                                             lineno, sourceline)
##        if i == index:
##            item = "> " + item
        return item

    def GetSubList(self):
        frame, lineno = self.info
        sublist = []
        if frame.f_globals is not frame.f_locals:
            item = VariablesTreeItem("<locals>", frame.f_locals, self.flist)
            sublist.append(item)
        item = VariablesTreeItem("<globals>", frame.f_globals, self.flist)
        sublist.append(item)
        return sublist

    def OnDoubleClick(self):
        if self.flist:
            frame, lineno = self.info
            filename = frame.f_code.co_filename
            edit = self.flist.open(filename)
            edit.gotoline(lineno)

class VariablesTreeItem(ObjectTreeItem):

    def GetText(self):
        return self.labeltext

    def GetLabelText(self):
        return None

    def IsExpandable(self):
        return len(self.object) > 0

    def keys(self):
        return self.object.keys()

    def GetSubList(self):
        sublist = []
        for key in self.keys():
            try:
                value = self.object[key]
            except KeyError:
                continue
            def setfunction(value, key=key, object=self.object):
                object[key] = value
            item = make_objecttreeitem(key + " =", value, setfunction)
            sublist.append(item)
        return sublist
        
def get_stack(t=None, f=None):
    if t is None:
        t = sys.last_traceback
    stack = []
    if t and t.tb_frame is f:
        t = t.tb_next
    while f is not None:
        stack.append((f, f.f_lineno))
        if f is self.botframe:
            break
        f = f.f_back
    stack.reverse()
    while t is not None:
        stack.append((t.tb_frame, t.tb_lineno))
        t = t.tb_next
    return stack

def get_exception(type=None, value=None):
    if type is None:
        type = sys.last_type
        value = sys.last_value
    if hasattr(type, "__name__"):
        type = type.__name__
    s = str(type)
    if value is not None:
        s = s + ": " + str(value)
    return s

if __name__ == "__main__":
    root = Tk()
    root.withdraw()
    StackBrowser(root)
