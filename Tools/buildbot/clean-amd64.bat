@rem Formerly used by the buildbot "clean" step.
@echo This script is no longer used and may be removed in the future.
@echo To get the same effect as this script, use `clean.bat` from this
@echo directory and pass `-p x64` as two arguments.
call "%~dp0clean.bat" -p x64 %*
