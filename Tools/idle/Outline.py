from Tkinter import *

class Outline:
    
    def __init__(self, root=None):
        if not root:
            import Tkinter
            root = Tkinter._default_root
        if not root:
            root = top = Tk()
        else:
            top = Toplevel(root)
        top.wm_title("Outline")
        self.canvas = canvas = Canvas(top, width=400, height=300,
                                      borderwidth=2, relief="sunken",
                                      background="#FFBBBB")
        canvas.pack(expand=1, fill="both")
        self.items = []
    
    def additem(self, level, open, label):
        x = 15*level + 5
        y = 15*len(self.items) + 5
        if open:
            id1 = self.canvas.create_polygon(x+3, y+3, x+13, y+3, x+8, y+8,
                                             outline="black",
                                             fill="green")
        else:
            id1 = self.canvas.create_polygon(x+3, y+4, x+7, y+8, x+3, y+12,
                                             outline="black",
                                             fill="red")
        w = Entry(self.canvas, borderwidth=0, background="#FFBBBB", width=0)
        w.insert("end", label)
	id2 = self.canvas.create_window(x+15, y, anchor="nw", window=w)
        self.items.append((level, open, label, id1, w, id2))
        

def main():
    o = Outline()
    o.additem(0, 1, "hello world")
    o.additem(1, 0, "sub1")
    o.additem(1, 1, "sub2")
    o.additem(2, 0, "sub2.a")
    o.additem(2, 0, "sub2.b")
    o.additem(1, 0, "sub3")

main()
