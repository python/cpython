IBM VisualAge C/C++ for OS/2
============================

To build Python for OS/2, change into ./os2vacpp and issue an 'NMAKE'
command.  This will build a PYTHON15.DLL containing the set of Python
modules listed in config.c and a small PYTHON.EXE to start the
interpreter.

By changing the C compiler flag /Gd- in the makefile to /Gd+, you can
reduce the size of these by causing Python to dynamically link to the
C runtime DLLs instead of including their bulk in your binaries. 
However, this means that any system on which you run Python must have
the VAC++ compiler installed in order to have those DLLs available.

During the build process you may see a couple of harmless warnings:

  From the C Compiler, "No function prototype given for XXX", which
  comes from the use of K&R parameters within Python for portability.

  From the ILIB librarian, "Module Not Found (XXX)", which comes
  from its attempt to perform the (-+) operation, which removes and
  then adds a .OBJ to the library.  The first time a build is done,
  it obviously cannot remove what is not yet built.

This build includes support for most Python functionality as well as
TCP/IP sockets.  It omits the Posix ability to 'fork' a process but
supports threads using OS/2 native capabilities.  I have tried to
support everything possible but here are a few usage notes.


-- os.popen() Usage Warnings

With respect to my implementation of popen() under OS/2:

    import os

    fd = os.popen("pkzip.exe -@ junk.zip", 'wb')
    fd.write("file1.txt\n")
    fd.write("file2.txt\n")
    fd.write("file3.txt\n")
    fd.write("\x1a")  # Should Not Be Necessary But Is
    fd.close()

There is a bug, either in the VAC++ compiler or OS/2 itself, where the
simple closure of the write-side of a pipe -to- a process does not
send an EOF to that process.  I find I must explicitly write a
control-Z (EOF) before closing the pipe.  This is not a problem when
using popen() in read mode.

One other slight difference with my popen() is that I return None
from the close(), instead of the Unix convention of the return code
of the spawned program.  I could find no easy way to do this under
OS/2.


-- BEGINLIBPATH/ENDLIBPATH

With respect to environment variables, this OS/2 port supports the
special-to-OS/2 magic names of 'BEGINLIBPATH' and 'ENDLIBPATH' to
control where to load conventional DLLs from.  Those names are
intercepted and converted to calls on the OS/2 kernel APIs and
are inherited by child processes, whether Python-based or not.

A few new attributes have been added to the os module:

    os.meminstalled  # Count of Bytes of RAM Installed on Machine
    os.memkernel     # Count of Bytes of RAM Reserved (Non-Swappable)
    os.memvirtual    # Count of Bytes of Virtual RAM Possible
    os.timeslice     # Duration of Scheduler Timeslice, in Milliseconds
    os.maxpathlen    # Maximum Length of a Path Specification, in chars
    os.maxnamelen    # Maximum Length of a Single Dir/File Name, in chars
    os.version       # Version of OS/2 Being Run e.g. "4.00"
    os.revision      # Revision of OS/2 Being Run (usually zero)
    os.bootdrive     # Drive that System Booted From e.g. "C:"
                     # (useful to find the CONFIG.SYS used to boot with)


-- Using Python as the Default OS/2 Batch Language

Note that OS/2 supports the Unix technique of putting the special
comment line at the time of scripts e.g. "#!/usr/bin/python" in
a different syntactic form.  To do this, put your script into a file
with a .CMD extension and added 'extproc' to the top as follows:

    extproc C:\Python\Python.exe -x
    import os
    print "Hello from Python"

The '-x' option tells Python to skip the first line of the file
while processing the rest as normal Python source.


-- Suggested Environment Variable Setup

With respect to the environment variables for Python, I use the
following setup:

    Set PYTHONHOME=E:\Tau\Projects\Python;D:\DLLs
    Set PYTHONPATH=.;E:\Tau\Projects\Python\Lib; \
                     E:\Tau\Projects\Python\Lib\plat-win

The EXEC_PREFIX (optional second pathspec on PYTHONHOME) is where
you put any Python extension DLLs you may create/obtain.  There
are none provided with this release.


-- Contact Info

Jeff Rush is no longer supporting the VACPP port :-(

I don't have the VACPP compiler, so can't reliably maintain this port. 

Anyone with VACPP who can contribute patches to keep this port buildable
should upload them to the Python Patch Manager at Sourceforge and 
assign them to me for review/checkin.

Andrew MacIntyre
aimacintyre at users.sourceforge.net
August 18, 2002.
