#! /usr/local/python

# TestDirList

from DirList import DirListWindow
from WindowParent import MainLoop

def main():
	import sys
	args = sys.argv[1:]
	if not args:
		args = ['.']
		# Mac: args = [':']
	for arg in args:
		w = DirListWindow().create(arg)
	MainLoop()

main()
