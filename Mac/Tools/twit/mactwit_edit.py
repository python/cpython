"""Edit a file using the MetroWerks editor. Modify to suit your needs"""
 
import MacOS
import aetools
import Metrowerks_Shell_Suite
import Required_Suite
 
_talker = None
 
class MWShell(aetools.TalkTo, 
 				Metrowerks_Shell_Suite.Metrowerks_Shell_Suite,
 				Required_Suite.Required_Suite):
	pass
 
def edit(file, line):
	global _talker
	if _talker == None:
		_talker = MWShell('CWIE', start=1)
	try:
		_talker.open(file)
		_talker.Goto_Line(line)
	except "(MacOS.Error, aetools.Error)":
		pass
 	
