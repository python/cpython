This package will install beta 1 of the third build of
the MacPython 2.3 additions for Mac OS X 10.3. 

Installation requires approximately 3.3 MB of disk
space, ignore the message that it will take zero bytes.

You must install onto your current boot disk, even
though the installer does not enforce this, otherwise
things will not work.

This installer does not contain a Python engine, as
Apple already includes that in 10.3. It does contain
a set of programs to allow easy access to it for Mac 
users (an integrated development environment, a Python 
extension package manager) and the waste module. 

The installer puts the applications in MacPython-2.3 in
your Applications folder.

The PythonIDE application has a Help command that gets
you started quickly with MacPython and contains
references to other documentation.

Changes since the second build:
- Package Manager has been updated to version 0.5:
  - added commands to open database description page
    and standard experimental database.
  - the scrollbar had a mind of its own. Fixed.
  - show all packages in case of an error message.
  - easier maintainance (mainly important for me:-)
    in the light of micro-releases of OSX.
  - some integration with external installers
- IDE fixes:
  - better handling of various end-of-line schemes.
  - fixed "run with commandline python" to use pythonw.
  - fixed a crash with very big scripts folders.
  - fixed the double-scroll problem when you single-clicked.
- Python fixes:
  - One fix is made to the Apple-installed Python itself.
    As distributed the installation of a newer Python would
    cause Apple python to have problems building extensions,
    this is fixed.
  
Changes since the first build:
- The startup crash of the IDE some people experienced
  has been fixed. The IDE Scripts folder is now in
  ~/Library/Python/IDE-Scripts.
- The full Python documentation can now be installed
  through the Package Manager.

More information on MacPython can be found at
http://www.cwi.nl/~jack/macpython, more
information on Python in general at
http://www.python.org.
