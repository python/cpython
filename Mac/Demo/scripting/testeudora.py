"""A test program that allows us to control Eudora"""

import sys
import addpack
addpack.addpack('Tools')
addpack.addpack('bgen')
addpack.addpack('ae')
import aetools
import Eudora_Suite
import Required_Suite
import MacOS

class Eudora(aetools.TalkTo, Required_Suite.Required_Suite, \
				Eudora_Suite.Eudora_Suite):
	"""A class that can talk to Eudora"""
	pass
	
# The Creator signature of eudora:
SIGNATURE="????"

def main():
	talker = Eudora(SIGNATURE)
	while 1:
		print 'get, put, quit (eudora) or exit (this program) ?'
		line = sys.stdin.readline()
		try:
			if line[0] == 'g':
				talker.connect(checking=1)
			elif line[0] == 'p':
				talker.connect(sending=1)
			elif line[0] == 'q':
				talker.quit()
			elif line[0] == 'e':
				break
		except MacOS.Error, arg:
			if arg[0] == -609:
				print 'Connection invalid, is eudora running?'
			else:
				print 'Error, possibly ', arg[1]
			
main()
