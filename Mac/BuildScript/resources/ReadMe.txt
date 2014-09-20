This package will install Python $FULL_VERSION for Mac OS X $MACOSX_DEPLOYMENT_TARGET for the following architecture(s): $ARCHITECTURES.

=============================
Update your version of Tcl/Tk to use IDLE or other Tk applications
=============================

To use IDLE or other programs that use the Tkinter graphical user interface toolkit, you need to install a newer third-party version of the Tcl/Tk frameworks.  Visit https://www.python.org/download/mac/tcltk/ for current information about supported and recommended versions of Tcl/Tk for this version of Python and of Mac OS X.

=============================
Installing on OS X 10.8 (Mountain Lion) or later systems
[CHANGED for Python 3.4.2]
=============================

As of Python 3.4.2, installer packages from python.org are now compatible with the Gatekeeper security feature introduced in OS X 10.8.   Downloaded packages can now be directly installed by double-clicking with the default system security settings.  Python.org installer packages for OS X are signed with the Developer ID of the builder, as identified on the download page for this release (https://www.python.org/downloads/).  To inspect the digital signature of the package, click on the lock icon in the upper right corner of the Install Python installer window.  Refer to Apple’s support pages for more information on Gatekeeper (http://support.apple.com/kb/ht5290).

=============================
Simplified web-based installs
[NEW for Python 3.4.2]
=============================

With the change to the newer flat format installer package, the download file now has a .pkg extension as it is no longer necessary to embed the installer within a disk image (.dmg) container.   If you download the Python installer through a web browser, the OS X installer application may open automatically to allow you to perform the install.  If your browser settings do not allow automatic open, double click on the downloaded installer file.

=============================
New Installation Options and Defaults
[NEW for Python 3.4.0]
=============================

The Python installer now includes an option to automatically install or upgrade pip, a tool for installing and managing Python packages.  This option is enabled by default and no Internet access is required.  If you do not want the installer to do this, select the Customize option at the Installation Type step and uncheck the Install or ugprade pip option.

To make it easier to use scripts installed by third-party Python packages, with pip or by other means, the Shell profile updater option is now enabled by default, as has been the case with Python 2.7.x installers. You can also turn this option off by selecting Customize and unchecking the Shell profile updater option. You can also update your shell profile later by launching the Update Shell Profile command found in the /Applications/Python $VERSION folder.  You may need to start a new terminal window for the changes to take effect.

=============================
Python 3 and Python 2 Co-existence
=============================

Python.org Python $VERSION and 2.7.x versions can both be installed on your system and will not conflict. Command names for Python 3 contain a 3 in them, python3 (or python$VERSION), idle3 (or idle$VERSION), pip3 (or pip$VERSION), etc.  Python 2.7 command names contain a 2 or no digit: python2 (or python2.7 or python), idle2 (or idle2.7 or idle), etc.  If you want to use pip with Python 2.7.x, download and install a separate copy of it from the Python Package Index (https://pypi.python.org/pypi/pip/).
