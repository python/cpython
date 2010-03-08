# Display hello, world in a button; clicking it quits the program

import sys
from Tkinter import *

def main():
    root = Tk()
    button = Button(root)
    button['text'] = 'Hello, world'
    button['command'] = quit_callback       # See below
    button.pack()
    root.mainloop()

def quit_callback():
    sys.exit(0)

main()
