from Tkinter import *

class Separator:
    
    def __init__(self, master, orient, min=10, thickness=5, bg=None):
        self.min = max(1, min)
        self.thickness = max(1, thickness)
        if orient in ("h", "horizontal"):
            self.side = "left"
            self.dim = "width"
            self.dir = "x"
            self.cursor = "sb_h_double_arrow"
    	elif orient in ("v", "vertical"):
    	    self.side = "top"
    	    self.dim = "height"
    	    self.dir = "y"
            self.cursor = "sb_v_double_arrow"
    	else:
    	    raise ValueError, "Separator: orient should be h or v"
    	self.winfo_dim = "winfo_" + self.dim
        self.master = master = Frame(master)
        master.pack(expand=1, fill="both")
        self.f1 = Frame(master)
        self.f1.pack(expand=1, fill="both", side=self.side)
        self.div = Frame(master, cursor=self.cursor)
        self.div[self.dim] = self.thickness
        self.div.pack(fill="both", side=self.side)
        self.f2 = Frame(master)
        self.f2.pack(expand=1, fill="both", side=self.side)
        self.div.bind("<ButtonPress-1>", self.divider_press)
        if bg:
            ##self.f1["bg"] = bg
            ##self.f2["bg"] = bg
            self.div["bg"] = bg

    def parts(self):
        return self.f1, self.f2

    def divider_press(self, event):
        self.press_event = event
        self.f1.pack_propagate(0)
        self.f2.pack_propagate(0)
        for f in self.f1, self.f2:
            for dim in "width", "height":
                f[dim] = getattr(f, "winfo_"+dim)()
        self.div.bind("<Motion>", self.div_motion)
        self.div.bind("<ButtonRelease-1>", self.div_release)
        self.div.grab_set()

    def div_motion(self, event):
        delta = getattr(event, self.dir) - getattr(self.press_event, self.dir)
        if delta:
            dim1 = getattr(self.f1, self.winfo_dim)()
            dim2 = getattr(self.f2, self.winfo_dim)()
            delta = max(delta, self.min-dim1)
            delta = min(delta, dim2-self.min)
            dim1 = dim1 + delta
            dim2 = dim2 - delta
            self.f1[self.dim] = dim1
            self.f2[self.dim] = dim2

    def div_release(self, event):
        self.div_motion(event)
        self.div.unbind("<Motion>")
        self.div.grab_release()

class VSeparator(Separator):

    def __init__(self, master, min=10, thickness=5, bg=None):
        Separator.__init__(self, master, "v", min, thickness, bg)

class HSeparator(Separator):

    def __init__(self, master, min=10, thickness=5, bg=None):
        Separator.__init__(self, master, "h", min, thickness, bg)

def main():
    root = Tk()
    tlist = []
    outer = HSeparator(root, bg="red")
    for part in outer.parts():
        inner = VSeparator(part, bg="blue")
        for f in inner.parts():
            t = Text(f, width=40, height=10, borderwidth=0)
            t.pack(fill="both", expand=1)
            tlist.append(t)
    tlist[0].insert("1.0", "Make your own Mondrian!")
    tlist[1].insert("1.0", "Move the colored dividers...")
    root.mainloop()

if __name__ == '__main__':
    main()
