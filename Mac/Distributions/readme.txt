How to make a Python-distribution.
----------------------------------

These notes are mainly for myself.

- Increase fragment version number in PythonCorePPC and PythonCoreCFM68K
- Increase version number in STR resource (preffilename) and VERS resource
- Update about box
- Increase version number in _versioncheck.py
- Build PythonStandSmall, run once in root folder
- Update Relnotes, readme's, Demo:build.html
- Make sure tkresources.rsrc is up-to-date
- fullbuild everything with increase-buildno
- Run configurepython
- set "no console" on all applets
- remove alis resource from all applets
- mkdistr
- remove stdwin, cwilib demos
- test on virgin system. Make sure to test tkinter too.
- make distribution archive
- binhextree, synctree
- update version number on the net
