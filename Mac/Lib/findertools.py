"""Utility routines depending on the finder."""

import Finder
import AppleEvents
import aetools
import MacOS
import sys
import macfs

_finder_talker = None

def _getfinder():
	global _finder_talker
	if not _finder_talker:
		_finder_talker = Finder.Finder()
	_finder_talker.send_flags = ( _finder_talker.send_flags | 
		AppleEvents.kAECanInteract | AppleEvents.kAECanSwitchLayer)
	return _finder_talker
	
def launch(file):
	"""Open a file thru the finder. Specify file by name or fsspec"""
	finder = _getfinder()
	fss = macfs.FSSpec(file)
	return finder.open(fss)
	
def Print(file):
	"""Print a file thru the finder. Specify file by name or fsspec"""
	finder = _getfinder()
	fss = macfs.FSSpec(file)
	return finder._print(fss)
	
def copy(src, dstdir):
	"""Copy a file to a folder"""
	finder = _getfinder()
	if type(src) == type([]):
		src_fss = []
		for s in src:
			src_fss.append(macfs.FSSpec(s))
	else:
		src_fss = macfs.FSSpec(src)
	dst_fss = macfs.FSSpec(dstdir)
	return finder.duplicate(src_fss, to=dst_fss)

def move(src, dstdir):
	"""Move a file to a folder"""
	finder = _getfinder()
	if type(src) == type([]):
		src_fss = []
		for s in src:
			src_fss.append(macfs.FSSpec(s))
	else:
		src_fss = macfs.FSSpec(src)
	dst_fss = macfs.FSSpec(dstdir)
	return finder.move(src_fss, to=dst_fss)
	
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
	
