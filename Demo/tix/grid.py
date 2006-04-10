###
import Tix as tk
from pprint import pprint

r= tk.Tk()
r.title("test")

l=tk.Label(r, name="a_label")
l.pack()

class MyGrid(tk.Grid):
    def __init__(self, *args, **kwargs):
        kwargs['editnotify']= self.editnotify
        tk.Grid.__init__(self, *args, **kwargs)
    def editnotify(self, x, y):
        return True

g = MyGrid(r, name="a_grid",
selectunit="cell")
g.pack(fill=tk.BOTH)
for x in xrange(5):
    for y in xrange(5):
        g.set(x,y,text=str((x,y)))

c = tk.Button(r, text="Close", command=r.destroy)
c.pack()

tk.mainloop()
