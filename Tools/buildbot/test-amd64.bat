@rem Formerly used by the buildbot "test" step.
@echo This script is no longer used and may be removed in the future.
@echo To get the same effect as this script, use
@echo     PCbuild\rt.bat -q -d -x64 -uall -rwW
@echo or use `test.bat` in this directory and pass `-x64` as an argument.
call "%~dp0test.bat" -x64 %*
