"""Easy to use dialogs.

Message(msg) -- display a message and an OK button.
AskString(prompt, default) -- ask for a string, display OK and Cancel buttons.
AskYesNoCancel(question, default) -- display a question and Yes, No and Cancel buttons.

More documentation in each function.
This module uses DLOG resources 256, 257 and 258.
Based upon STDWIN dialogs with the same names and functions.
"""

from Dlg import GetNewDialog, SetDialogItemText, GetDialogItemText, ModalDialog


def Message(msg):
	"""Display a MESSAGE string.
	
	Return when the user clicks the OK button or presses Return.
	
	The MESSAGE string can be at most 255 characters long.
	"""
	
	id = 256
	d = GetNewDialog(id, -1)
	if not d:
		print "Can't get DLOG resource with id =", id
		return
	tp, h, rect = d.GetDialogItem(2)
	SetDialogItemText(h, msg)
	while 1:
		n = ModalDialog(None)
		if n == 1:
			return


def AskString(prompt, default = ""):
	"""Display a PROMPT string and a text entry field with a DEFAULT string.
	
	Return the contents of the text entry field when the user clicks the
	OK button or presses Return.
	Return None when the user clicks the Cancel button.
	
	If omitted, DEFAULT is empty.
	
	The PROMPT and DEFAULT strings, as well as the return value,
	can be at most 255 characters long.
	"""
	
	id = 257
	d = GetNewDialog(id, -1)
	if not d:
		print "Can't get DLOG resource with id =", id
		return
	tp, h, rect = d.GetDialogItem(3)
	SetDialogItemText(h, prompt)
	tp, h, rect = d.GetDialogItem(4)
	SetDialogItemText(h, default)
#	d.SetDialogItem(4, 0, 255)
	while 1:
		n = ModalDialog(None)
		if n == 1:
			tp, h, rect = d.GetDialogItem(4)
			return GetDialogItemText(h)
		if n == 2: return None


def AskYesNoCancel(question, default = 0):
##	"""Display a QUESTION string which can be answered with Yes or No.
##	
##	Return 1 when the user clicks the Yes button.
##	Return 0 when the user clicks the No button.
##	Return -1 when the user clicks the Cancel button.
##	
##	When the user presses Return, the DEFAULT value is returned.
##	If omitted, this is 0 (No).
##	
##	The QUESTION strign ca be at most 255 characters.
##	"""
	
	id = 258
	d = GetNewDialog(id, -1)
	if not d:
		print "Can't get DLOG resource with id =", id
		return
	# Button assignments:
	# 1 = default (invisible)
	# 2 = Yes
	# 3 = No
	# 4 = Cancel
	# The question string is item 5
	tp, h, rect = d.GetDialogItem(5)
	SetDialogItemText(h, question)
	while 1:
		n = ModalDialog(None)
		if n == 1: return default
		if n == 2: return 1
		if n == 3: return 0
		if n == 4: return -1


def test():
	Message("Testing EasyDialogs.")
	ok = AskYesNoCancel("Do you want to proceed?")
	if ok > 0:
		s = AskString("Enter your first name")
		Message("Thank you,\015%s" % `s`)


if __name__ == '__main__':
	test()
