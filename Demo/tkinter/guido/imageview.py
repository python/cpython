from Tkinter import *
import sys

def main():
    filename = sys.argv[1]
    root = Tk()
    label = Label(root)
    img = PhotoImage(file=filename)
    label['image'] = img
    label.pack()
    root.mainloop()

main()
