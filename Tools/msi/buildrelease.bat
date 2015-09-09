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
set TESTTARGETDIR=


:CheckOpts
if "%1" EQU "-h" goto Help
if "%1" EQU "-c" (set CERTNAME=%~2) && shift && shift && goto CheckOpts
if "%1" EQU "--certificate" (set CERTNAME=%~2) && shift && shift && goto CheckOpts
if "%1" EQU "-o" (set OUTDIR=%~2) && shift && shift && goto CheckOpts
if "%1" EQU "--out" (set OUTDIR=%~2) && shift && shift && goto CheckOpts
if "%1" EQU "-D" (set SKIPDOC=1) && shift && goto CheckOpts
if "%1" EQU "--skip-doc" (set SKIPDOC=1) && shift && goto CheckOpts
if "%1" EQU "-B" (set SKIPBUILD=1) && shift && goto CheckOpts
if "%1" EQU "--skip-build" (set SKIPBUILD=1) && shift && goto CheckOpts
if "%1" EQU "--download" (set DOWNLOAD_URL=%~2) && shift && shift && goto CheckOpts
if "%1" EQU "--test" (set TESTTARGETDIR=%~2) && shift && shift && goto CheckOpts
if "%1" EQU "-b" (set TARGET=Build) && shift && goto CheckOpts
if "%1" EQU "--build" (set TARGET=Build) && shift && goto CheckOpts
if "%1" EQU "-x86" (set BUILDX86=1) && shift && goto CheckOpts
if "%1" EQU "-x64" (set BUILDX64=1) && shift && goto CheckOpts

if not defined BUILDX86 if not defined BUILDX64 (set BUILDX86=1) && (set BUILDX64=1)

:builddoc
if "%SKIPBUILD%" EQU "1" goto skipdoc
if "%SKIPDOC%" EQU "1" goto skipdoc

if not defined PYTHON where py -q || echo Cannot find py on path and PYTHON is not set. && exit /B 1
if not defined SPHINXBUILD where sphinx-build -q || echo Cannot find sphinx-build on path and SPHINXBUILD is not set. && exit /B 1
call "%D%..\..\doc\make.bat" htmlhelp
if errorlevel 1 goto :eof
:skipdoc

where hg /q || echo Cannot find Mercurial on PATH && exit /B 1

where dlltool /q && goto skipdlltoolsearch
set _DLLTOOL_PATH=
where /R "%D%..\..\externals" dlltool > "%TEMP%\dlltool.loc" 2> nul && set /P _DLLTOOL_PATH= < "%TEMP%\dlltool.loc" & del "%TEMP%\dlltool.loc" 
if not exist "%_DLLTOOL_PATH%" echo Cannot find binutils on PATH or in external && exit /B 1
for %%f in (%_DLLTOOL_PATH%) do set PATH=%PATH%;%%~dpf
set _DLLTOOL_PATH=
:skipdlltoolsearch

if defined BUILDX86 (
    call :build x86
    if errorlevel 1 exit /B
)

if defined BUILDX64 (
    call :build x64
    if errorlevel 1 exit /B
)

if defined TESTTARGETDIR (
    call "%D%testrelease.bat" -t "%TESTTARGETDIR%"
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

if exist "%BUILD%en-us" (
    echo Deleting %BUILD%en-us
    rmdir /q/s "%BUILD%en-us"
    if errorlevel 1 exit /B
)

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
    call "%PCBUILD%build.bat" -e -p %BUILD_PLAT% -t %TARGET% %CERTOPTS%
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

msbuild "%D%make_zip.proj" /t:Build %BUILDOPTS% %CERTOPTS%

if not "%OUTDIR%" EQU "" (
    mkdir "%OUTDIR%\%OUTDIR_PLAT%"
    copy /Y "%BUILD%en-us\*.cab" "%OUTDIR%\%OUTDIR_PLAT%"
    copy /Y "%BUILD%en-us\*.exe" "%OUTDIR%\%OUTDIR_PLAT%"
    copy /Y "%BUILD%en-us\*.msi" "%OUTDIR%\%OUTDIR_PLAT%"
    copy /Y "%BUILD%en-us\*.msu" "%OUTDIR%\%OUTDIR_PLAT%"
)

exit /B 0

:Help
echo buildrelease.bat [--out DIR] [-x86] [-x64] [--certificate CERTNAME] [--build] [--skip-build]
echo                  [--skip-doc] [--download DOWNLOAD URL] [--test TARGETDIR] [-h]
echo.
echo    --out (-o)          Specify an additional output directory for installers
echo    -x86                Build x86 installers
echo    -x64                Build x64 installers
echo    --build (-b)        Incrementally build Python rather than rebuilding
echo    --skip-build (-B)   Do not build Python (just do the installers)
echo    --skip-doc (-D)     Do not build documentation
echo    --download          Specify the full download URL for MSIs (should include {2})
echo    --test              Specify the test directory to run the installer tests
echo    -h                  Display this help information
echo.
echo If no architecture is specified, all architectures will be built.
echo If --test is not specified, the installer tests are not run.
echo.