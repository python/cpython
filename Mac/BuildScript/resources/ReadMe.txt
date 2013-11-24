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

              **NEW* As of Python 3.4.0b1:

New Installation Options and Defaults
=====================================

The Python installer now includes an option to automatically install
or upgrade pip, a tool for installing and managing Python packages.
This option is enabled by default and no Internet access is required.
If you do not want the installer to do this, select the "Customize"
option at the "Installation Type" step and uncheck the "Install or
ugprade pip" option.

To make it easier to use scripts installed by third-party Python
packages, with pip or by other means, the "Shell profile updater"
option is now enabled by default, as has been the case with Python
2.7.x installers. You can also turn this option off by selecting
"Customize" and unchecking the "Shell profile updater" option. You
can also update your shell profile later by launching the "Update
Shell Profile" command found in the /Applications/Python $VERSION
folder.  You may need to start a new terminal window for the
changes to take effect.

Python.org Python $VERSION and 2.7.x versions can both be installed and
will not conflict. Command names for Python 3 contain a 3 in them,
python3 (or python$VERSION), idle3 (or idle$VERSION), pip3 (or pip$VERSION), etc.
Python 2.7 command names contain a 2 or no digit: python2 (or
python2.7 or python), idle2 (or idle2.7 or idle), etc. If you want to
use pip with Python 2.7.x, you will need to download and install a
separate copy of it from the Python Package Index
(https://pypi.python.org/pypi).

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
