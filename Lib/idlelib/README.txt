IDLEfork README
===============

IDLEfork is an official experimental fork of Python's Integrated 
DeveLopment Environment IDLE.  The biggest change is to execute
Python code in a separate process, which is /restarted/ for each
Run (F5) initiated from an editor window.  This enhancement of
IDLE has often been requested, and is now finally available,
complete with debugger. 

There is also a new GUI configuration manager which makes it easy
to select fonts, colors, and startup options.

IDLEfork will be merged back into the Python distribution in the
near future (probably 2.3), replacing the current version of IDLE.

As David Scherer aptly put it in the original IDLEfork README,
"It is alpha software and might be unstable. If it breaks, you get to 
keep both pieces." 

If you find bugs let us know about them by using the IDLEfork Bug
Tracker.  See the IDLEfork home page at

http://idlefork.sourceforge.net 

for details.  Patches are always appreciated at the IDLEfork Patch
Tracker.

Please see the files NEWS.txt and ChangeLog for more up to date
information on changes in this release of IDLEfork.

Thanks for trying IDLEfork.


IDLEfork 0.9 Alpha 0
--------------------------------

Introduced the new RPC implementation, which includes a debugger.  The
output of user code is to the shell, and the shell may be used to
inspect the environment after the run has finished.  (In version 0.8.1
the shell environment was separate from the environment of the user
code.)

Introduced the configuration GUI and a new About dialog.

Adapted to the Mac platform.

Multiple bug fixes and usability enhancements.

Known issues:

- Can't kill a tight loop in the Windows version: Use the Task Manager!
- Printing under Linux may be problematic.
- The debugger is pretty slow.
- RPC stack levels are not being pruned from debugger tracebacks.
- Changelog and NEWS.txt are incomplete.
