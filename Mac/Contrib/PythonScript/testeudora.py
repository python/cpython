"""A test program that allows us to control Eudora"""

import sys
import MacOS
import PythonScript
	
# The Creator signature of eudora:
SIGNATURE="CSOm"
TIMEOUT = 10*60*60



def main():
	PythonScript.PsScript(SIGNATURE, TIMEOUT)
	talker = PythonScript.PyScript
	ev = PythonScript.PsEvents
	pc = PythonScript.PsClass
	while 1:
		print 'get, put, name (of first folder), list (foldernames), quit (eudora) or exit (this program) ?'
		line = sys.stdin.readline()
		try:
			if line[0] == 'g':
				print 'check'
				print talker(ev.Activate)
				print talker(ev.Connect, Checking=1)
			elif line[0] == 'p':
				print talker(ev.Connect, Sending=1)
			elif line[0] == 'n':
				id = talker(ev.Get, pc.Mail_folder("").Mailbox(1).Name())
				print "It is called", id, "\n"
			elif line[0] == 'l':
				id = talker(ev.Count, pc.Mail_folder(""), Each='Mailbox')
				print "There are", id, "mailboxes"
			elif line[0] == 'q':
				print talker(ev.Quit)
			elif line[0] == 'e':
				break
		except MacOS.Error, arg:
			if arg[0] == -609:
				print 'Connection invalid, is eudora running?'
			else:
				print 'MacOS Error:', arg[1]
			
main()
