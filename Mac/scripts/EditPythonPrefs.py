"""Edit the Python Preferences file."""
import addpack
addpack.addpack('Tools')
addpack.addpack('bgen')
addpack.addpack('evt')

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
REVERT_ITEM = 4
DIR_ITEM = 5

# Resource IDs in the preferences file
PATH_STRINGS_ID = 128
DIRECTORY_ID = 128

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
		
def interact(list, pythondir):
	"""Let the user interact with the dialog"""
	opythondir = pythondir
	try:
		# Try to go to the "correct" dir for GetDirectory
		os.chdir(pythondir.as_pathname())
	except os.error:
		pass
	d = GetNewDialog(DIALOG_ID, -1)
	tp, h, rect = d.GetDialogItem(1)
	SetDialogItemText(h, string.joinfields(list, '\r'))
	while 1:
		n = ModalDialog(None)
		if n == OK_ITEM:
			break
		if n == CANCEL_ITEM:
			return None
		if n == REVERT_ITEM:
			return [], pythondir
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
	
def main():
	try:
		h = OpenResFile('EditPythonPrefs.rsrc')
	except Res.Error:
		pass	# Assume we already have acces to our own resource
	
	# Find the preferences folder and our prefs file, create if needed.	
	vrefnum, dirid = macfs.FindFolder(kOnSystemDisk, 'pref', 0)
	preff_fss = macfs.FSSpec((vrefnum, dirid, 'Python Preferences'))
	try:
		preff_handle = FSpOpenResFile(preff_fss, WRITE)
	except Res.Error:
		# Create it
		message('No preferences file, creating one...')
		FSpCreateResFile(preff_fss, 'PYTH', 'pref', smAllScripts)
		preff_handle = FSpOpenResFile(preff_fss, WRITE)
		
	# Load the path and directory resources
	try:
		sr = GetResource('STR#', PATH_STRINGS_ID)
	except (MacOS.Error, Res.Error):
		message('Cannot find any sys.path resource! (Old python?)')
		sys.exit(0)
	d = sr.data
	l = restolist(d)
	
	try:
		dr = GetResource('alis', DIRECTORY_ID)
		fss, fss_changed = macfs.RawAlias(dr.data).Resolve()
	except (MacOS.Error, Res.Error):
		dr = None
		fss = macfs.FSSpec(os.getcwd())
		fss_changed = 1
	
	# Let the user play away
	result = interact(l, fss)
	
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

if __name__ == '__main__':
	print # Stupid, to init toolboxes...
	main()
