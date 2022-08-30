@echo off
rem A batch program to clean a particular configuration,
rem just for convenience.

set dir=%~dp0

call "%dir%build.bat" -t Clean %*

rem Clean previous instances of ouput files in `Python\deepfreeze`
rem and `Python\frozen_modules` directories to ensure successful building
rem on Windows.

del "%dir%\..\Python\deepfreeze\*.c"
del "%dir%\..\Python\frozen_modules\*.h"
