"""Easy to use dialogs.

Message(msg) -- display a message and an OK button.
AskString(prompt, default) -- ask for a string, display OK and Cancel buttons.
AskYesNoCancel(question, default) -- display a question and Yes, No and Cancel buttons.
bar = Progress(label, maxvalue) -- Display a progress bar
bar.set(value) -- Set value

More documentation in each function.
This module uses DLOG resources 256, 257 and 258.
Based upon STDWIN dialogs with the same names and functions.
"""

from Dlg import GetNewDialog, SetDialogItemText, GetDialogItemText, ModalDialog
import Qd
import addpack
addpack.addpack('Tools')
addpack.addpack('bgen')
addpack.addpack('qd')
import QuickDraw


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
	d.SetDialogDefaultItem(1)
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
	d.SetDialogDefaultItem(1)
	d.SetDialogCancelItem(2)
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
	d.SetDialogCancelItem(4)
	if default in (2, 3, 4):
		d.SetDialogDefaultItem(default)
	while 1:
		n = ModalDialog(None)
		if n == 1: return default
		if n == 2: return 1
		if n == 3: return 0
		if n == 4: return -1
		
class ProgressBar:
	def __init__(self, label="Working...", maxval=100):
		self.label = label
		self.maxval = maxval
		self.curval = -1
		self.d = GetNewDialog(259, -1)
		tp, text_h, rect = self.d.GetDialogItem(2)
		SetDialogItemText(text_h, "Progress...")
		self._update(0)
		
	def _update(self, value):
		tp, h, bar_rect = self.d.GetDialogItem(3)
		Qd.SetPort(self.d)
		
		Qd.FrameRect(bar_rect)	# Draw outline
		
		inner_rect = Qd.InsetRect(bar_rect, 1, 1)
		Qd.ForeColor(QuickDraw.whiteColor)
		Qd.BackColor(QuickDraw.whiteColor)
		Qd.PaintRect(inner_rect)	# Clear internal
		
		l, t, r, b = inner_rect
		r = int(l + (r-l)*value/self.maxval)
		inner_rect = l, t, r, b
		Qd.ForeColor(QuickDraw.blackColor)
		Qd.BackColor(QuickDraw.blackColor)
		Qd.PaintRect(inner_rect)	# Draw bar
		
		# Restore settings
		Qd.ForeColor(QuickDraw.blackColor)
		Qd.BackColor(QuickDraw.whiteColor)
		
		# Test for cancel button
		if ModalDialog(self._filterfunc) == 1:
			raise KeyboardInterrupt
			
	def _filterfunc(self, d, e, *more):
		return 2 # XXXX For now, this disables the cancel button
				
	def set(self, value):
		if value < 0: value = 0
		if value > self.maxval: value = self.maxval
		self._update(value)
		


def test():
	Message("Testing EasyDialogs.")
	ok = AskYesNoCancel("Do you want to proceed?")
	if ok > 0:
		s = AskString("Enter your first name")
		Message("Thank you,\015%s" % `s`)
	bar = ProgressBar("Counting...", 100)
	for i in range(100):
		bar.set(i)
	del bar


if __name__ == '__main__':
	test()
