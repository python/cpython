"""Utility routines depending on the finder."""

import Finder_7_0_Suite
import AppleEvents
import aetools
import MacOS
import sys
import macfs

SIGNATURE='MACS'

class Finder(aetools.TalkTo, Finder_7_0_Suite.Finder_7_0_Suite):
	pass
	
_finder_talker = None

def _getfinder():
	global _finder_talker
	if not _finder_talker:
		_finder_talker = Finder(SIGNATURE)
	_finder_talker.send_flags = ( _finder_talker.send_flags | 
		AppleEvents.kAECanInteract | AppleEvents.kAECanSwitchLayer)
	return _finder_talker
	
def launch(file):
	"""Open a file thru the finder. Specify file by name or fsspec"""
	finder = _getfinder()
	fss = macfs.FSSpec(file)
	vRefNum, parID, name = fss.as_tuple()
	dir_fss = macfs.FSSpec((vRefNum, parID, ''))
	file_alias = fss.NewAlias()
	dir_alias = dir_fss.NewAlias()
	return finder.open(dir_alias, items=[file_alias])
	
def Print(file):
	"""Print a file thru the finder. Specify file by name or fsspec"""
	finder = _getfinder()
	fss = macfs.FSSpec(file)
	vRefNum, parID, name = fss.as_tuple()
	dir_fss = macfs.FSSpec((vRefNum, parID, ''))
	file_alias = fss.NewAlias()
	dir_alias = dir_fss.NewAlias()
	return finder._print(dir_alias, items=[file_alias])
	
def copy(src, dstdir):
	"""Copy a file to a folder"""
	finder = _getfinder()
	src_fss = macfs.FSSpec(src)
	dst_fss = macfs.FSSpec(dstdir)
	src_alias = src_fss.NewAlias()
	dst_alias = dst_fss.NewAlias()
	return finder.copy_to(dst_alias, _from=[src_alias])

def move(src, dstdir):
	"""Move a file to a folder"""
	finder = _getfinder()
	src_fss = macfs.FSSpec(src)
	dst_fss = macfs.FSSpec(dstdir)
	src_alias = src_fss.NewAlias()
	dst_alias = dst_fss.NewAlias()
	return finder.move_to(dst_alias, _from=[src_alias])
	
def sleep():
	"""Put the mac to sleep"""
	finder = _getfinder()
	finder.sleep()
	
def shutdown():
	"""Shut the mac down"""
	finder = _getfinder()
	finder.shut_down()
	
def restart():
	"""Restart the mac"""
	finder = _getfinder()
	finder.restart()
	

def main():
	print 'Testing launch...'
	fss, ok = macfs.PromptGetFile('File to launch:')
	if ok:
		result = launch(fss)
		if result:
			print 'Result: ', result
		print 'Press return-',
		sys.stdin.readline()
	print 'Testing print...'
	fss, ok = macfs.PromptGetFile('File to print:')
	if ok:
		result = Print(fss)
		if result:
			print 'Result: ', result
		print 'Press return-',
		sys.stdin.readline()
	print 'Testing copy...'
	fss, ok = macfs.PromptGetFile('File to copy:')
	if ok:
		dfss, ok = macfs.GetDirectory()
		if ok:
			result = copy(fss, dfss)
			if result:
				print 'Result:', result
			print 'Press return-',
			sys.stdin.readline()
	print 'Testing move...'
	fss, ok = macfs.PromptGetFile('File to move:')
	if ok:
		dfss, ok = macfs.GetDirectory()
		if ok:
			result = move(fss, dfss)
			if result:
				print 'Result:', result
			print 'Press return-',
			sys.stdin.readline()
	import EasyDialogs
	print 'Testing sleep...'
	if EasyDialogs.AskYesNoCancel('Sleep?') > 0:
		result = sleep()
		if result:
			print 'Result:', result
		print 'Press return-',
		sys.stdin.readline()
	print 'Testing shutdown...'
	if EasyDialogs.AskYesNoCancel('Shut down?') > 0:
		result = shutdown()
		if result:
			print 'Result:', result
		print 'Press return-',
		sys.stdin.readline()
	print 'Testing restart...'
	if EasyDialogs.AskYesNoCancel('Restart?') > 0:
		result = restart()
		if result:
			print 'Result:', result
		print 'Press return-',
		sys.stdin.readline()
		
if __name__ == '__main__':
	main()
	
