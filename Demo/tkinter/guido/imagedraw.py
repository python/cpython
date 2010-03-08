"""Draw on top of an image"""

from Tkinter import *
import sys

def main():
    filename = sys.argv[1]
    root = Tk()
    img = PhotoImage(file=filename)
    w, h = img.width(), img.height()
    canv = Canvas(root, width=w, height=h)
    canv.create_image(0, 0, anchor=NW, image=img)
    canv.pack()
    canv.bind('<Button-1>', blob)
    root.mainloop()

def blob(event):
    x, y = event.x, event.y
    canv = event.widget
    r = 5
    canv.create_oval(x-r, y-r, x+r, y+r, fill='red', outline="")

main()
