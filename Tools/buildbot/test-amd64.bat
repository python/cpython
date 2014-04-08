@rem Used by the buildbot "test" step.

rem The following line should be removed before #20035 is closed
set TCL_LIBRARY=%CD%\..\tcltk64\lib\tcl8.6
cd PCbuild
call rt.bat -d -q -x64 -uall -rwW -n --timeout=3600 %1 %2 %3 %4 %5 %6 %7 %8 %9
