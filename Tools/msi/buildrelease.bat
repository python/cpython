@setlocal
@echo off

rem This script is intended for building official releases of Python.
rem To use it to build alternative releases, you should clone this file
rem and modify the following three URIs.
rem
rem The first two will ensure that your release can be installed
rem alongside an official Python release, while the second specifies
rem the URL that will be used to download installation files. The
rem files available from this URL *will* conflict with your installer.
rem Trust me, you don't want them, even if it seems like a good idea.

set RELEASE_URI_X86=http://www.python.org/win32
set RELEASE_URI_X64=http://www.python.org/amd64
set DOWNLOAD_URL_BASE=https://www.python.org/ftp/python
set DOWNLOAD_URL=

set D=%~dp0
set PCBUILD=%D%..\..\PCBuild\

set BUILDX86=
set BUILDX64=
set TARGET=Rebuild


:CheckOpts
if "%1" EQU "-c" (set CERTNAME=%~2) && shift && shift && goto CheckOpts
if "%1" EQU "-o" (set OUTDIR=%~2) && shift && shift && goto CheckOpts
if "%1" EQU "-D" (set SKIPDOC=1) && shift && goto CheckOpts
if "%1" EQU "-B" (set SKIPBUILD=1) && shift && goto CheckOpts
if "%1" EQU "--download" (set DOWNLOAD_URL=%~2) && shift && shift && goto CheckOpts
if "%1" EQU "-b" (set TARGET=Build) && shift && goto CheckOpts
if '%1' EQU '-x86' (set BUILDX86=1) && shift && goto CheckOpts
if '%1' EQU '-x64' (set BUILDX64=1) && shift && goto CheckOpts

if not defined BUILDX86 if not defined BUILDX64 (set BUILDX86=1) && (set BUILDX64=1)

:builddoc
if "%SKIPBUILD%" EQU "1" goto skipdoc
if "%SKIPDOC%" EQU "1" goto skipdoc

call "%D%..\..\doc\make.bat" htmlhelp
if errorlevel 1 goto :eof
:skipdoc

where dlltool 2>nul >"%TEMP%\dlltool.loc"
if errorlevel 1 dir "%D%..\..\externals\dlltool.exe" /s/b > "%TEMP%\dlltool.loc"
if errorlevel 1 echo Cannot find binutils on PATH or in externals & exit /B 1
set /P DLLTOOL= < "%TEMP%\dlltool.loc"
set PATH=%PATH%;%DLLTOOL:~,-12%
set DLLTOOL=
del "%TEMP%\dlltool.loc"


if defined BUILDX86 (
    call :build x86
    if errorlevel 1 exit /B
)

if defined BUILDX64 (
    call :build x64
    if errorlevel 1 exit /B
)

exit /B 0

:build
@setlocal
@echo off

if "%1" EQU "x86" (
    call "%PCBUILD%env.bat" x86
    set BUILD=%PCBUILD%win32\
    set BUILD_PLAT=Win32
    set OUTDIR_PLAT=win32
    set OBJDIR_PLAT=x86
    set RELEASE_URI=%RELEASE_URI_X86%
) ELSE (
    call "%PCBUILD%env.bat" x86_amd64
    set BUILD=%PCBUILD%amd64\
    set BUILD_PLAT=x64
    set OUTDIR_PLAT=amd64
    set OBJDIR_PLAT=x64
    set RELEASE_URI=%RELEASE_URI_X64%
)

echo on
if exist "%BUILD%en-us" (
    echo Deleting %BUILD%en-us
    rmdir /q/s "%BUILD%en-us"
    if errorlevel 1 exit /B
)

echo on
if exist "%D%obj\Release_%OBJDIR_PLAT%" (
    echo Deleting "%D%obj\Release_%OBJDIR_PLAT%"
    rmdir /q/s "%D%obj\Release_%OBJDIR_PLAT%"
    if errorlevel 1 exit /B
)

if not "%CERTNAME%" EQU "" (
    set CERTOPTS="/p:SigningCertificate=%CERTNAME%"
) else (
    set CERTOPTS=
)

if not "%SKIPBUILD%" EQU "1" (
    call "%PCBUILD%build.bat" -e -p %BUILD_PLAT% -d -t %TARGET% %CERTOPTS%
    if errorlevel 1 exit /B
    call "%PCBUILD%build.bat" -p %BUILD_PLAT% -t %TARGET% %CERTOPTS%
    if errorlevel 1 exit /B
    @rem build.bat turns echo back on, so we disable it again
    @echo off
)

"%BUILD%python.exe" "%D%get_wix.py"

set BUILDOPTS=/p:Platform=%1 /p:BuildForRelease=true /p:DownloadUrl=%DOWNLOAD_URL% /p:DownloadUrlBase=%DOWNLOAD_URL_BASE% /p:ReleaseUri=%RELEASE_URI%
msbuild "%D%bundle\releaselocal.wixproj" /t:Rebuild %BUILDOPTS% %CERTOPTS% /p:RebuildAll=true
if errorlevel 1 exit /B
msbuild "%D%bundle\releaseweb.wixproj" /t:Rebuild %BUILDOPTS% %CERTOPTS% /p:RebuildAll=false
if errorlevel 1 exit /B

if not "%OUTDIR%" EQU "" (
    mkdir "%OUTDIR%\%OUTDIR_PLAT%"
    copy /Y "%BUILD%en-us\*.cab" "%OUTDIR%\%OUTDIR_PLAT%"
    copy /Y "%BUILD%en-us\*.exe" "%OUTDIR%\%OUTDIR_PLAT%"
    copy /Y "%BUILD%en-us\*.msi" "%OUTDIR%\%OUTDIR_PLAT%"
)

