"""Edit the Python Preferences file."""

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

# resource IDs in our own resources (dialogs, etc)
MESSAGE_ID = 256

DIALOG_ID = 512
TEXT_ITEM = 1
OK_ITEM = 2
CANCEL_ITEM = 3
DIR_ITEM = 4
TITLE_ITEM = 5

# Resource IDs in the preferences file
PATH_STRINGS_ID = 128
DIRECTORY_ID = 128
OPTIONS_ID = 128

# Override IDs (in the applet)
OVERRIDE_PATH_STRINGS_ID = 129
OVERRIDE_DIRECTORY_ID = 129
OVERRIDE_OPTIONS_ID = 129

READ = 1
WRITE = 2
smAllScripts = -3
kOnSystemDisk = 0x8000

def restolist(data):
	"""Convert STR# resource data to a list of strings"""
	if not data:
		return []
	num, = struct.unpack('h', data[:2])
	data = data[2:]
	rv = []
	for i in range(num):
		strlen = ord(data[0])
		if strlen < 0: strlen = strlen + 256
		str = data[1:strlen+1]
		data = data[strlen+1:]
		rv.append(str)
	return rv
	
def listtores(list):
	"""Convert a list of strings to STR# resource data"""
	rv = struct.pack('h', len(list))
	for str in list:
		rv = rv + chr(len(str)) + str
	return rv

def message(str = "Hello, world!", id = MESSAGE_ID):
	"""Show a simple alert with a text message"""
	d = GetNewDialog(id, -1)
	print 'd=', d
	tp, h, rect = d.GetDialogItem(2)
	SetDialogItemText(h, str)
	while 1:
		n = ModalDialog(None)
		if n == 1: break
		
def interact(list, pythondir, title):
	"""Let the user interact with the dialog"""
	opythondir = pythondir
	try:
		# Try to go to the "correct" dir for GetDirectory
		os.chdir(pythondir.as_pathname())
	except os.error:
		pass
	d = GetNewDialog(DIALOG_ID, -1)
	tp, h, rect = d.GetDialogItem(TITLE_ITEM)
	SetDialogItemText(h, title)
	tp, h, rect = d.GetDialogItem(TEXT_ITEM)
	SetDialogItemText(h, string.joinfields(list, '\r'))
##	d.SetDialogDefaultItem(OK_ITEM)
	d.SetDialogCancelItem(CANCEL_ITEM)
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
				pythondir = fss
	tmp = string.splitfields(GetDialogItemText(h), '\r')
	rv = []
	for i in tmp:
		if i:
			rv.append(i)
	return rv, pythondir
	
def getprefpath(id):
	# Load the path and directory resources
	try:
		sr = GetResource('STR#', id)
	except (MacOS.Error, Res.Error):
		return None, None
	d = sr.data
	l = restolist(d)
	return l, sr

def getprefdir(id):
	try:
		dr = GetResource('alis', id)
		fss, fss_changed = macfs.RawAlias(dr.data).Resolve()
	except (MacOS.Error, Res.Error):
		return None, None, 1
	return fss, dr, fss_changed
	
def openpreffile(rw):
	# Find the preferences folder and our prefs file, create if needed.	
	vrefnum, dirid = macfs.FindFolder(kOnSystemDisk, 'pref', 0)
	preff_fss = macfs.FSSpec((vrefnum, dirid, 'Python Preferences'))
	try:
		preff_handle = FSpOpenResFile(preff_fss, rw)
	except Res.Error:
		# Create it
		message('No preferences file, creating one...')
		FSpCreateResFile(preff_fss, 'Pyth', 'pref', smAllScripts)
		preff_handle = FSpOpenResFile(preff_fss, rw)
	return preff_handle
	
def openapplet(name):
	fss = macfs.FSSpec(name)
	try:
		app_handle = FSpOpenResFile(fss, WRITE)
	except Res.Error:
		message('File does not have a resource fork.')
		sys.exit(0)
	return app_handle
		
	
def edit_preferences():
	preff_handle = openpreffile(WRITE)
	
	l, sr = getprefpath(PATH_STRINGS_ID)
	if l == None:	
		message('Cannot find any sys.path resource! (Old python?)')
		sys.exit(0)
		
	fss, dr, fss_changed = getprefdir(DIRECTORY_ID)
	if fss == None:
		fss = macfs.FSSpec(os.getcwd())
		fss_changed = 1
	
	# Let the user play away
	result = interact(l, fss, 'System-wide preferences')
	
	# See what we have to update, and how
	if result == None:
		sys.exit(0)
		
	pathlist, nfss = result
	if nfss != fss:
		fss_changed = 1
		
	if fss_changed or pathlist != l:
		if fss_changed:
			alias = nfss.NewAlias()
			if dr:
				dr.data = alias.data
				dr.ChangedResource()
			else:
				dr = Resource(alias.data)
				dr.AddResource('alis', DIRECTORY_ID, '')
				
		if pathlist != l:
			if pathlist == []:
				if sr.HomeResFile() == preff_handle:
					sr.RemoveResource()
			elif sr.HomeResFile() == preff_handle:
				sr.data = listtores(pathlist)
				sr.ChangedResource()
			else:
				sr = Resource(listtores(pathlist))
				sr.AddResource('STR#', PATH_STRINGS_ID, '')
				
	CloseResFile(preff_handle)
	
def edit_applet(name):
	pref_handle = openpreffile(READ)
	app_handle = openapplet(name)
	
	notfound = ''
	l, sr = getprefpath(OVERRIDE_PATH_STRINGS_ID)
	if l == None:
		notfound = 'path'
		
		l, dummy = getprefpath(PATH_STRINGS_ID)
		if l == None:	
			message('Cannot find any sys.path resource! (Old python?)')
			sys.exit(0)
		
	fss, dr, fss_changed = getprefdir(OVERRIDE_DIRECTORY_ID)
	if fss == None:
		if notfound:
			notfound = notfound + ' and ' + 'directory'
		else:
			notfound = 'directory'
		fss, dummy, dummy2 = getprefdir(DIRECTORY_ID)
		if fss == None:
			fss = macfs.FSSpec(os.getcwd())
			fss_changed = 1
	
	dummy = dummy2 = None # Discard them.
	
	if notfound:
		message('Warning: initial %s taken from system-wide defaults'%notfound)
	# Let the user play away
	result = interact(l, fss, name)
	
	# See what we have to update, and how
	if result == None:
		sys.exit(0)
		
	pathlist, nfss = result
	if nfss != fss:
		fss_changed = 1
		
	if fss_changed or pathlist != l:
		if fss_changed:
			alias = nfss.NewAlias()
			if dr:
				dr.data = alias.data
				dr.ChangedResource()
			else:
				dr = Resource(alias.data)
				dr.AddResource('alis', OVERRIDE_DIRECTORY_ID, '')
				
		if pathlist != l:
			if pathlist == []:
				if sr.HomeResFile() == app_handle:
					sr.RemoveResource()
			elif sr and sr.HomeResFile() == app_handle:
				sr.data = listtores(pathlist)
				sr.ChangedResource()
			else:
				sr = Resource(listtores(pathlist))
				sr.AddResource('STR#', OVERRIDE_PATH_STRINGS_ID, '')
				
	CloseResFile(app_handle)

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
	print # Stupid, to init toolboxes...
	main()
