#
# Example 1 - Using fl in python without callbacks.
#
# The form is named 'main_form' and resides on file 'test_nocb.fd'.
# It has three objects named button1, button2 and exitbutton.
#
import fl		# The forms library
import FL		# Symbolic constants for the above
import flp		# The module to parse .fd files
import sys

# The following struct is created to hold the instance variables
# main_form, button1, button2 and exitbutton.

class struct: pass
container = struct()

#
# We now first parse the forms file

parsetree = flp.parse_form('test_nocb', 'main_form')

#
# Next we create it

flp.create_full_form(container, parsetree)

#
# And display it

container.main_form.show_form(FL.PLACE_MOUSE, 1, '')

#
# And interact until the exit button is pressed
while 1:
	selected_obj = fl.do_forms()
	if selected_obj == container.button1:
		print 'Button 1 selected'
	elif selected_obj == container.button2:
		print 'Button 2 selected'
	elif selected_obj == container.exitbutton:
		print 'Ok, bye bye'
		sys.exit(0)
	else:
		print 'do_forms() returned unknown object ', selected_obj
