
"""mpwsystem -
A simple example of how to use Apple Events to implement a "system()"
call that invokes ToolServer on the command.

Contributed by Daniel Brotsky <dev@brotsky.com>.

(renamed from aesystem to mpwsystem by Jack)

system(cmd, infile = None, outfile = None, errfile = None)

1. Every call to system sets "lastStatus" and "lastOutput" to the 
status and output
produced by the command when executed in ToolServer.  (lastParameters 
and lastAttributes
are set to the values of the AppleEvent result.)

2. system returns lastStatus unless the command result indicates a MacOS error,
in which case os.Error is raised with the errnum as associated value.

3. You can specify ToolServer-understandable pathnames for 
redirection of input,
output, and error streams.  By default, input is Dev:Null, output is captured
and returned to the caller, diagnostics are captured and returned to 
the caller.
(There's a 64K limit to how much can be captured and returned this way.)"""

import os
import aetools

try: server
except NameError: server = aetools.TalkTo("MPSX", 1)

lastStatus = None
lastOutput = None
lastErrorOutput = None
lastScript = None
lastEvent = None
lastReply = None
lastParameters = None
lastAttributes = None

def system(cmd, infile = None, outfile = None, errfile = None):
	global lastStatus, lastOutput, lastErrorOutput
	global lastScript, lastEvent, lastReply, lastParameters, lastAttributes
	cmdline = cmd
	if infile: cmdline += " <" + infile
	if outfile: cmdline += " >" + outfile
	if errfile: cmdline += " " + str(chr(179)) + errfile
	lastScript = "set Exit 0\r" + cmdline + "\rexit {Status}"
	lastEvent = server.newevent("misc", "dosc", {"----" : lastScript})
	(lastReply, lastParameters, lastAttributes) = server.sendevent(lastEvent)
	if lastParameters.has_key('stat'): lastStatus = lastParameters['stat']
	else: lastStatus = None
	if lastParameters.has_key('----'): lastOutput = lastParameters['----']
	else: lastOutput = None
	if lastParameters.has_key('diag'): lastErrorOutput = lastParameters['diag']
	else: lastErrorOutput = None
	if lastParameters['errn'] != 0:
		raise os.Error, lastParameters['errn']
	return lastStatus
	
if __name__ == '__main__':
	sts = system('alert "Hello World"')
	print 'system returned', sts
	
	
