from Tkinter import *

# some vocabulary to keep from getting confused. This terminology 
# is something I cooked up for this file, but follows the man pages 
# pretty closely
# 
# 
# 
#       This is a MENUBUTTON
#       V
# +-------------+
# |             |
# 
# +------------++------------++------------+
# |            ||            ||            |
# |  File      ||  Edit      || Options    |   <-------- the MENUBAR
# |            ||            ||            |
# +------------++------------++------------+
# | New...         |
# | Open...        |
# | Print          |
# |                |  <------ This is a MENU. The lines of text in the menu are
# |                |                          MENU ENTRIES
# |                +---------------+
# | Open Files >   | file1         |               
# |                | file2         |
# |                | another file  | <------ this cascading part is also a MENU
# +----------------|               |
#                  |               |
#                  |               |
#                  |               |
#                  +---------------+



def new_file():
    print "opening new file"


def open_file():
    print "opening OLD file"


def makeFileMenu():
    # make menu button : "File"
    File_button = Menubutton(mBar, text='File', underline=0)
    File_button.pack(side=LEFT, padx="1m")
    File_button.menu = Menu(File_button)
    
    # add an item. The first param is a menu entry type, 
    # must be one of: "cascade", "checkbutton", "command", "radiobutton", "seperator"
    # see menu-demo-2.py for examples of use
    File_button.menu.add_command(label='New...', underline=0, 
				 command=new_file)
    
    
    File_button.menu.add_command(label='Open...', underline=0, 
				 command=open_file)
    
    File_button.menu.add_command(label='Quit', underline=0, 
				 command='exit')

    # set up a pointer from the file menubutton back to the file menu
    File_button['menu'] = File_button.menu

    return File_button



def makeEditMenu():
    Edit_button = Menubutton(mBar, text='Edit', underline=0)
    Edit_button.pack(side=LEFT, padx="1m")
    Edit_button.menu = Menu(Edit_button)

    # just to be cute, let's disable the undo option:
    Edit_button.menu.add('command', label="Undo")
    # Since the tear-off bar is the 0th entry,
    # undo is the 1st entry...
    Edit_button.menu.entryconfig(1, state=DISABLED)

    # and these are just for show. No "command" callbacks attached.
    Edit_button.menu.add_command(label="Cut")
    Edit_button.menu.add_command(label="Copy")
    Edit_button.menu.add_command(label="Paste")

    # set up a pointer from the file menubutton back to the file menu
    Edit_button['menu'] = Edit_button.menu

    return Edit_button


#################################################

#### Main starts here ...
root = Tk()


# make a menu bar
mBar = Frame(root, relief=RAISED, borderwidth=2)
mBar.pack(fill=X)

File_button = makeFileMenu()
Edit_button = makeEditMenu()

# finally, install the buttons in the menu bar. 
# This allows for scanning from one menubutton to the next.
mBar.tk_menuBar(File_button, Edit_button)

root.title('menu demo')
root.iconname('packer')

root.mainloop()






