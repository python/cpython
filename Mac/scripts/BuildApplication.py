"""Create a standalone application from a Python script.

This puts up a dialog asking for a Python source file ('TEXT').
The output is a file with the same name but its ".py" suffix dropped.
It is created by copying an applet template, all used shared libs and 
then adding 'PYC ' resources containing compiled versions of all used 
modules written in Python and the main script itself, as __main__.
"""


import sys

import string
import os
import macfs
import MacOS
import Res
import Dlg
import EasyDialogs
import buildtools

# Hmmm...
MACFREEZEPATH = os.path.join(sys.prefix, ":Mac:Tools:macfreeze")
if MACFREEZEPATH not in sys.path:
	sys.path.append(MACFREEZEPATH)

import macgen_bin

# dialog, items
DLG_ID = 400
OK_BUTTON = 1
CANCEL_BUTTON = 2
GENFAT_BUTTON = 4
GENPPC_BUTTON = 5
GEN68K_BUTTON = 6

# Define this if we cannot generate 68/fat binaries (Python 1.6)
PPC_ONLY=1


try:
	Res.GetResource('DITL', DLG_ID)
except Res.Error:
	Res.FSpOpenResFile("BuildApplication.rsrc", 1)
else:
	pass # we're an applet


def main():
	try:
		buildapplication()
	except buildtools.BuildError, detail:
		EasyDialogs.Message(detail)


def buildapplication(debug = 0):
	buildtools.DEBUG = debug
	
	# Ask for source text if not specified in sys.argv[1:]
	
	if not sys.argv[1:]:
		srcfss, ok = macfs.PromptGetFile('Select Python source:', 'TEXT')
		if not ok:
			return
		filename = srcfss.as_pathname()
	else:
		if sys.argv[2:]:
			raise buildtools.BuildError, "please select one file at a time"
		filename = sys.argv[1]
	tp, tf = os.path.split(filename)
	
	# interact with user
	architecture, ok = interact(tf)
	if not ok:
		return
	if tf[-3:] == '.py':
		tf = tf[:-3]
	else:
		tf = tf + '.app'
	
	dstfss, ok = macfs.StandardPutFile('Save application as:', tf)
	if not ok:
		return
	dstfilename = dstfss.as_pathname()
	
	macgen_bin.generate(filename, dstfilename, None, architecture, 1)


class radio:
	
	def __init__(self, dlg, *items):
		self.items = {}
		for item in items:
			ctl = dlg.GetDialogItemAsControl(item)
			self.items[item] = ctl
	
	def set(self, setitem):
		for item, ctl in self.items.items():
			if item == setitem:
				ctl.SetControlValue(1)
			else:
				ctl.SetControlValue(0)
	
	def get(self):
		for item, ctl in self.items.items():
			if ctl.GetControlValue():
				return item
	
	def hasitem(self, item):
		return self.items.has_key(item)


def interact(scriptname):
	if PPC_ONLY:
		return 'pwpc', 1
	d = Dlg.GetNewDialog(DLG_ID, -1)
	if not d:
		raise "Can't get DLOG resource with id =", DLG_ID
	d.SetDialogDefaultItem(OK_BUTTON)
	d.SetDialogCancelItem(CANCEL_BUTTON)
	Dlg.ParamText(scriptname, "", "", "")
	
	radiogroup = radio(d, GENFAT_BUTTON, GENPPC_BUTTON, GEN68K_BUTTON)
	radiogroup.set(GENFAT_BUTTON)
	
	gentype = 'fat'
	while 1:
		n = Dlg.ModalDialog(None)
		if n == OK_BUTTON or n == CANCEL_BUTTON:
			break
		elif radiogroup.hasitem(n):
			radiogroup.set(n)
	genitem = radiogroup.get()
	del radiogroup
	del d
	if genitem == GENFAT_BUTTON:
		gentype = 'fat'
	elif genitem == GENPPC_BUTTON:
		gentype = 'pwpc'
	elif genitem == GEN68K_BUTTON:
		gentype = 'm68k'
	return gentype, n == OK_BUTTON


if __name__ == '__main__':
	main()
