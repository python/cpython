#
# Example 2 - Using fl in python with callbacks.
#
# The form is named 'main_form' and resides on file 'test_cb.fd'.
# It has three objects named button1, button2 and exitbutton.
# All buttons have callbacks with the same names as their corresponding
# buttons but with CB appended.
#
import fl		# The forms library
import FL		# Symbolic constants for the above
import flp		# The module to parse .fd files
import sys

# The following struct is created to hold the instance variables
# main_form, button1, button2 and exitbutton.

class myform:
	#
	# The constructor parses and creates the form, but doesn't
	# display it (yet).
	def __init__(self, number):
		#
		# First we parse the form
		parsetree = flp.parse_form('test_cb', 'main_form')
		#
		# Next we create it
		
		flp.create_full_form(self, parsetree)

		# And keep our number
		self.number = number

	#
	# The show function displays the form. It doesn't do any interaction,
	# though.
	def show(self):
		self.main_form.show_form(FL.PLACE_SIZE, 1, '')

	# The callback functions
	def button1CB(self, obj, arg):
		print 'Button 1 pressed on form', self.number

	def button2CB(self, obj, arg):
		print 'Button 2 pressed on form', self.number

	def exitbuttonCB(self, obj, arg):
		print 'Ok, bye bye'
		sys.exit(0)

#
# The main program. Instantiate two variables of the forms class
# and interact with them.

form1 = myform(1)
form2 = myform(2)

form1.show()
form2.show()

obj = fl.do_forms()
print 'do_forms() returned. This should not happen. obj=', obj
