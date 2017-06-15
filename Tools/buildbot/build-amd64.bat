@rem Formerly used by the buildbot "compile" step.
@echo This script is no longer used and may be removed in the future.
@echo To get the same effect as this script, use
@echo     PCbuild\build.bat -d -e -k -p x64
call "%~dp0build.bat" -p x64 %*
