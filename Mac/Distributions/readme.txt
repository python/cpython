How to make a Python-distribution.
----------------------------------

These notes are mainly for myself, or for whoever tries to make a MacPython
distribution when I'm fed up with it. They were last updated for 2.1a2.

- Increase fragment version number in PythonCore and PythonCoreCarbon.
  the fragment number is Python's sys.hexversion, it should be set in the
  "PEF" preferences.
- Increase version number in _versioncheck.py
- Build PythonStandSmall, run once in root folder
- Update Relnotes, readme's, Demo:build.html
- Make sure tkresources.rsrc is up-to-date
- fullbuild everything with increase-buildno
- Run configurepython
- set "no console" on all applets
- remove alis resource from all applets
- mkdistr binary.include
- mkdistr dev.include
- make distribution archive with Installer Vise
- test on virgin system. Make sure to test tkinter too.
