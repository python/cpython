Python 1.4beta3 for Windows NT 3.5 and Windows 95
=================================================

The zip file pyth14b3.zip contains a preliminary binary release of
Python 1.4beta3 for Windows NT 3.5 and Windows '95, with Tcl/Tk
support.  The installation has not been tested on Windows 3.1 with
Win32s.  For general information on Python, see
http://www.python.org/.


To install
----------

Unzip the archive in the root of a file system with enough space.  It
will create a directory \Python1.4b3 containing subdirectories Bin and
Lib and some files, including setup.bat, setup.py, uninstall.bat, and
uninstall.py.  (If you don't have a zip program that supports long
filenames, get the file winzip95.exe and install it -- this is WinZip
6.1 for 32-bit Windows systems.)

Run the SETUP.BAT file found in directory just created.  When it is
done, press Enter.

Tcl/Tk support requires additional installation steps, see below.


To use
------

Python runs in a console (DOS) window.  From the File Manager, run the
file python.exe found in the Bin subdirectory.  You can also drag it
to a program group of your choice for easier access.  Opening any file
ending in .py from the file manager should run it.


To use with Tkinter
-------------------

Get the file win41p1.exe from /pub/python/nt/ on ftp.python.org or
from ftp site ftp.sunlabs.com, directory /pub/tcl/.  This is a
self-extracting archive containing the Tcl/Tk distribution for Windows
NT.  Don't use an older version.

Using the control panel, set the TCL_LIBRARY and TK_LIBRARY
environment variables.  E.g. if you installed Tcl/Tk in C:\TCL (the
default suggested by the installer), set TCL_LIBRARY to
"C:\TCL\lib\tcl7.5" and set TK_LIBRARY to "C:\TCL\lib\tk4.1".

On Windows '95, you need to edit AUTOEXEC.BAT for this, e.g. by adding
the lines

	SET TCL_LIBRARY=C:\Program Files\TCL\lib\tcl7.5
	SET TK_LIBRARY=C:\Program Files\TCL\lib\tk4.1

(substituting the actual location of the TCL installation directory).

On Windows '95, you also need to add the directory "C:\TCL\bin" (or
whereever the Tcl bin directory ended up) to the PATH environment
variable in the AUTOEXEC.BAT file.  Do this by editing the
AUTOEXEC.BAT file, e.g. by adding this line to the end:

	SET PATH="%PATH%";"C:\Program Files\TCL\bin"

(substituting the actual location of the TCL installation directory).

Once Tcl/Tk is installed, you should be able to type the following
commands in Python:

	>>> import Tkinter
	>>> Tkinter._test()

This creates a simple test dialog box (you may have to move the Python
window a bit to see it).  Click on OK to get the Python prompt back.


Troubleshooting
---------------

The following procedure will test successive components required for
successful use of Python and Tkinter.  The steps before "import
_tkinter" can be used to verify the proper installation of the Python
core.

- First, run the Python interpreter (python.exe).  This should give
you a ">>>" prompt in a "MS-DOS console" window.  This may fail with a
complaint about being unable to find the file MSVC40RT.DLL.  This file
(along with several other files) is included in the MSOFTDLL.EXE
self-extracting archive available in the /pub/python/wpy directory on
ftp.python.org.  After extraction, move MSVCRT40.NT to
\Windows\System\MSVCRT40.DLL (note the change of extension).

- If you can't get a ">>>" prompt, your core Python installation may
be botched.  Reinstall from the ZIP file (see above) and run
SETUP.BAT.

- At the ">>>" prompt, type a few commands.  Each command should
succeed without complaints.  Remember that Python is a case sensitive
language, so type the commands exactly as shown ("tkinter" and
"Tkinter" are two very different things).

>>> import sys

	If this fails, you can't type :-) (Explanation: this is a
	built-in module that is pre-initialized before the first ">>>"
	prompt is printed.  There is no way that this import can fail
	except by a typo.)

>>> import string

	If this fails, the Python library cannot be found.  Reinstall
	Python.  (Explanation: the registry entry for PythonPath is
	botched.  Inspect sys.path to see what it is.  If it is
	something like ['.', '.\\lib', '.\\lib\\win'], the setup.py
	script has not run successfully and you may get away with
	rerunning the SETUP.BAT file.)

>>> import _tkinter

	This can fail in a number of ways:

	ImportError: No module named _tkinter
		The Python module file _tkinter.dll can't be found.
		Since it is installed by default, the installation is
		probably botched.  Reinstall Python.

	ImportError: DLL load failed: The specified module could not
	be found.  (Possibly with a dialog box explaining that
	TCL75.DLL or TK41.DLL could not be found.)
		Probably a Tcl/Tk installation error.  Reinstall Tcl/Tk.
		Note that on Windows '95, you may need to add the Tcl
		bin directory to the PATH environment variable.

	Other failures:
		It may be possible that you have an early prerelease
		TCL75.DLL or TK41.DLL, which is incompatible with the
		_tkinter module in the Python distribution.  This will
		most likely result in error messages that don't make a
		lot of sense.  Try installing Tcl/Tk from the
		win41p1.exe self-extracting archive found in
		/pub/python/nt on ftp.python.org.

>>> import Tkinter

	If this fails, your Python library or sys.path is botched.
	Your best bet, again, is to reinstall Python.

>>> Tkinter._test()

	This should pop up a window with a label ("Proof-of-existence
	test for TK") and two buttons ("Click me!" and "QUIT").
	If you get nothing at all (not even a ">>>" prompt), the
	window is probably hiding behind the Python console window.
	Move the console window around to reveal the test window.

	If you get an exception instead, it is most likely a verbose
	complaint from Tcl/Tk about improper installation.  This is
	usually caused by bad or missing values for the environment
	variables TK_LIBRARY or TCL_LIBRARY.  See the installation
	instructions above.


To uninstall
------------

Run the batch file UNINSTALL.BAT.  This will run the Python script
uninstall.py, which undoes the registry additions and removes most
installed files.  The batch file then proceed to remove some files
that the Python script can't remove (because it's using them).  The
batch file ends with an error because it deletes itself.  Hints on how
to avoid this (and also on how to remove the installation directory
itself) are gracefully accepted.


September 3, 1996

--Guido van Rossum (home page: http://www.python.org/~guido/)
