import os
import sys
import linecache

from TreeWidget import TreeNode, TreeItem, ScrolledCanvas
from ObjectBrowser import ObjectTreeItem, make_objecttreeitem

def StackBrowser(root, flist=None, tb=None, top=None):
    if top is None:
        from Tkinter import Toplevel
        top = Toplevel(root)
    sc = ScrolledCanvas(top, bg="white", highlightthickness=0)
    sc.frame.pack(expand=1, fill="both")
    item = StackTreeItem(flist, tb)
    node = TreeNode(sc.canvas, None, item)
    node.expand()

class StackTreeItem(TreeItem):

    def __init__(self, flist=None, tb=None):
        self.flist = flist
        self.stack = self.get_stack(tb)
        self.text = self.get_exception()

    def get_stack(self, tb):
        if tb is None:
            tb = sys.last_traceback
        stack = []
        if tb and tb.tb_frame is None:
            tb = tb.tb_next
        while tb is not None:
            stack.append((tb.tb_frame, tb.tb_lineno))
            tb = tb.tb_next
        return stack

    def get_exception(self):
        type = sys.last_type
        value = sys.last_value
        if hasattr(type, "__name__"):
            type = type.__name__
        s = str(type)
        if value is not None:
            s = s + ": " + str(value)
        return s

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
        sourceline = sourceline.strip()
        if funcname in ("?", "", None):
            item = "%s, line %d: %s" % (modname, lineno, sourceline)
        else:
            item = "%s.%s(...), line %d: %s" % (modname, funcname,
                                             lineno, sourceline)
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
            if os.path.isfile(filename):
                self.flist.gotofileline(filename, lineno)

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


def _test():
    try:
        import testcode
        reload(testcode)
    except:
        sys.last_type, sys.last_value, sys.last_traceback = sys.exc_info()
    from Tkinter import Tk
    root = Tk()
    StackBrowser(None, top=root)
    root.mainloop()

if __name__ == "__main__":
    _test()
