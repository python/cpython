"""Edit the Python Preferences file."""
#
# This program is getting more and more clunky. It should really
# be rewritten in a modeless way some time soon.

from Dlg import *
from Events import *
from Res import *
import string
import struct
import macfs
import MacOS
import os
import sys
import Res # For Res.Error
import pythonprefs
import EasyDialogs
import Help

# resource IDs in our own resources (dialogs, etc)
MESSAGE_ID = 256

DIALOG_ID = 511
TEXT_ITEM = 1
OK_ITEM = 2
CANCEL_ITEM = 3
DIR_ITEM = 4
TITLE_ITEM = 5
OPTIONS_ITEM = 7
HELP_ITEM = 9

# The options dialog. There is a correspondence between
# the dialog item numbers and the option.
OPT_DIALOG_ID = 510

# Map dialog item numbers to option names (and the reverse)
opt_dialog_map = [
	None,
	"inspect",
	"verbose",
	"optimize",
	"unbuffered",
	"debugging",
	"keepopen",
	"keeperror",
	"nointopt",
	"noargs",
	"delayconsole",
	None, None, None, None, None, None, None, None, # 11-18 are different
	"oldexc",
	"nosite"]
opt_dialog_dict = {}
for i in range(len(opt_dialog_map)):
	if opt_dialog_map[i]:
		opt_dialog_dict[opt_dialog_map[i]] = i
# 1 thru 10 are the options
# The GUSI creator/type and delay-console
OD_CREATOR_ITEM = 11
OD_TYPE_ITEM = 12
OD_OK_ITEM = 13
OD_CANCEL_ITEM = 14
OD_HELP_ITEM = 22

def optinteract(options):
	"""Let the user interact with the options dialog"""
	d = GetNewDialog(OPT_DIALOG_ID, -1)
	tp, h, rect = d.GetDialogItem(OD_CREATOR_ITEM)
	SetDialogItemText(h, options['creator'])
	tp, h, rect = d.GetDialogItem(OD_TYPE_ITEM)
	SetDialogItemText(h, options['type'])
	d.SetDialogDefaultItem(OD_OK_ITEM)
	d.SetDialogCancelItem(OD_CANCEL_ITEM)
	
	while 1:
		for name in opt_dialog_dict.keys():
			num = opt_dialog_dict[name]
			tp, h, rect = d.GetDialogItem(num)
			h.as_Control().SetControlValue(options[name])
		n = ModalDialog(None)
		if n == OD_OK_ITEM:
			tp, h, rect = d.GetDialogItem(OD_CREATOR_ITEM)
			ncreator = GetDialogItemText(h)
			tp, h, rect = d.GetDialogItem(OD_TYPE_ITEM)
			ntype = GetDialogItemText(h)
			if len(ncreator) == 4 and len(ntype) == 4:
				options['creator'] = ncreator
				options['type'] = ntype
				return options
			else:
				MacOS.SysBeep()
		elif n == OD_CANCEL_ITEM:
			return
		elif n in (OD_CREATOR_ITEM, OD_TYPE_ITEM):
			pass
		elif n == OD_HELP_ITEM:
			onoff = Help.HMGetBalloons()
			Help.HMSetBalloons(not onoff)
		elif 1 <= n <= len(opt_dialog_map):
			options[opt_dialog_map[n]] = (not options[opt_dialog_map[n]])

			
def interact(options, title):
	"""Let the user interact with the dialog"""
	try:
		# Try to go to the "correct" dir for GetDirectory
		os.chdir(options['dir'].as_pathname())
	except os.error:
		pass
	d = GetNewDialog(DIALOG_ID, -1)
	tp, h, rect = d.GetDialogItem(TITLE_ITEM)
	SetDialogItemText(h, title)
	tp, h, rect = d.GetDialogItem(TEXT_ITEM)
##	SetDialogItemText(h, string.joinfields(list, '\r'))
	h.data = string.joinfields(options['path'], '\r')
	d.SelectDialogItemText(TEXT_ITEM, 0, 32767)
	d.SelectDialogItemText(TEXT_ITEM, 0, 0)
##	d.SetDialogDefaultItem(OK_ITEM)
	d.SetDialogCancelItem(CANCEL_ITEM)
	d.GetDialogWindow().ShowWindow()
	d.DrawDialog()
	while 1:
		n = ModalDialog(None)
		if n == OK_ITEM:
			break
		if n == CANCEL_ITEM:
			return None
##		if n == REVERT_ITEM:
##			return [], pythondir
		if n == DIR_ITEM:
			fss, ok = macfs.GetDirectory('Select python home folder:')
			if ok:
				options['dir'] = fss
		elif n == HELP_ITEM:
			onoff = Help.HMGetBalloons()
			Help.HMSetBalloons(not onoff)
		if n == OPTIONS_ITEM:
			noptions = options
			for k in options.keys():
				noptions[k] = options[k]
			noptions = optinteract(noptions)
			if noptions:
				options = noptions
	tmp = string.splitfields(h.data, '\r')
	newpath = []
	for i in tmp:
		if i:
			newpath.append(i)
	options['path'] = newpath
	return options
	
	
def edit_preferences():
	handler = pythonprefs.PythonOptions()
	result = interact(handler.load(), 'System-wide preferences')
	if result:
		handler.save(result)
	
def edit_applet(name):
	handler = pythonprefs.AppletOptions(name)
	result = interact(handler.load(), os.path.split(name)[1])
	if result:
		handler.save(result)

def main():
	try:
		h = OpenResFile('EditPythonPrefs.rsrc')
	except Res.Error:
		pass	# Assume we already have acces to our own resource
	
	if len(sys.argv) <= 1:
		edit_preferences()
	else:
		for appl in sys.argv[1:]:
			edit_applet(appl)
		

if __name__ == '__main__':
	main()
