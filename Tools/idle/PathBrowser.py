import os
import sys
import imp
import string
import tkMessageBox

from MultiScrolledLists import MultiScrolledLists

class PathBrowser(MultiScrolledLists):
    
    def __init__(self, flist):
        self.flist = flist
        MultiScrolledLists.__init__(self, flist.root, 4)
    
    def longtitle(self):
        return "Path Browser"
    
    def width(self, i):
        return 30
    
    def height(self, i):
        return 20
    
    def subtitle(self, i):
        if i == 0:
            return "Path Entries (sys.path)"
        if i-1 >= len(self.path):
            return ""
        if i == 1:
            return self.path[0]
        if i == 2:
            return "Classes in " + self.path[1]
        if i == 3:
            s = self.path[2]
            i = string.find(s, "(")
            if i > 0:
                s = s[:i]
            return "Methods of " + s
        return ""
    
    def items(self, i):
        if i == 0:
            return sys.path
        if i == 1:
            return self.listmodules()
        if i == 2:
            return self.listclasses()
        if i == 3:
            return self.listmethods()
    
    def listmodules(self):
        dir = self.path[0] or os.curdir
        modules = {}
        suffixes = imp.get_suffixes()
        allnames = os.listdir(dir)
        sorted = []
        for suff, mode, flag in suffixes:
            i = -len(suff)
            for name in allnames:
                normed_name = os.path.normcase(name)
                if normed_name[i:] == suff:
                    mod_name = name[:i]
                    if not modules.has_key(mod_name):
                        modules[mod_name] = None
                        sorted.append((normed_name, name))
        sorted.sort()
        names = []
        for nn, name in sorted:
            names.append(name)
        return names
    
    def listclasses(self):
        import pyclbr
        dir = self.path[0]
        file = self.path[1]
        name, ext = os.path.splitext(file)
        if os.path.normcase(ext) != ".py":
            self.top.bell()
            return []
        try:
            self.top.configure(cursor="watch")
            self.top.update_idletasks()
            try:
                dict = pyclbr.readmodule(name, [dir] + sys.path)
            finally:
                self.top.configure(cursor="")
        except ImportError, msg:
            tkMessageBox.showerror("Import error", str(msg), parent=root)
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
    
    def listmethods(self):
        try:
            cl = self.classes[self.path[2]]
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
    
    def on_double(self, index, i):
        if i == 0:
            return
        if i >= 1:
            dir = self.path[0]
            file = self.path[1]
            name, ext = os.path.splitext(file)
            if os.path.normcase(ext) != ".py":
                self.top.bell()
                return
            fullname = os.path.join(dir, file)
            edit = self.flist.open(fullname)
            if i >= 2:
                classname = self.path[2]
                try:
                    cl = self.classes[classname]
                except KeyError:
                    cl = None
                else:
                    if i == 2:
                        edit.gotoline(cl.lineno)
                    else:
                        methodname = self.path[3]
                        edit.gotoline(cl.methods[methodname])


def main():
    import PyShell
    PathBrowser(PyShell.flist)

if __name__ == "__main__":
    main()
