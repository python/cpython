from test_support import verbose, TestFailed
import sunaudiodev
import os

def findfile(file):
	if os.path.isabs(file): return file
	import sys
	for dn in sys.path:
		fn = os.path.join(dn, file)
		if os.path.exists(fn): return fn
	return file

def play_sound_file(path):
    fp = open(path, 'r')
    data = fp.read()
    fp.close()
    a = sunaudiodev.open('w')
    a.write(data)
    a.close()

def test():
    print os.getcwd()
    play_sound_file(findfile('audiotest.au'))

test()
