# One or more ScrolledLists with HSeparators between them.
# There is a hierarchical relationship between them:
# the right list displays the substructure of the selected item
# in the left list.

import string
from Tkinter import *
from WindowList import ListedToplevel
from Separator import HSeparator
from ScrolledList import ScrolledList

class MultiScrolledLists:
    
    def __init__(self, root, nlists=2):
        assert nlists >= 1
        self.root = root
        self.nlists = nlists
        self.path = []
        # create top
        self.top = top = ListedToplevel(root)
        top.protocol("WM_DELETE_WINDOW", self.close)
        top.bind("<Escape>", self.close)
        self.settitle()
        # create frames and separators in between
        self.frames = []
        self.separators = []
        last = top
        for i in range(nlists-1):
            sepa = HSeparator(last)
            self.separators.append(sepa)
            frame, last = sepa.parts()
            self.frames.append(frame)
        self.frames.append(last)
        # create labels and lists
        self.labels = []
        self.lists = []
        for i in range(nlists):
            frame = self.frames[i]
            label = Label(frame, text=self.subtitle(i),
                relief="groove", borderwidth=2)
            label.pack(fill="x")
            self.labels.append(label)
            list = ScrolledList(frame, width=self.width(i),
                height=self.height(i))
            self.lists.append(list)
            list.on_select = \
                lambda index, i=i, self=self: self.on_select(index, i)
            list.on_double = \
                lambda index, i=i, self=self: self.on_double(index, i)
        # fill leftmost list (rest get filled on demand)
        self.fill(0)
        # XXX one after_idle isn't enough; two are...
        top.after_idle(self.call_pack_propagate_1)
    
    def call_pack_propagate_1(self):
        self.top.after_idle(self.call_pack_propagate)
    
    def call_pack_propagate(self):
        for frame in self.frames:
            frame.pack_propagate(0)
    
    def close(self, event=None):
        self.top.destroy()
    
    def settitle(self):
        short = self.shorttitle()
        long = self.longtitle()
        if short and long:
            title = short + " - " + long
        elif short:
            title = short
        elif long:
            title = long
        else:
            title = "Untitled"
        icon = short or long or title
        self.top.wm_title(title)
        self.top.wm_iconname(icon)

    def longtitle(self):
        # override this
        return "Multi Scrolled Lists"
    
    def shorttitle(self):
        # override this
        return None
    
    def width(self, i):
        # override this
        return 20
    
    def height(self, i):
        # override this
        return 10
    
    def subtitle(self, i):
        # override this
        return "Column %d" % i
     
    def fill(self, i):
        for k in range(i, self.nlists):
            self.lists[k].clear()
            self.labels[k].configure(text=self.subtitle(k))
        list = self.lists[i]
        l = self.items(i)
        for s in l:
            list.append(s)
        
    def on_select(self, index, i):
        item = self.lists[i].get(index)
        del self.path[i:]
        self.path.append(item)
        if i+1 < self.nlists:
            self.fill(i+1)
   
    def items(self, i):
        # override this
        l = []
        for k in range(10):
            s = str(k)
            if i > 0:
                s = self.path[i-1] + "." + s
            l.append(s)
        return l
    
    def on_double(self, index, i):
        pass


def main():
    root = Tk()
    quit = Button(root, text="Exit", command=root.destroy)
    quit.pack()
    MultiScrolledLists(root, 4)
    root.mainloop()

if __name__ == "__main__":
    main()
