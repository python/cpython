# option menu sample (Fredrik Lundh, September 1997)

from Tkinter import *

root = Tk()

#
# standard usage

var1  = StringVar()
var1.set("One") # default selection

menu1 = OptionMenu(root, var1, "One", "Two", "Three")
menu1.pack()

#
# initialize from a sequence

CHOICES = "Aah", "Bee", "Cee", "Dee", "Eff"

var2  = StringVar()
var2.set(CHOICES[0])

menu2 = apply(OptionMenu, (root, var2) + tuple(CHOICES))
menu2.pack()

root.mainloop()
