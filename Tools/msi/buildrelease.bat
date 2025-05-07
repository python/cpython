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
rem Do not change the scheme to https. Otherwise, releases built with this
rem script will not be upgradable to/from official releases of Python.
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
set PCBUILD=%D%..\..\PCbuild\
if NOT DEFINED Py_OutDir set Py_OutDir=%PCBUILD%
set EXTERNALS=%D%..\..\externals\windows-installer\

set BUILDX86=
set BUILDX64=
set BUILDARM64=
set TARGET=Rebuild
set TESTTARGETDIR=
set PGO=-m test -q --pgo
set BUILDMSI=1
set BUILDNUGET=1
set BUILDZIP=1


:CheckOpts
if    "%1" EQU "-h" goto Help
if    "%1" EQU "-c" (set CERTNAME=%~2) && shift && shift && goto CheckOpts
if    "%1" EQU "--certificate" (set CERTNAME=%~2) && shift && shift && goto CheckOpts
if    "%1" EQU "-o" (set OUTDIR=%~2) && shift && shift && goto CheckOpts
if    "%1" EQU "--out" (set OUTDIR=%~2) && shift && shift && goto CheckOpts
if    "%1" EQU "-D" (set SKIPDOC=1) && shift && goto CheckOpts
if    "%1" EQU "--skip-doc" (set SKIPDOC=1) && shift && goto CheckOpts
if    "%1" EQU "-B" (set SKIPBUILD=1) && shift && goto CheckOpts
if    "%1" EQU "--skip-build" (set SKIPBUILD=1) && shift && goto CheckOpts
if    "%1" EQU "--download" (set DOWNLOAD_URL=%~2) && shift && shift && goto CheckOpts
if    "%1" EQU "--test" (set TESTTARGETDIR=%~2) && shift && shift && goto CheckOpts
if    "%1" EQU "-b" (set TARGET=Build) && shift && goto CheckOpts
if    "%1" EQU "--build" (set TARGET=Build) && shift && goto CheckOpts
if /I "%1" EQU "-x86" (set BUILDX86=1) && shift && goto CheckOpts
if /I "%1" EQU "-Win32" (set BUILDX86=1) && shift && goto CheckOpts
if /I "%1" EQU "-x64" (set BUILDX64=1) && shift && goto CheckOpts
if /I "%1" EQU "-arm64" (set BUILDARM64=1) && shift && goto CheckOpts
if    "%1" EQU "--pgo" (set PGO=%~2) && shift && shift && goto CheckOpts
if    "%1" EQU "--skip-pgo" (set PGO=) && shift && goto CheckOpts
if    "%1" EQU "--skip-nuget" (set BUILDNUGET=) && shift && goto CheckOpts
if    "%1" EQU "--skip-zip" (set BUILDZIP=) && shift && goto CheckOpts
if    "%1" EQU "--skip-msi" (set BUILDMSI=) && shift && goto CheckOpts

if "%1" NEQ "" echo Invalid option: "%1" && exit /B 1

if not defined BUILDX86 if not defined BUILDX64 if not defined BUILDARM64 (set BUILDX86=1) && (set BUILDX64=1)

if not exist "%GIT%" where git > "%TEMP%\git.loc" 2> nul && set /P GIT= < "%TEMP%\git.loc" & del "%TEMP%\git.loc"
if not exist "%GIT%" echo Cannot find Git on PATH && exit /B 1

call "%D%get_externals.bat"
call "%PCBUILD%find_msbuild.bat" %MSBUILD%
if ERRORLEVEL 1 (echo Cannot locate MSBuild.exe on PATH or as MSBUILD variable & exit /b 2)

:builddoc
if "%SKIPBUILD%" EQU "1" goto skipdoc
if "%SKIPDOC%" EQU "1" goto skipdoc

call "%D%..\..\doc\make.bat" html
if errorlevel 1 exit /B %ERRORLEVEL%
:skipdoc

if defined BUILDX86 (
    call :build x86
    if errorlevel 1 exit /B %ERRORLEVEL%
)

if defined BUILDX64 (
    call :build x64 "%PGO%"
    if errorlevel 1 exit /B %ERRORLEVEL%
)

if defined BUILDARM64 (
    call :build ARM64
    if errorlevel 1 exit /B %ERRORLEVEL%
)

if defined TESTTARGETDIR (
    call "%D%testrelease.bat" -t "%TESTTARGETDIR%"
    if errorlevel 1 exit /B %ERRORLEVEL%
)

exit /B 0

:build
@setlocal
@echo off

if "%1" EQU "x86" (
    set PGO=
    set BUILD=%Py_OutDir%win32\
    set BUILD_PLAT=Win32
    set OUTDIR_PLAT=win32
    set OBJDIR_PLAT=x86
) else if "%1" EQU "x64" (
    set BUILD=%Py_OutDir%amd64\
    set PGO=%~2
    set BUILD_PLAT=x64
    set OUTDIR_PLAT=amd64
    set OBJDIR_PLAT=x64
) else if "%1" EQU "ARM64" (
    set BUILD=%Py_OutDir%arm64\
    set PGO=%~2
    set BUILD_PLAT=ARM64
    set OUTDIR_PLAT=arm64
    set OBJDIR_PLAT=arm64
) else (
    echo Unknown platform %1
    exit /B 1
)

if exist "%BUILD%en-us" (
    echo Deleting %BUILD%en-us
    rmdir /q/s "%BUILD%en-us"
    if errorlevel 1 exit /B %ERRORLEVEL%
)

if exist "%D%obj\Debug_%OBJDIR_PLAT%" (
    echo Deleting "%D%obj\Debug_%OBJDIR_PLAT%"
    rmdir /q/s "%D%obj\Debug_%OBJDIR_PLAT%"
    if errorlevel 1 exit /B %ERRORLEVEL%
)

if exist "%D%obj\Release_%OBJDIR_PLAT%" (
    echo Deleting "%D%obj\Release_%OBJDIR_PLAT%"
    rmdir /q/s "%D%obj\Release_%OBJDIR_PLAT%"
    if errorlevel 1 exit /B %ERRORLEVEL%
)

if not "%CERTNAME%" EQU "" (
    set CERTOPTS="/p:SigningCertificate=%CERTNAME%"
) else (
    set CERTOPTS=
)
if not "%PGO%" EQU "" (
    set PGOOPTS=--pgo-job "%PGO%"
) else (
    set PGOOPTS=
)
if not "%SKIPBUILD%" EQU "1" (
    @echo call "%PCBUILD%build.bat" -e -p %BUILD_PLAT% -t %TARGET% %PGOOPTS% %CERTOPTS%
    @call "%PCBUILD%build.bat" -e -p %BUILD_PLAT% -t %TARGET% %PGOOPTS% %CERTOPTS%
    @if errorlevel 1 exit /B %ERRORLEVEL%
    @rem build.bat turns echo back on, so we disable it again
    @echo off

    @echo call "%PCBUILD%build.bat" -d -e -p %BUILD_PLAT% -t %TARGET%
    @call "%PCBUILD%build.bat" -d -e -p %BUILD_PLAT% -t %TARGET%
    @if errorlevel 1 exit /B %ERRORLEVEL%
    @rem build.bat turns echo back on, so we disable it again
    @echo off
)

if "%OUTDIR_PLAT%" EQU "win32" (
    %MSBUILD% "%D%launcher\launcher.wixproj" /p:Platform=x86 %CERTOPTS% /p:ReleaseUri=%RELEASE_URI%
    if errorlevel 1 exit /B %ERRORLEVEL%
) else if not exist "%Py_OutDir%win32\en-us\launcher.msi" (
    %MSBUILD% "%D%launcher\launcher.wixproj" /p:Platform=x86 %CERTOPTS% /p:ReleaseUri=%RELEASE_URI%
    if errorlevel 1 exit /B %ERRORLEVEL%
)

set BUILDOPTS=/p:Platform=%1 /p:BuildForRelease=true /p:DownloadUrl=%DOWNLOAD_URL% /p:DownloadUrlBase=%DOWNLOAD_URL_BASE% /p:ReleaseUri=%RELEASE_URI%
if defined BUILDMSI (
    %MSBUILD% "%D%bundle\releaselocal.wixproj" /t:Rebuild %BUILDOPTS% %CERTOPTS% /p:RebuildAll=true
    if errorlevel 1 exit /B %ERRORLEVEL%
)

if defined BUILDZIP (
    if "%BUILD_PLAT%" EQU "ARM64" (
        echo Skipping embeddable ZIP generation for ARM64 platform
    ) else (
        %MSBUILD% "%D%make_zip.proj" /t:Build %BUILDOPTS% %CERTOPTS% /p:OutputPath="%BUILD%en-us"
        if errorlevel 1 exit /B %ERRORLEVEL%
    )
)

if defined BUILDNUGET (
    if "%BUILD_PLAT%" EQU "ARM64" (
        echo Skipping Nuget package generation for ARM64 platform
    ) else (
        %MSBUILD% "%D%..\nuget\make_pkg.proj" /t:Build /p:Configuration=Release /p:Platform=%1 /p:OutputPath="%BUILD%en-us"
        if errorlevel 1 exit /B %ERRORLEVEL%
    )
)

if not "%OUTDIR%" EQU "" (
    mkdir "%OUTDIR%\%OUTDIR_PLAT%"
    mkdir "%OUTDIR%\%OUTDIR_PLAT%\binaries"
    mkdir "%OUTDIR%\%OUTDIR_PLAT%\symbols"
    robocopy "%BUILD%en-us" "%OUTDIR%\%OUTDIR_PLAT%" /XF "*.wixpdb"
    robocopy "%BUILD%\" "%OUTDIR%\%OUTDIR_PLAT%\binaries" *.exe *.dll *.pyd /XF "_test*" /XF "*_d.*" /XF "_freeze*" /XF "tcl*" /XF "tk*" /XF "*_test.*"
    robocopy "%BUILD%\" "%OUTDIR%\%OUTDIR_PLAT%\symbols" *.pdb              /XF "_test*" /XF "*_d.*" /XF "_freeze*" /XF "tcl*" /XF "tk*" /XF "*_test.*"
)

exit /B 0

:Help
echo buildrelease.bat [--out DIR] [-x86] [-x64] [-arm64] [--certificate CERTNAME] [--build] [--pgo COMMAND]
echo                  [--skip-build] [--skip-doc] [--skip-nuget] [--skip-zip] [--skip-pgo]
echo                  [--download DOWNLOAD URL] [--test TARGETDIR]
echo                  [-h]
echo.
echo    --out (-o)          Specify an additional output directory for installers
echo    -x86                Build x86 installers
echo    -x64                Build x64 installers
echo    -arm64              Build ARM64 installers
echo    --build (-b)        Incrementally build Python rather than rebuilding
echo    --skip-build (-B)   Do not build Python (just do the installers)
echo    --skip-doc (-D)     Do not build documentation
echo    --pgo               Specify PGO command for x64 installers
echo    --skip-pgo          Build x64 installers without using PGO
echo    --skip-msi          Do not build executable/MSI packages
echo    --skip-nuget        Do not build Nuget packages
echo    --skip-zip          Do not build embeddable package
echo    --download          Specify the full download URL for MSIs
echo    --test              Specify the test directory to run the installer tests
echo    -h                  Display this help information
echo.
echo If no architecture is specified, all architectures will be built.
echo If --test is not specified, the installer tests are not run.
echo.
echo For the --pgo option, any Python command line can be used, or 'default' to
echo use the default task (-m test --pgo).
echo.
echo x86 and ARM64 builds will never use PGO. ARM64 builds will never generate
echo embeddable or Nuget packages.
echo.
echo The following substitutions will be applied to the download URL:
echo     Variable        Description         Example
echo     {version}       version number      3.5.0
echo     {arch}          architecture        amd64, win32
echo     {releasename}   release name        a1, b2, rc3 (or blank for final)
echo     {msi}           MSI filename        core.msi
