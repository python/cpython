@rem Used by the buildbot "test" step.

setlocal

call "%~dp0..\..\PCbuild\rt.bat" -d -q -uall -rwW -n --timeout=3600 %*
