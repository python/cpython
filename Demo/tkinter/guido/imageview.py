from Tkinter import *
import sys

def main():
    filename = sys.argv[1]
    root = Tk()
    img = PhotoImage(file=filename)
    label = Label(root, image=img)
    label.pack()
    root.mainloop()

main()
