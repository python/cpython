#
# Test of generated AE modules.
#
import sys
import macfs

import aetools
from AppleScript_Suite import AppleScript_Suite
from Required_Suite import Required_Suite
from Standard_Suite import Standard_Suite

class ScriptableEditor(aetools.TalkTo, AppleScript_Suite, Required_Suite,
		Standard_Suite):
	
	def __init__(self):
		aetools.TalkTo.__init__(self, 'quil')
		self.activate()
		
app = ScriptableEditor()
rv = app.open(macfs.FSSpec(sys.argv[0]))
print 'Opened', sys.argv[0]
print 'Return value:', rv
rv = app.get(aetools.Word(10, aetools.Document(1)))
print 'Got word 10 doc 1:', rv
sys.exit(1)
