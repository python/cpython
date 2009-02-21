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
# |                |  <-------- This is a MENU. The lines of text in the menu are
# |                |                            MENU ENTRIES
# |                +---------------+
# | Open Files >   | file1         |
# |                | file2         |
# |                | another file  | <------ this cascading part is also a MENU
# +----------------|               |
#                  |               |
#                  |               |
#                  |               |
#                  +---------------+



# some miscellaneous callbacks
def new_file():
    print "opening new file"

def open_file():
    print "opening OLD file"

def print_something():
    print "picked a menu item"



anchovies = 0

def print_anchovies():
    global anchovies
    anchovies = not anchovies
    print "anchovies?", anchovies

def makeCommandMenu():
    # make menu button
    Command_button = Menubutton(mBar, text='Simple Button Commands',
                                underline=0)
    Command_button.pack(side=LEFT, padx="2m")

    # make the pulldown part of the File menu. The parameter passed is the master.
    # we attach it to the button as a python attribute called "menu" by convention.
    # hopefully this isn't too confusing...
    Command_button.menu = Menu(Command_button)

    # just to be cute, let's disable the undo option:
    Command_button.menu.add_command(label="Undo")
    # undo is the 0th entry...
    Command_button.menu.entryconfig(0, state=DISABLED)

    Command_button.menu.add_command(label='New...', underline=0,
                                    command=new_file)
    Command_button.menu.add_command(label='Open...', underline=0,
                                    command=open_file)
    Command_button.menu.add_command(label='Different Font', underline=0,
                                    font='-*-helvetica-*-r-*-*-*-180-*-*-*-*-*-*',
                                    command=print_something)

    # we can make bitmaps be menu entries too. File format is X11 bitmap.
    # if you use XV, save it under X11 bitmap format. duh-uh.,..
    Command_button.menu.add_command(
        bitmap="info")
        #bitmap='@/home/mjc4y/dilbert/project.status.is.doomed.last.panel.bm')

    # this is just a line
    Command_button.menu.add('separator')

    # change the color
    Command_button.menu.add_command(label='Quit', underline=0,
                                    background='red',
                                    activebackground='green',
                                    command=Command_button.quit)

    # set up a pointer from the file menubutton back to the file menu
    Command_button['menu'] = Command_button.menu

    return Command_button



def makeCascadeMenu():
    # make menu button
    Cascade_button = Menubutton(mBar, text='Cascading Menus', underline=0)
    Cascade_button.pack(side=LEFT, padx="2m")

    # the primary pulldown
    Cascade_button.menu = Menu(Cascade_button)

    # this is the menu that cascades from the primary pulldown....
    Cascade_button.menu.choices = Menu(Cascade_button.menu)

    # ...and this is a menu that cascades from that.
    Cascade_button.menu.choices.weirdones = Menu(Cascade_button.menu.choices)

    # then you define the menus from the deepest level on up.
    Cascade_button.menu.choices.weirdones.add_command(label='avacado')
    Cascade_button.menu.choices.weirdones.add_command(label='belgian endive')
    Cascade_button.menu.choices.weirdones.add_command(label='beefaroni')

    # definition of the menu one level up...
    Cascade_button.menu.choices.add_command(label='Chocolate')
    Cascade_button.menu.choices.add_command(label='Vanilla')
    Cascade_button.menu.choices.add_command(label='TuttiFruiti')
    Cascade_button.menu.choices.add_command(label='WopBopaLoopBapABopBamBoom')
    Cascade_button.menu.choices.add_command(label='Rocky Road')
    Cascade_button.menu.choices.add_command(label='BubbleGum')
    Cascade_button.menu.choices.add_cascade(
        label='Weird Flavors',
        menu=Cascade_button.menu.choices.weirdones)

    # and finally, the definition for the top level
    Cascade_button.menu.add_cascade(label='more choices',
                                    menu=Cascade_button.menu.choices)

    Cascade_button['menu'] = Cascade_button.menu

    return Cascade_button

def makeCheckbuttonMenu():
    global fred
    # make menu button
    Checkbutton_button = Menubutton(mBar, text='Checkbutton Menus',
                                    underline=0)
    Checkbutton_button.pack(side=LEFT, padx='2m')

    # the primary pulldown
    Checkbutton_button.menu = Menu(Checkbutton_button)

    # and all the check buttons. Note that the "variable" "onvalue" and "offvalue" options
    # are not supported correctly at present. You have to do all your application
    # work through the calback.
    Checkbutton_button.menu.add_checkbutton(label='Pepperoni')
    Checkbutton_button.menu.add_checkbutton(label='Sausage')
    Checkbutton_button.menu.add_checkbutton(label='Extra Cheese')

    # so here's a callback
    Checkbutton_button.menu.add_checkbutton(label='Anchovy',
                                            command=print_anchovies)

    # and start with anchovies selected to be on. Do this by
    # calling invoke on this menu option. To refer to the "anchovy" menu
    # entry we need to know it's index. To do this, we use the index method
    # which takes arguments of several forms:
    #
    # argument        what it does
    # -----------------------------------
    # a number        -- this is useless.
    # "last"          -- last option in the menu
    # "none"          -- used with the activate command. see the man page on menus
    # "active"        -- the currently active menu option. A menu option is made active
    #                         with the 'activate' method
    # "@number"       -- where 'number' is an integer and is treated like a y coordinate in pixels
    # string pattern  -- this is the option used below, and attempts to match "labels" using the
    #                    rules of Tcl_StringMatch
    Checkbutton_button.menu.invoke(Checkbutton_button.menu.index('Anchovy'))

    # set up a pointer from the file menubutton back to the file menu
    Checkbutton_button['menu'] = Checkbutton_button.menu

    return Checkbutton_button


def makeRadiobuttonMenu():
    # make menu button
    Radiobutton_button = Menubutton(mBar, text='Radiobutton Menus',
                                    underline=0)
    Radiobutton_button.pack(side=LEFT, padx='2m')

    # the primary pulldown
    Radiobutton_button.menu = Menu(Radiobutton_button)

    # and all the Radio buttons. Note that the "variable" "onvalue" and "offvalue" options
    # are not supported correctly at present. You have to do all your application
    # work through the calback.
    Radiobutton_button.menu.add_radiobutton(label='Republican')
    Radiobutton_button.menu.add_radiobutton(label='Democrat')
    Radiobutton_button.menu.add_radiobutton(label='Libertarian')
    Radiobutton_button.menu.add_radiobutton(label='Commie')
    Radiobutton_button.menu.add_radiobutton(label='Facist')
    Radiobutton_button.menu.add_radiobutton(label='Labor Party')
    Radiobutton_button.menu.add_radiobutton(label='Torie')
    Radiobutton_button.menu.add_radiobutton(label='Independent')
    Radiobutton_button.menu.add_radiobutton(label='Anarchist')
    Radiobutton_button.menu.add_radiobutton(label='No Opinion')

    # set up a pointer from the file menubutton back to the file menu
    Radiobutton_button['menu'] = Radiobutton_button.menu

    return Radiobutton_button


def makeDisabledMenu():
    Dummy_button = Menubutton(mBar, text='Dead Menu', underline=0)
    Dummy_button.pack(side=LEFT, padx='2m')

    # this is the standard way of turning off a whole menu
    Dummy_button["state"] = DISABLED
    return Dummy_button


#################################################
#### Main starts here ...
root = Tk()


# make a menu bar
mBar = Frame(root, relief=RAISED, borderwidth=2)
mBar.pack(fill=X)

Command_button     = makeCommandMenu()
Cascade_button     = makeCascadeMenu()
Checkbutton_button = makeCheckbuttonMenu()
Radiobutton_button = makeRadiobuttonMenu()
NoMenu             = makeDisabledMenu()

# finally, install the buttons in the menu bar.
# This allows for scanning from one menubutton to the next.
mBar.tk_menuBar(Command_button, Cascade_button, Checkbutton_button, Radiobutton_button, NoMenu)


root.title('menu demo')
root.iconname('menu demo')

root.mainloop()
