'''
Minimal test module
'''#

import sys
import PythonScript

SIGNATURE = 'MACS'
TIMEOUT = 10*60*60

PythonScript.PsScript(SIGNATURE, TIMEOUT)
p = PythonScript.PyScript
ev = PythonScript.PsEvents
pc = PythonScript.PsClass
pp = PythonScript.PsProperties

startup = str(p(ev.Get, pc.Desktopobject(1).Startup_disk().Name()))
print 'startup',startup, type(startup)
print p(ev.Get, pc.Disk(startup).Folder(7).File(1).Name())
print p(ev.Get, pc.Disk(1).Name())
print p(ev.Get, pc.Disk('every').Name())
print p(ev.Make, None, New='Alias_file', At=pp.Desktop(''), To=pp.System_folder(1))

sys.exit(1)
	
