IDLEfork README
===============

IDLEfork is an official experimental fork of Python's Integrated DeveLopment
Environment, IDLE.  The biggest change is to execute Python code in a separate
process, which is /restarted/ for each Run (F5) initiated from an editor
window.  This enhancement of IDLE has often been requested, and is now finally
available, complete with the IDLE debugger.  The magic "reload/import *"
incantations are no longer required when editing/testing a module two or three
steps down the import chain.

It is possible to interrupt tightly looping user code with a control-c, even on
Windows.

There is also a new GUI configuration manager which makes it easy to select
fonts, colors, keybindings, and startup options.  There is new feature where
the user can specify additional help sources, either locally or on the web.

IDLEfork will be merged back into the Python distribution in the near future
(probably 2.3), replacing the current version of IDLE.

For information on this release, refer to NEWS.txt

If you find bugs let us know about them by using the IDLEfork Bug Tracker.  See
the IDLEfork home page at

http://idlefork.sourceforge.net 

for details.  Patches are always appreciated at the IDLEfork Patch Tracker, and
Change Requests should be posted to the RFE Tracker at

https://sourceforge.net/tracker/?group_id=9579&atid=359579  

There is a mail list for IDLE: idle-dev@python.org.  You can join at

http://mail.python.org/mailman/listinfo/idle-dev

Thanks for trying IDLEfork.
