Python 1.4beta3 for Windows NT 3.5
==================================

The zip file pyth14b3.zip contains a preliminary binary release of
Python 1.4beta3 for Windows NT 3.5, with Tcl/Tk support.  The
installation has not been tested on Windows '95 or Windows 3.1 with
Win32s.  For general information on Python, see http://www.python.org/.

To install:

Unzip the archive in the root of a file system with enough space.  It
will create a directory \Python1.4b3 containing subdirectories Bin and
Lib and files setup.bat and setup.py.  (If you don't have a zip
program that supports long filenames, get the file winzip95.exe and
install it -- this is WinZip 6.1 for 32-bit Windows systems.)

Run the SETUP.BAT file found in directory just created.  When it is
done, press Enter.

To use:

Python runs in a console (DOS) window.  From the File Manager, run the
file python.exe found in the Bin subdirectory.  You can also drag it
to a program group of your choice for easier access.  Opening any file
ending in .py from the file manager should run it.

To use with Tkinter:

Get the file win41p1.exe from here or from ftp site ftp.sunlabs.com,
directory /pub/tcl/.  This is a self-extracting archive containing the
Tcl/Tk distribution for Windows NT.  Don't use an older version.

Using the control panel, set the TCL_LIBRARY and TK_LIBRARY
environment variables.  E.g. if you installed Tcl/Tk in C:\TCL (the
default suggested by the installer), set TCL_LIBRARY to
"C:\TCL\lib\tcl7.5" and set TK_LIBRARY to "C:\TCL\lib\tk4.1".

Once Tcl/Tk is installed, you should be able to type the following
commands in Python:

	>>> import Tkinter
	>>> Tkinter._test()

This creates a simple test dialog box (you may have to move the Python
window a bit to see it).  Click on OK to get the Python prompt back.

August 30, 1996

--Guido van Rossum (home page: http://www.python.org/~guido/)
