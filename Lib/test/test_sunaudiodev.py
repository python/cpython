from test_support import verbose, TestFailed
import sunaudiodev
import os

def findfile(file):
	if os.path.isabs(file): return file
	import sys
	path = sys.path
	try:
		path = [os.path.dirname(__file__)] + path
	except NameError:
		pass
	for dn in path:
		fn = os.path.join(dn, file)
		if os.path.exists(fn): return fn
	return file

def play_sound_file(path):
    fp = open(path, 'r')
    data = fp.read()
    fp.close()
    try:
	a = sunaudiodev.open('w')
    except sunaudiodev.error, msg:
	raise TestFailed, msg
    else:
	a.write(data)
	a.close()

def test():
    play_sound_file(findfile('audiotest.au'))

test()
