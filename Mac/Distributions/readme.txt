How to make a Python-distribution.
----------------------------------

These notes are mainly for myself, or for whoever tries to make a MacPython
distribution when I'm fed up with it. They were last updated for 2.1b2.

- Increase fragment version number in PythonCore and PythonCoreCarbon.
  the fragment number is Python's sys.hexversion, it should be set in the
  "PEF" preferences.
- Increase version number in _versioncheck.py
- Build PythonStandSmall, run once in root folder
- Update Relnotes, readme's, Demo:build.html
- Make sure tkresources.rsrc is up-to-date
- fullbuild everything with increase-buildno
- Update Numeric and build/install it both with Classic and with Carbon python
- Run configurepython
- Recompile OSAm and possibly other Contrib stuff
- mkdistr binary.include
- mkdistr dev.include
- make distribution archive with Installer Vise
  Things to make sure of:
  - Finder icon positions
  - Version numbers in "Packages..." window
  - Version number in "Installer Settings" -> "Easy Install Text"
  - Version number in "Project" -> Attributes
  - Version number in "Project" -> PostProcess
  - Version number in "Internet" -> "Download Sites"
  - Version number in "Internet" -> "File Groups".
- test on virgin systems (OSX, OS9, OS8 without Carbon). Make sure to test
  tkinter too.
- Upload
- Update README file in ftp directory
- Change version number in public_html/macpythonversion.txt .
- Update macpython.html
- Send an announcement to:
   pythonmac-sig@python.org
   python-dev@python.org
   python-announce@python.org
   archivist@info-mac.org
   adcnews@apple.com
   http://www.macupdate.com
   http://guide.apple.com/usindex.html
   http://www.versiontracker.com/ Jack.Jansen@oratrix.com