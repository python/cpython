"""nsremote - Control Netscape from python.

Interface modelled after unix-interface done
by hassan@cs.stanford.edu.

Jack Jansen, CWI, January 1996.
"""
#
# Note: this module currently uses the funny SpyGlass AppleEvents, since
# these seem to be the only way to get the info from Netscape. It would
# be nicer to use the more "object oriented" standard OSA stuff, when it
# is implemented in Netscape.
#
import sys

import aetools
import Standard_Suite
import WWW_Suite
import MacOS

class Netscape(aetools.TalkTo, Standard_Suite.Standard_Suite, WWW_Suite.WWW_Suite):
	pass	
	
SIGNATURE='MOSS'

Error = 'nsremote.Error'

_talker = None

def _init():
	global _talker
	if _talker == None:
		_talker = Netscape(SIGNATURE)

def list(dpyinfo=""):
	_init()
	list = _talker.list_windows()
	return map(lambda x: (x, 'version unknown'), list)
	
def geturl(windowid=0, dpyinfo=""):
	_init()
	if windowid == 0:
		ids = _talker.list_windows()
		if not ids:
			raise Error, 'No netscape windows open'
		windowid = ids[0]
	info = _talker.get_window_info(windowid)
	return info
	
def openurl(url, windowid=0, dpyinfo=""):
	_init()
	if windowid == 0:
		_talker.OpenURL(url)
	else:
		_talker.OpenURL(url, toWindow=windowid)
		
def _test():
	"""Test program: Open www.python.org in all windows, then revert"""
	import sys
	windows_and_versions = list()
	windows_and_urls = map(lambda x: (x[0], geturl(x[0])[0]), windows_and_versions)
	for id, version in windows_and_versions:
		openurl('http://www.python.org/', windowid=id)
	print 'Type return to revert to old contents-'
	sys.stdin.readline()
	for id, url in windows_and_urls:
		openurl(url, id)
		
if __name__ == '__main__':
	_test()
	
