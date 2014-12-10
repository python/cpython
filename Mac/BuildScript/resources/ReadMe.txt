This package will install Python $FULL_VERSION for Mac OS X $MACOSX_DEPLOYMENT_TARGET for the following architecture(s): $ARCHITECTURES.

=============================
Which installer variant should I use?
=============================

Python.org provides two installer variants for download: one that installs a 64-bit/32-bit Intel Python capable of running on Mac OS X 10.6 (Snow Leopard) or later; and one that installs a 32-bit-only (Intel and PPC) Python capable of running on Mac OS X 10.5 (Leopard) or later.  This ReadMe was installed with the $MACOSX_DEPLOYMENT_TARGET variant.  Unless you are installing to an 10.5 system or you need to build applications that can run on 10.5 systems, use the 10.6 variant if possible.  There are some additional operating system functions that are supported starting with 10.6 and you may see better performance using 64-bit mode.  By default, Python will automatically run in 64-bit mode if your system supports it.  Also see Certificate verification and OpenSSL below.

=============================
Update your version of Tcl/Tk to use IDLE or other Tk applications
=============================

To use IDLE or other programs that use the Tkinter graphical user interface toolkit, you need to install a newer third-party version of the Tcl/Tk frameworks.  Visit https://www.python.org/download/mac/tcltk/ for current information about supported and recommended versions of Tcl/Tk for this version of Python and of Mac OS X.

=============================
Installing on OS X 10.8 (Mountain Lion) or later systems
[CHANGED for Python 2.7.9]
=============================

As of Python 2.7.9, installer packages from python.org are now compatible with the Gatekeeper security feature introduced in OS X 10.8.   Downloaded packages can now be directly installed by double-clicking with the default system security settings.  Python.org installer packages for OS X are signed with the Developer ID of the builder, as identified on the download page for this release (https://www.python.org/downloads/).  To inspect the digital signature of the package, click on the lock icon in the upper right corner of the Install Python installer window.  Refer to Apple’s support pages for more information on Gatekeeper (http://support.apple.com/kb/ht5290).

=============================
Simplified web-based installs
[NEW for Python 2.7.9]
=============================

With the change to the newer flat format installer package, the download file now has a .pkg extension as it is no longer necessary to embed the installer within a disk image (.dmg) container.   If you download the Python installer through a web browser, the OS X installer application may open automatically to allow you to perform the install.  If your browser settings do not allow automatic open, double click on the downloaded installer file.

=============================
New Installation Options and Defaults
[NEW for Python 2.7.9]
=============================

The Python installer now includes an option to automatically install or upgrade pip, a tool for installing and managing Python packages.  This option is enabled by default and no Internet access is required.  If you do not want the installer to do this, select the Customize option at the Installation Type step and uncheck the Install or upgrade pip option.  For other changes in this release, see the Release Notes link for this release at https://www.python.org/downloads/.

=============================
Certificate verification and OpenSSL
[CHANGED for Python 2.7.9]
=============================

Python 2.7.9 includes a number of network security enhancements that have been approved for inclusion in Python 2.7 maintenance releases.  PEP 476 changes several standard library modules, like httplib, urllib2, and xmlrpclib, to by default verify certificates presented by servers over secure (TLS) connections.  The verification is performed by the OpenSSL libraries that Python is linked to.  Prior to 2.7.9, the python.org installers dynamically linked with Apple-supplied OpenSSL libraries shipped with OS X.  OS X provides a multiple level security framework that stores trust certificates in system and user keychains managed by the Keychain Access application and the security command line utility.

For OS X 10.5, Apple provides OpenSSL 0.9.7 libraries.  This version of Apple's OpenSSL does not use the certificates from the system security framework, even when used on newer versions of OS X.  Instead it consults a traditional OpenSSL concatenated certificate file (cafile) or certificate directory (capath), located in /System/Library/OpenSSL.  These directories are typically empty and not managed by OS X; you must manage them yourself or supply your own SSL contexts.  OpenSSL 0.9.7 is obsolete by current security standards, lacking a number of important features found in later versions.  Among the problems this causes is the inability to verify higher-security certificates now used by python.org services, including the Python Package Index, PyPI.  To solve this problem, as of 2.7.9 the 10.5+ 32-bit-only python.org variant is linked with a private copy of OpenSSL 1.0.1j; it consults the same default certificate directory, /System/Library/OpenSSL.   As before, it is still necessary to manage certificates yourself when you use this Python variant and, with certificate verification now enabled by default, you may now need to take additional steps to ensure your Python programs have access to CA certificates you trust.  If you use this Python variant to build standalone applications with third-party tools like py2app, you may now need to bundle CA certificates in them or otherwise supply non-default SSL contexts.

For OS X 10.6+, Apple also provides OpenSSL 0.9.8 libraries.  Apple's 0.9.8 version includes an important additional feature: if a certificate cannot be verified using the manually administered certificates in /System/Library/OpenSSL, the certificates managed by the system security framework In the user and system keychains are also consulted (using Apple private APIs).  For this reason, for 2.7.9 the 64-bit/32-bit 10.6+ python.org variant continues to be dynamically linked with Apple's OpenSSL 0.9.8 since it was felt that the loss of the system-provided certificates and management tools outweighs the additional security features provided by newer versions of OpenSSL.  This will likely change in future releases of the python.org installers as Apple has deprecated use of the system-supplied OpenSSL libraries.  If you do need features from newer versions of OpenSSL, there are third-party OpenSSL wrapper packages available through PyPI.

The bundled pip included with 2.7.9 has its own default certificate store for verifying download connections.

=============================
Binary installer support for OS X 10.4 and 10.3.9 discontinued
[CHANGED for Python 2.7.9]
=============================

As previously announced, binary installers for Python 2.7.9 from python.org no longer support Mac OS X 10.3.9 (Panther) and 10.4.x (Tiger) systems.  These systems were last updated by Apple in 2005 and 2007.  As of 2.7.9, the 32-bit-only installer supports PPC and Intel Macs running OS X 10.5 (Leopard). 10.5 was the last OS X release for PPC machines (G4 and G5).  The 64-/32-bit installer configuration remains unchanged and should normally be used on OS X 10.6 (Snow Leopard) and later systems.  This aligns Python 2.7.x installer configurations with those currently provided with Python 3.x.  If needed, it is still possible to build Python from source for 10.3.9 and 10.4.

=============================
Python 3 and Python 2 Co-existence
=============================

Python.org Python 2.7 and 3.x versions can both be installed on your system and will not conflict.  Python 2 command names contain a 2 or no digit: python2 (or python2.7 or python), idle2 (or idle2.7 or idle), pip2 (or pip2.7 or pip), etc.  Command names for Python 3 contain a 3 in them: python3, idle3, pip3, etc.
