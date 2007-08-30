@rem Used by the buildbot "test" step.
cd PCbuild
set PYTHONNOERRORWINDOW=1
call rt.bat -d -q -uall -rw
