@setlocal
@echo off

rem This script is intended for building official releases of Python.
rem To use it to build alternative releases, you should clone this file
rem and modify the following three URIs.

rem These two will ensure that your release can be installed
rem alongside an official Python release, by modifying the GUIDs used
rem for all components.
rem
rem The following substitutions will be applied to the release URI:
rem     Variable        Description         Example
rem     {arch}          architecture        amd64, win32
set RELEASE_URI=http://www.python.org/{arch}

rem This is the URL that will be used to download installation files.
rem The files available from the default URL *will* conflict with your
rem installer. Trust me, you don't want them, even if it seems like a
rem good idea.
rem
rem The following substitutions will be applied to the download URL:
rem     Variable        Description         Example
rem     {version}       version number      3.5.0
rem     {arch}          architecture        amd64, win32
rem     {releasename}   release name        a1, b2, rc3 (or blank for final)
rem     {msi}           MSI filename        core.msi
set DOWNLOAD_URL=https://www.python.org/ftp/python/{version}/{arch}{releasename}/{msi}

set D=%~dp0
set PCBUILD=%D%..\..\PCBuild\
set EXTERNALS=%D%..\..\externals\windows-installer\

set BUILDX86=
set BUILDX64=
set TARGET=Rebuild
set TESTTARGETDIR=
set PGO=


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
if "%1" EQU "--pgo" (set PGO=%~2) && shift && shift && goto CheckOpts

if "%1" NEQ "" echo Invalid option: "%1" && exit /B 1

if not defined BUILDX86 if not defined BUILDX64 (set BUILDX86=1) && (set BUILDX64=1)

call "%D%get_externals.bat"

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
where /R "%EXTERNALS%\" dlltool > "%TEMP%\dlltool.loc" 2> nul && set /P _DLLTOOL_PATH= < "%TEMP%\dlltool.loc" & del "%TEMP%\dlltool.loc" 
if not exist "%_DLLTOOL_PATH%" echo Cannot find binutils on PATH or in external && exit /B 1
for %%f in (%_DLLTOOL_PATH%) do set PATH=%PATH%;%%~dpf
set _DLLTOOL_PATH=
:skipdlltoolsearch

if defined BUILDX86 (
    call :build x86
    if errorlevel 1 exit /B
)

if defined BUILDX64 (
    call :build x64 "%PGO%"
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
) else if "%~2" NEQ "" (
    call "%PCBUILD%env.bat" amd64
    set PGO=%~2
    set BUILD=%PCBUILD%amd64-pgo\
    set BUILD_PLAT=x64
    set OUTDIR_PLAT=amd64
    set OBJDIR_PLAT=x64
) else (
    call "%PCBUILD%env.bat" amd64
    set BUILD=%PCBUILD%amd64\
    set BUILD_PLAT=x64
    set OUTDIR_PLAT=amd64
    set OBJDIR_PLAT=x64
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
    @call "%PCBUILD%build.bat" -e -p %BUILD_PLAT% -d -t %TARGET% %CERTOPTS%
    @if errorlevel 1 exit /B
    @rem build.bat turns echo back on, so we disable it again
    @echo off
    
    if "%PGO%" EQU "" (
        @call "%PCBUILD%build.bat" -e -p %BUILD_PLAT% -t %TARGET% %CERTOPTS%
    ) else (
        @call "%PCBUILD%build.bat" -e -p %BUILD_PLAT% -c PGInstrument -t %TARGET% %CERTOPTS%
        @if errorlevel 1 exit /B
        
        @del "%BUILD%*.pgc"
        if "%PGO%" EQU "default" (
            "%BUILD%python.exe" -m test -q --pgo
        ) else if "%PGO%" EQU "default2" (
            "%BUILD%python.exe" -m test -r -q --pgo
            "%BUILD%python.exe" -m test -r -q --pgo
        ) else if "%PGO%" EQU "default10" (
            for /L %%i in (0, 1, 9) do "%BUILD%python.exe" -m test -q -r --pgo
        ) else if "%PGO%" EQU "pybench" (
            "%BUILD%python.exe" "%PCBUILD%..\Tools\pybench\pybench.py"
        ) else (
            "%BUILD%python.exe" %PGO%
        )
        
        @call "%PCBUILD%build.bat" -e -p %BUILD_PLAT% -c PGUpdate -t %TARGET% %CERTOPTS%
    )
    @if errorlevel 1 exit /B
    @echo off
)

set BUILDOPTS=/p:Platform=%1 /p:BuildForRelease=true /p:DownloadUrl=%DOWNLOAD_URL% /p:DownloadUrlBase=%DOWNLOAD_URL_BASE% /p:ReleaseUri=%RELEASE_URI%
if "%PGO%" NEQ "" set BUILDOPTS=%BUILDOPTS% /p:PGOBuildPath=%BUILD%
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
echo                  [--pgo COMMAND] [--skip-doc] [--download DOWNLOAD URL] [--test TARGETDIR]
echo                  [-h]
echo.
echo    --out (-o)          Specify an additional output directory for installers
echo    -x86                Build x86 installers
echo    -x64                Build x64 installers
echo    --build (-b)        Incrementally build Python rather than rebuilding
echo    --skip-build (-B)   Do not build Python (just do the installers)
echo    --skip-doc (-D)     Do not build documentation
echo    --pgo               Build x64 installers using PGO
echo    --download          Specify the full download URL for MSIs
echo    --test              Specify the test directory to run the installer tests
echo    -h                  Display this help information
echo.
echo If no architecture is specified, all architectures will be built.
echo If --test is not specified, the installer tests are not run.
echo.
echo For the --pgo option, any Python command line can be used as well as the
echo following shortcuts:
echo     Shortcut        Description
echo     default         Test suite with --pgo
echo     default2        2x test suite with --pgo and randomized test order
echo     default10       10x test suite with --pgo and randomized test order
echo     pybench         pybench script
echo.
echo The following substitutions will be applied to the download URL:
echo     Variable        Description         Example
echo     {version}       version number      3.5.0
echo     {arch}          architecture        amd64, win32
echo     {releasename}   release name        a1, b2, rc3 (or blank for final)
echo     {msi}           MSI filename        core.msi
