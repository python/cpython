This package will install Python $FULL_VERSION for Mac OS X
$MACOSX_DEPLOYMENT_TARGET for the following architecture(s):
$ARCHITECTURES.

               **** IMPORTANT ****

Installing on OS X 10.8 (Mountain Lion) or later systems
========================================================

If you are attempting to install on an OS X 10.8+ system, you may
see a message that Python can't be installed because it is from an
unidentified developer.  This is because this Python installer
package is not yet compatible with the Gatekeeper security feature
introduced in OS X 10.8.  To allow Python to be installed, you
can override the Gatekeeper policy for this install.  In the Finder,
instead of double-clicking, control-click or right click the "Python"
installer package icon.  Then select "Open using ... Installer" from
the contextual menu that appears.

               **** IMPORTANT ****

Update your version of Tcl/Tk to use IDLE or other Tk applications
==================================================================

To use IDLE or other programs that use the Tkinter graphical user
interface toolkit, you may need to install a newer third-party version
of the Tcl/Tk frameworks.  Visit http://www.python.org/download/mac/tcltk/
for current information about supported and recommended versions of
Tcl/Tk for this version of Python and of Mac OS X.

               **** IMPORTANT ****

Binary installer support for 10.4 and 10.3.9 to be discontinued
===============================================================

Python 2.7.7 is the last release for which binary installers will be
released on python.org that support OS X 10.3.9 (Panther) and 10.4.x
(Tiger) systems.  These systems were last updated by Apple in 2005
and 2007.  As of 2.7.8, the 32-bit-only installer will support PPC
and Intel Macs running OS X 10.5 (Leopard) and later.  10.5 was the
last OS X release for PPC machines (G4 and G5).  (The 64-/32-bit
installer configuration will remain unchanged.)  This aligns Python
2.7.x installer configurations with those currently provided with
Python 3.x.  Some of the reasons for making this change are:
there were significant additions and compatibility improvements to
the OS X POSIX system APIs in OS X 10.5 that Python users can now
take advantage of; it is increasingly difficult to build and test
on obsolete 10.3 and 10.4 systems and with the 10.3 ABI; and it is
assumed that most remaining legacy PPC systems have upgraded to 10.5.
To ease the transition, for Python 2.7.7 only we are providing three
binary installers: (1) the legacy deprecated 32-bit-only 10.3+
PPC/Intel format, (2) the newer 32-bit-only 10.5+ PPC/Intel format,
and (3) the current 64-bit/32-bit 10.6+ Intel-only format.  While
future releases will not provide the deprecated installer, it will
still be possible to build Python from source on 10.3.9 and 10.4
systems if needed.

Using this version of Python on OS X
====================================

Python consists of the Python programming language interpreter, plus
a set of programs to allow easy access to it for Mac users including
an integrated development environment, IDLE, plus a set of pre-built
extension modules that open up specific Macintosh technologies to
Python programs.

The installer puts applications, an "Update Shell Profile" command,
and a link to the optionally installed Python Documentation into the
"Python $VERSION" subfolder of the system Applications folder,
and puts the underlying machinery into the folder
$PYTHONFRAMEWORKINSTALLDIR. It can
optionally place links to the command-line tools in /usr/local/bin as
well. Double-click on the "Update Shell Profile" command to add the
"bin" directory inside the framework to your shell's search path.

You must install onto your current boot disk, even though the
installer may not enforce this, otherwise things will not work.

You can verify the integrity of the disk image file containing the
installer package and this ReadMe file by comparing its md5 checksum
and size with the values published on the release page linked at
http://www.python.org/download/

Installation requires approximately $INSTALL_SIZE MB of disk space,
ignore the message that it will take zero bytes.

More information on Python in general can be found at
http://www.python.org.
