from Tkinter import *

#	The way to think about this is that each radio button menu
#	controls a different variable -- clicking on one of the
#	mutually exclusive choices in a radiobutton assigns some value
#	to an application variable you provide. When you define a
#	radiobutton menu choice, you have the option of specifying the
#	name of a varaible and value to assign to that variable when
#	that choice is selected. This clever mechanism relieves you,
#	the programmer, from having to write a dumb callback that
#	probably wouldn't have done anything more than an assignment
#	anyway. The Tkinter options for this follow their Tk
#	counterparts: 
#	{"variable" : my_flavor_variable, "value" : "strawberry"}
#       where my_flavor_variable is an instance of one of the
#       subclasses of Variable, provided in Tkinter.py (there is
#	StringVar(), IntVar(), DoubleVar() and BooleanVar() to choose
#	from) 



def makePoliticalParties(var):
    # make menu button 
    Radiobutton_button = Menubutton(mBar, text='Political Party', 
				    underline=0)
    Radiobutton_button.pack(side=LEFT, padx='2m')
    
    # the primary pulldown
    Radiobutton_button.menu = Menu(Radiobutton_button)

    Radiobutton_button.menu.add_radiobutton(label='Republican', 
					    variable=var, value=1)

    Radiobutton_button.menu.add('radiobutton', {'label': 'Democrat', 
						'variable' : var, 
						'value' : 2})

    Radiobutton_button.menu.add('radiobutton', {'label': 'Libertarian', 
						'variable' : var, 
						'value' : 3})
    
    var.set(2)

    # set up a pointer from the file menubutton back to the file menu
    Radiobutton_button['menu'] = Radiobutton_button.menu

    return Radiobutton_button


def makeFlavors(var):
    # make menu button 
    Radiobutton_button = Menubutton(mBar, text='Flavors', 
				    underline=0)
    Radiobutton_button.pack(side=LEFT, padx='2m')

    # the primary pulldown
    Radiobutton_button.menu = Menu(Radiobutton_button)

    Radiobutton_button.menu.add_radiobutton(label='Strawberry',
					    variable=var, value='Strawberry')

    Radiobutton_button.menu.add_radiobutton(label='Chocolate',
					    variable=var, value='Chocolate')

    Radiobutton_button.menu.add_radiobutton(label='Rocky Road',
					    variable=var, value='Rocky Road')

    # choose a default
    var.set("Chocolate")

    # set up a pointer from the file menubutton back to the file menu
    Radiobutton_button['menu'] = Radiobutton_button.menu

    return Radiobutton_button


def printStuff():
    print "party is", party.get()
    print "flavor is", flavor.get()
    print

#################################################
#### Main starts here ...
root = Tk()


# make a menu bar
mBar = Frame(root, relief=RAISED, borderwidth=2)
mBar.pack(fill=X)

# make two application variables, 
# one to control each radio button set
party = IntVar()
flavor = StringVar()

Radiobutton_button = makePoliticalParties(party)
Radiobutton_button2 = makeFlavors(flavor)

# finally, install the buttons in the menu bar. 
# This allows for scanning from one menubutton to the next.
mBar.tk_menuBar(Radiobutton_button, Radiobutton_button2)

b = Button(root, text="print party and flavor", foreground="red",
	   command=printStuff)
b.pack(side=TOP)

root.title('menu demo')
root.iconname('menu demo')

root.mainloop()
