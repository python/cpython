"""A test program that allows us to control Eudora"""

import sys
import aetools
import Eudora_Suite
import Required_Suite
import Standard_Suite
import MacOS

class Eudora(Eudora_Suite.Eudora_Suite, Required_Suite.Required_Suite, \
				Standard_Suite.Standard_Suite, aetools.TalkTo):
	"""A class that can talk to Eudora"""
	pass
	
# The Creator signature of eudora:
SIGNATURE="CSOm"

def main():
	talker = Eudora(SIGNATURE, start=1)
	while 1:
		print 'get, put, name (of first folder), list (foldernames), quit (eudora) or exit (this program) ?'
		line = sys.stdin.readline()
		try:
			if line[0] == 'g':
				talker.connect(checking=1)
			elif line[0] == 'p':
				talker.connect(sending=1)
			elif line[0] == 'n':
				id = Eudora_Suite.mailbox(1).name
				name = talker._get(id, as='text')
				print "It is called", name
			elif line[0] == 'l':
				pass
			elif line[0] == 'q':
				talker.quit()
			elif line[0] == 'e':
				break
		except MacOS.Error, arg:
			if arg[0] == -609:
				print 'Connection invalid, is eudora running?'
			else:
				print 'MacOS Error:', arg[1]
		except aetools.Error, arg:
			print 'Eudora returned error:', arg
			
main()
