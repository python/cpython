"""Import a module while pretending its name is __main__. This
can be used to run scripts from the PackedLib resource file while pretending
they have been double-clicked."""

import imp
import sys
import os
import string
import Dlg
import macfs

DIALOG_ID = 512
OK = 1
CANCEL = 2
SCRIPTNAME=3
ARGV=4
STDIN_CONS=5
STDIN_FILE=6
STDOUT_CONS=7
STDOUT_FILE=8
WORKING_DIR=9
PAUSE=10

def import_as_main(name):
	fp, path, (suffix, mode, type) = imp.find_module(name)
	if type == imp.PY_SOURCE:
		imp.load_source('__main__', path, fp)
	elif type == imp.PY_COMPILED:
		imp.load_compiled('__main__', path, fp)
	elif type == imp.PY_RESOURCE:
		imp.load_resource('__main__', path)
		
def interact():
	d = Dlg.GetNewDialog(DIALOG_ID, -1)
	wdir = stdin = stdout = None
	pause = 0

	tp, in_c_h, rect = d.GetDialogItem(STDIN_CONS)
	tp, in_f_h, rect = d.GetDialogItem(STDIN_FILE)
	tp, out_c_h, rect = d.GetDialogItem(STDOUT_CONS)
	tp, out_f_h, rect = d.GetDialogItem(STDOUT_FILE)
	tp, pause_h, rect = d.GetDialogItem(PAUSE)
	in_c_h = in_c_h.as_Control()
	in_f_h = in_f_h.as_Control()
	out_c_h = out_c_h.as_Control()
	out_f_h = out_f_h.as_Control()
	pause_h = pause_h.as_Control()

	while 1:
		in_c_h.SetControlValue(not stdin)
		in_f_h.SetControlValue(not not stdin)
		out_c_h.SetControlValue(not stdout)
		out_f_h.SetControlValue(not not stdout)
		pause_h.SetControlValue(pause)
		
		n = Dlg.ModalDialog(None)
		if n == OK:
			break
		elif n == CANCEL:
			sys.exit(0)
		elif n == STDIN_CONS:
			stdin = None
		elif n == STDIN_FILE:
			fss, ok = macfs.StandardGetFile('TEXT')
			if ok:
				stdin = fss
		elif n == STDOUT_FILE:
			fss, ok = macfs.StandardPutFile('stdout:')
			if ok:
				stdout = fss
		elif n == WORKING_DIR:
			fss, ok = macfs.GetDirectory()
			if ok:
				wdir = fss
		elif n == PAUSE:
			pause = (not pause)
		
	tp, h, rect = d.GetDialogItem(SCRIPTNAME)
	name = Dlg.GetDialogItemText(h)
	tp, h, rect = d.GetDialogItem(ARGV)
	argv = Dlg.GetDialogItemText(h)
	return name, argv, stdin, stdout, wdir, pause
	
def main():
	curdir = os.getcwd()
	import Res
	try:
		Res.FSpOpenResFile('RunLibScript.rsrc', 1)
	except:
		pass # Assume we're an applet already
	name, argv, stdin, stdout, wdir, pause = interact()
	if not name:
		sys.exit(0)
	sys.argv = [name] + string.split(argv)
	if stdin:
		sys.stdin = open(stdin.as_pathname())
	if stdout:
		sys.stdout = open(stdout.as_pathname(), 'w')
	if wdir:
		os.chdir(wdir.as_pathname())
	else:
		os.chdir(curdir)

	import_as_main(name)

	if pause:
		sys.exit(1)
	
if __name__ == '__main__':
	main()
			
