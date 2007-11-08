@echo off
setlocal

set target=end

if "%1"=="makeinfo" goto makeinfo
if "%1"=="clean" goto clean
if "%1"=="build" goto build
if "%1"=="setargv" goto setargv
if "%1"=="" goto build

echo Usage: build.bat build
echo        build.bat clean
echo        build.bat setargv
goto end

:clean
del *.pyd *.exe *.dll *.exp *.lib *.pdb *.o
rmdir /S /Q temp
rmdir /S /Q x86-temp-release 
goto %target%

:setargv
cl /c /I"%SDK%\src\crt" /MD /D_CRTBLD "%SDK%\src\crt\setargv.c"
if not exist setargv.obj echo An error occured & goto end
echo copy setargv.obj "%SDK%\Lib"
copy setargv.obj "%SDK%\Lib"
del setargv.obj
goto %target%

:makeinfo
nant -buildfile:python.build all
lib /def: x86-temp-release\make_buildinfo\make_buildinfo.obj
lib /def: x86-temp-release\make_versioninfo\make_versioninfo.obj
goto %target%

:build
if not exist make_buildinfo.lib set target=realbuild & goto makeinfo
if not exist make_versioninfo.lib set target=realbuild & goto makeinfo
if exist "%SDK%\Lib\setargv.obj" goto realbuild
echo !!!!!!!!
echo setargv.obj is missing. Please call build setargv
echo !!!!!!!!

:realbuild
if not exist sqlite3.dll copy ..\..\sqlite-source-3.3.4\sqlite3.dll .
nant -buildfile:python.build all
goto end

:end