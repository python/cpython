@rem Used by the buildbot "test" step.

setlocal
rem The following line should be removed before #20035 is closed
set TCL_LIBRARY=%~dp0..\..\externals\tcltk\lib\tcl8.6

call "%~dp0..\..\PCbuild\rt.bat" -d -q -uall -rwW -n --timeout=3600 %*
