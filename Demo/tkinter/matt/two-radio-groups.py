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



def makePoliticalParties():
    # make menu button 
    Radiobutton_button = Menubutton(mBar, {'text': 'Political Party', 
					   'underline': 0,
					   Pack: {'side': 'left', 
						  'padx': '2m'}})
    
    # the primary pulldown
    Radiobutton_button.menu = Menu(Radiobutton_button)

    Radiobutton_button.menu.add('radiobutton', {'label': 'Republican', 
						'variable' : party, 
						'value' : 1})

    Radiobutton_button.menu.add('radiobutton', {'label': 'Democrat', 
						'variable' : party, 
						'value' : 2})

    Radiobutton_button.menu.add('radiobutton', {'label': 'Libertarian', 
						'variable' : party, 
						'value' : 3})
    
    party.set(2)

    # set up a pointer from the file menubutton back to the file menu
    Radiobutton_button['menu'] = Radiobutton_button.menu

    return Radiobutton_button


def makeFlavors():
    # make menu button 
    Radiobutton_button = Menubutton(mBar, {'text': 'Flavors', 
					   'underline': 0,
					   Pack: {'side': 'left', 
						  'padx': '2m'}})
    # the primary pulldown
    Radiobutton_button.menu = Menu(Radiobutton_button)

    Radiobutton_button.menu.add('radiobutton', {'label': 'Strawberry', 
						'variable' : flavor, 
						'value' : 'Strawberry'})

    Radiobutton_button.menu.add('radiobutton', {'label': 'Chocolate', 
						'variable' : flavor, 
						'value' : 'Chocolate'})

    Radiobutton_button.menu.add('radiobutton', {'label': 'Rocky Road', 
						'variable' : flavor, 
						'value' : 'Rocky Road'})

    # choose a default
    flavor.set("Chocolate")

    # set up a pointer from the file menubutton back to the file menu
    Radiobutton_button['menu'] = Radiobutton_button.menu

    return Radiobutton_button


def printStuff():
    print "party is", party.get()
    print "flavor is", flavor.get()
    print ""

#################################################
#### Main starts here ...
root = Tk()


# make a menu bar
mBar = Frame(root, {'relief': 'raised', 
		    'bd': 2,
		    Pack: {'side': 'top', 
			   'fill': 'x'}})

# make two application variables, 
# one to control each radio button set
party = IntVar()
flavor = StringVar()

Radiobutton_button = makePoliticalParties()
Radiobutton_button2 = makeFlavors()

# finally, install the buttons in the menu bar. 
# This allows for scanning from one menubutton to the next.
mBar.tk_menuBar(Radiobutton_button, Radiobutton_button2)

b = Button(root, {"text": "print party and flavor", 
		  "command" : printStuff, 
		  "fg":  "red"})
b.pack({"side" : "top"})

root.title('menu demo')
root.iconname('menu demo')

root.mainloop()
