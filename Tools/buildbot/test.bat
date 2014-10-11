@rem Used by the buildbot "test" step.

rem The following line should be removed before #20035 is closed
set TCL_LIBRARY=%~dp0..\..\..\tcltk\lib\tcl8.6

ver | findstr "Version 6." >nul
if %ERRORLEVEL% == 1 goto xp

"%~dp0..\..\PCbuild\python_d.exe" "%~dp0..\scripts\run_tests.py" -j 1 -u all -W --timeout=3600 %*
goto done

:xp
cd PCbuild
call rt.bat -d -q -uall -rwW -n --timeout=3600 %1 %2 %3 %4 %5 %6 %7 %8 %9

:done
