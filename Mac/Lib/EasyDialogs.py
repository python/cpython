"""Easy to use dialogs.

Message(msg) -- display a message and an OK button.
AskString(prompt, default) -- ask for a string, display OK and Cancel buttons.
AskPassword(prompt, default) -- like AskString(), but shows text as bullets.
AskYesNoCancel(question, default) -- display a question and Yes, No and Cancel buttons.
bar = Progress(label, maxvalue) -- Display a progress bar
bar.set(value) -- Set value
bar.inc( *amount ) -- increment value by amount (default=1)
bar.label( *newlabel ) -- get or set text label. 

More documentation in each function.
This module uses DLOG resources 260 and on.
Based upon STDWIN dialogs with the same names and functions.
"""

from Dlg import GetNewDialog, SetDialogItemText, GetDialogItemText, ModalDialog
import Qd
import QuickDraw
import Dialogs
import Windows
import Dlg,Win,Evt,Events # sdm7g
import Ctl
import MacOS
import string
from ControlAccessor import *	# Also import Controls constants

def cr2lf(text):
	if '\r' in text:
		text = string.join(string.split(text, '\r'), '\n')
	return text

def lf2cr(text):
	if '\n' in text:
		text = string.join(string.split(text, '\n'), '\r')
	if len(text) > 253:
		text = text[:253] + '\311'
	return text

def Message(msg, id=260, ok=None):
	"""Display a MESSAGE string.
	
	Return when the user clicks the OK button or presses Return.
	
	The MESSAGE string can be at most 255 characters long.
	"""
	
	d = GetNewDialog(id, -1)
	if not d:
		print "Can't get DLOG resource with id =", id
		return
	h = d.GetDialogItemAsControl(2)
	SetDialogItemText(h, lf2cr(msg))
	if ok != None:
		h = d.GetDialogItemAsControl(1)
		h.SetControlTitle(ok)
	d.SetDialogDefaultItem(1)
	while 1:
		n = ModalDialog(None)
		if n == 1:
			return


def AskString(prompt, default = "", id=261, ok=None, cancel=None):
	"""Display a PROMPT string and a text entry field with a DEFAULT string.
	
	Return the contents of the text entry field when the user clicks the
	OK button or presses Return.
	Return None when the user clicks the Cancel button.
	
	If omitted, DEFAULT is empty.
	
	The PROMPT and DEFAULT strings, as well as the return value,
	can be at most 255 characters long.
	"""
	
	d = GetNewDialog(id, -1)
	if not d:
		print "Can't get DLOG resource with id =", id
		return
	h = d.GetDialogItemAsControl(3)
	SetDialogItemText(h, lf2cr(prompt))
	h = d.GetDialogItemAsControl(4)
	SetDialogItemText(h, lf2cr(default))
	d.SelectDialogItemText(4, 0, 999)
#	d.SetDialogItem(4, 0, 255)
	if ok != None:
		h = d.GetDialogItemAsControl(1)
		h.SetControlTitle(ok)
	if cancel != None:
		h = d.GetDialogItemAsControl(2)
		h.SetControlTitle(cancel)
	d.SetDialogDefaultItem(1)
	d.SetDialogCancelItem(2)
	while 1:
		n = ModalDialog(None)
		if n == 1:
			h = d.GetDialogItemAsControl(4)
			return cr2lf(GetDialogItemText(h))
		if n == 2: return None

def AskPassword(prompt,	 default='', id=264, ok=None, cancel=None):	
	"""Display a PROMPT string and a text entry field with a DEFAULT string.
	The string is displayed as bullets only.
	
	Return the contents of the text entry field when the user clicks the
	OK button or presses Return.
	Return None when the user clicks the Cancel button.
	
	If omitted, DEFAULT is empty.
	
	The PROMPT and DEFAULT strings, as well as the return value,
	can be at most 255 characters long.
	"""
	d = GetNewDialog(id, -1)
	if not d:
		print "Can't get DLOG resource with id =", id
		return
	h = d.GetDialogItemAsControl(3)
	SetDialogItemText(h, lf2cr(prompt))	
	pwd = d.GetDialogItemAsControl(4)
	bullets = '\245'*len(default)
##	SetControlData(pwd, kControlEditTextPart, kControlEditTextTextTag, bullets)
	SetControlData(pwd, kControlEditTextPart, kControlEditTextPasswordTag, default)
	d.SelectDialogItemText(4, 0, 999)
	Ctl.SetKeyboardFocus(d, pwd, kControlEditTextPart)
	if ok != None:
		h = d.GetDialogItemAsControl(1)
		h.SetControlTitle(ok)
	if cancel != None:
		h = d.GetDialogItemAsControl(2)
		h.SetControlTitle(cancel)
	d.SetDialogDefaultItem(Dialogs.ok)
	d.SetDialogCancelItem(Dialogs.cancel)
	while 1:
		n = ModalDialog(None)
		if n == 1:
			h = d.GetDialogItemAsControl(4)
			return cr2lf(GetControlData(pwd, kControlEditTextPart, kControlEditTextPasswordTag))
		if n == 2: return None

def AskYesNoCancel(question, default = 0, yes=None, no=None, cancel=None, id=262):
	"""Display a QUESTION string which can be answered with Yes or No.
	
	Return 1 when the user clicks the Yes button.
	Return 0 when the user clicks the No button.
	Return -1 when the user clicks the Cancel button.
	
	When the user presses Return, the DEFAULT value is returned.
	If omitted, this is 0 (No).
	
	The QUESTION strign ca be at most 255 characters.
	"""
	
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
	h = d.GetDialogItemAsControl(5)
	SetDialogItemText(h, lf2cr(question))
	if yes != None:
		h = d.GetDialogItemAsControl(2)
		h.SetControlTitle(yes)
	if no != None:
		h = d.GetDialogItemAsControl(3)
		h.SetControlTitle(no)
	if cancel != None:
		if cancel == '':
			d.HideDialogItem(4)
		else:
			h = d.GetDialogItemAsControl(4)
			h.SetControlTitle(cancel)
	d.SetDialogCancelItem(4)
	if default == 1:
		d.SetDialogDefaultItem(2)
	elif default == 0:
		d.SetDialogDefaultItem(3)
	elif default == -1:
		d.SetDialogDefaultItem(4)
	while 1:
		n = ModalDialog(None)
		if n == 1: return default
		if n == 2: return 1
		if n == 3: return 0
		if n == 4: return -1


		

screenbounds = Qd.qd.screenBits.bounds
screenbounds = screenbounds[0]+4, screenbounds[1]+4, \
	screenbounds[2]-4, screenbounds[3]-4

				
class ProgressBar:
	def __init__(self, title="Working...", maxval=100, label="", id=263):
		self.maxval = maxval
		self.curval = -1
		self.d = GetNewDialog(id, -1)
		self.title(title)
		self.label(label)
		self._update(0)
		self.d.DrawDialog()

	def __del__( self ):
		self.d.BringToFront()
		self.d.HideWindow()
		del self.d
		
	def title(self, newstr=""):
		"""title(text) - Set title of progress window"""
		self.d.BringToFront()
		w = self.d.GetDialogWindow()
		w.SetWTitle(newstr)
		
	def label( self, *newstr ):
		"""label(text) - Set text in progress box"""
		self.d.BringToFront()
		if newstr:
			self._label = lf2cr(newstr[0])
		text_h = self.d.GetDialogItemAsControl(2)
		SetDialogItemText(text_h, self._label)		
				
	def _update(self, value):
		maxval = self.maxval
		if maxval == 0:
			# XXXX Quick fix. Should probably display an unknown duration
			value = 0
			maxval = 1
		if maxval > 32767:
			value = int(value/(maxval/32767.0))
			maxval = 32767
		progbar = self.d.GetDialogItemAsControl(3)
		progbar.SetControlMaximum(maxval)
		progbar.SetControlValue(value)	
		# Test for cancel button
		
		ready, ev = Evt.WaitNextEvent( Events.mDownMask, 1  )
		if ready : 
			what,msg,when,where,mod = ev
			part = Win.FindWindow(where)[0]
			if Dlg.IsDialogEvent(ev):
				ds = Dlg.DialogSelect(ev)
				if ds[0] and ds[1] == self.d and ds[-1] == 1:
					raise KeyboardInterrupt, ev
			else:
				if part == 4:	# inDrag 
					self.d.DragWindow(where, screenbounds)
				else:
					MacOS.HandleEvent(ev) 
			
			
	def set(self, value, max=None):
		"""set(value) - Set progress bar position"""
		if max != None:
			self.maxval = max
		if value < 0: value = 0
		if value > self.maxval: value = self.maxval
		self.curval = value
		self._update(value)

	def inc(self, n=1):
		"""inc(amt) - Increment progress bar position"""
		self.set(self.curval + n)

def test():
	import time

	Message("Testing EasyDialogs.")
	ok = AskYesNoCancel("Do you want to proceed?")
	ok = AskYesNoCancel("Do you want to identify?", yes="Identify", no="No")
	if ok > 0:
		s = AskString("Enter your first name", "Joe")
		s2 = AskPassword("Okay %s, tell us your nickname"%s, s, cancel="None")
		if not s2:
			Message("%s has no secret nickname"%s)
		else:
			Message("Hello everybody!!\nThe secret nickname of %s is %s!!!"%(s, s2))
	text = ( "Working Hard...", "Hardly Working..." , 
			"So far, so good!", "Keep on truckin'" )
	bar = ProgressBar("Progress, progress...", 100)
	try:
		appsw = MacOS.SchedParams(1, 0)
		for i in range(100):
			bar.set(i)
			time.sleep(0.1)
			if i % 10 == 0:
				bar.label(text[(i/10) % 4])
		bar.label("Done.")
		time.sleep(0.3) 	# give'em a chance to see the done.
	finally:
		del bar
		apply(MacOS.SchedParams, appsw)


	
	
if __name__ == '__main__':
	try:
		test()
	except KeyboardInterrupt:
		Message("Operation Canceled.")

