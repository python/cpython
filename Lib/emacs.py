# Execute Emacs code from a Python interpreter.
# This code should be imported from a Python interpreter that is
# running as an inferior process of Emacs.
# See misc/py-connect.el for the companion Emacs lisp code.
# Author: Terrence M. Brannon.

start_marker = '+'
end_marker   = '~'

def eval (string):
	tmpstr = start_marker + '(' + string + ')' + end_marker
	print tmpstr

def dired (directory):
	eval( 'dired ' + '"' + directory + '"' )

def buffer_menu ():
	eval( 'buffer-menu(buffer-list)' )
