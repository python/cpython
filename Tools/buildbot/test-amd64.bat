@rem Used by the buildbot "test" step.

rem The following line should be removed before #20035 is closed
set TCL_LIBRARY=%~dp0..\..\..\tcltk64\lib\tcl8.6

"%~dp0..\..\PCbuild\amd64\python_d.exe" "%~dp0..\scripts\run_tests.py" -j 1 -u all -W --timeout=3600 %*
