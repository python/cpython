@echo off
goto :Run

:Usage
echo.
echo Before running prepare_libffi.bat 
echo   LIBFFI_SOURCE environment variable must be set to the location of
echo     of python-source-deps clone of libffi branch
echo   VCVARSALL must be set to location of vcvarsall.bat
echo   cygwin must be installed (see below)
echo   SH environment variable must be set to the location of sh.exe
echo.
echo Tested with cygwin-x86 from https://www.cygwin.com/install.html
echo Select http://mirrors.kernel.org as the download site
echo Include the following cygwin packages in cygwin configuration:
echo     make, autoconf, automake, libtool, dejagnu
echo.
echo NOTE: dejagnu is only required for running tests.
echo       set LIBFFI_TEST=1 to run tests (optional)
echo.
echo Based on https://github.com/libffi/libffi/blob/master/.appveyor.yml
echo.
echo.
echo.Available flags:
echo.  -x64    build for x64
echo.  -x86    build for x86
echo.  -arm32  build for arm32
echo.  -arm64  build for arm64
echo.  -?      this help
echo.  --install-cygwin  install cygwin to c:\cygwin
exit /b 127

:Run

set BUILD_X64=
set BUILD_X86=
set BUILD_ARM32=
set BUILD_ARM64=
set BUILD_PDB=
set BUILD_NOOPT=
set INSTALL_CYGWIN=

:CheckOpts
if "%1"=="" goto :CheckOptsDone
if /I "%1"=="-x64" (set BUILD_X64=1) & shift & goto :CheckOpts
if /I "%1"=="-x86" (set BUILD_X86=1) & shift & goto :CheckOpts
if /I "%1"=="-arm32" (set BUILD_ARM32=1) & shift & goto :CheckOpts
if /I "%1"=="-arm64" (set BUILD_ARM64=1) & shift & goto :CheckOpts
if /I "%1"=="-pdb" (set BUILD_PDB=-g) & shift & goto :CheckOpts
if /I "%1"=="-noopt" (set BUILD_NOOPT=CFLAGS='-Od -warn all') & shift & goto :CheckOpts
if /I "%1"=="-?" goto :Usage
if /I "%1"=="--install-cygwin" (set INSTALL_CYGWIN=1) & shift & goto :CheckOpts
goto :Usage

:CheckOptsDone

if NOT DEFINED BUILD_X64 if NOT DEFINED BUILD_X86 if NOT DEFINED BUILD_ARM32 if NOT DEFINED BUILD_ARM64 (
    set BUILD_X64=1
    set BUILD_X86=1
    set BUILD_ARM32=1
    set BUILD_ARM64=1
)

if "%INSTALL_CYGWIN%"=="1" call :InstallCygwin

setlocal
if NOT DEFINED SH if exist c:\cygwin\bin\sh.exe set SH=c:\cygwin\bin\sh.exe

if NOT DEFINED VCVARSALL (
    if exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" (
        set VCVARSALL="C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
    )
)
if ^%VCVARSALL:~0,1% NEQ ^" SET VCVARSALL="%VCVARSALL%"

if NOT DEFINED LIBFFI_SOURCE echo.&&echo ERROR LIBFFI_SOURCE environment variable not set && goto :Usage
if NOT DEFINED SH echo ERROR SH environment variable not set && goto :Usage

if not exist %SH% echo ERROR %SH% does not exist && goto :Usage
if not exist %LIBFFI_SOURCE% echo ERROR %LIBFFI_SOURCE% does not exist && goto :Usage

set OLDPWD=%LIBFFI_SOURCE%
pushd %LIBFFI_SOURCE%

%SH% --login -lc "cygcheck -dc cygwin"
set GET_MSVCC=%SH% -lc "cd $OLDPWD; export MSVCC=`/usr/bin/find $PWD -name msvcc.sh`; echo ${MSVCC};"
FOR /F "usebackq delims==" %%i IN (`%GET_MSVCC%`) do @set MSVCC=%%i

echo.
echo VCVARSALL    : %VCVARSALL%
echo SH           : %SH%
echo LIBFFI_SOURCE: %LIBFFI_SOURCE% 
echo MSVCC        : %MSVCC%
echo.

if not exist Makefile.in (
    %SH% -lc "(cd $LIBFFI_SOURCE; ./autogen.sh;)"
    if errorlevel 1 exit /B 1
)

if "%BUILD_X64%"=="1" call :BuildOne x64 x86_64-w64-cygwin x86_64-w64-cygwin
if "%BUILD_X86%"=="1" call :BuildOne x86 i686-pc-cygwin i686-pc-cygwin
if "%BUILD_ARM32%"=="1" call :BuildOne x86_arm i686-pc-cygwin arm-w32-cygwin
if "%BUILD_ARM64%"=="1" call :BuildOne x86_arm64 i686-pc-cygwin aarch64-w64-cygwin

popd
endlocal
exit /b 0
REM all done


REM this subroutine is called once for each architecture
:BuildOne

setlocal

REM Initialize variables
set VCVARS_PLATFORM=%1
set BUILD=%2
set HOST=%3
set ASSEMBLER=
set SRC_ARCHITECTURE=x86

if NOT DEFINED VCVARS_PLATFORM echo ERROR bad VCVARS_PLATFORM&&exit /b 123

if /I "%VCVARS_PLATFORM%" EQU "x64" (
    set ARCH=amd64
    set ARTIFACTS=%LIBFFI_SOURCE%\x86_64-w64-cygwin
    set ASSEMBLER=-m64
    set SRC_ARCHITECTURE=x86
)
if /I "%VCVARS_PLATFORM%" EQU "x86" (
    set ARCH=win32
    set ARTIFACTS=%LIBFFI_SOURCE%\i686-pc-cygwin
    set ASSEMBLER=
    set SRC_ARCHITECTURE=x86
)
if /I "%VCVARS_PLATFORM%" EQU "x86_arm" (
    set ARCH=arm32
    set ARTIFACTS=%LIBFFI_SOURCE%\arm-w32-cygwin
    set ASSEMBLER=-marm
    set SRC_ARCHITECTURE=ARM
)
if /I "%VCVARS_PLATFORM%" EQU "x86_arm64" (
    set ARCH=arm64
    set ARTIFACTS=%LIBFFI_SOURCE%\aarch64-w64-cygwin
    set ASSEMBLER=-marm64
    set SRC_ARCHITECTURE=aarch64
)

if NOT DEFINED LIBFFI_OUT set LIBFFI_OUT=%~dp0\..\externals\libffi
set _LIBFFI_OUT=%LIBFFI_OUT%\%ARCH%

echo get VS build environment
call %VCVARSALL% %VCVARS_PLATFORM%

echo clean %_LIBFFI_OUT%
if exist %_LIBFFI_OUT% (rd %_LIBFFI_OUT% /s/q)

echo ================================================================
echo Configure the build to generate fficonfig.h and ffi.h
echo ================================================================
%SH% -lc "(cd $OLDPWD; ./configure CC='%MSVCC% %ASSEMBLER% %BUILD_PDB%' CXX='%MSVCC% %ASSEMBLER% %BUILD_PDB%' LD='link' CPP='cl -nologo -EP' CXXCPP='cl -nologo -EP' CPPFLAGS='-DFFI_BUILDING_DLL' %BUILD_NOOPT% NM='dumpbin -symbols' STRIP=':' --build=$BUILD --host=$HOST;)"
if errorlevel 1 exit /B %ERRORLEVEL%

echo ================================================================
echo Building libffi
echo ================================================================
%SH% -lc "(cd $OLDPWD; export PATH=/usr/bin:$PATH; cp src/%SRC_ARCHITECTURE%/ffitarget.h include; make; find .;)"
if errorlevel 1 exit /B %ERRORLEVEL%

REM Tests are not needed to produce artifacts
if "%LIBFFI_TEST%" EQU "1" (
    echo "Running tests..."
    %SH% -lc "(cd $OLDPWD; export PATH=/usr/bin:$PATH; cp `find $PWD -name 'libffi-?.dll'` $HOST/testsuite/; make check; cat `find ./ -name libffi.log`)"
) else (
    echo "Not running tests"
)


echo copying files to %_LIBFFI_OUT%
if not exist %_LIBFFI_OUT%\include (md %_LIBFFI_OUT%\include)
copy %ARTIFACTS%\.libs\libffi-7.dll %_LIBFFI_OUT%
copy %ARTIFACTS%\.libs\libffi-7.lib %_LIBFFI_OUT%
copy %ARTIFACTS%\.libs\libffi-7.pdb %_LIBFFI_OUT%
copy %ARTIFACTS%\fficonfig.h %_LIBFFI_OUT%\include
copy %ARTIFACTS%\include\*.h %_LIBFFI_OUT%\include

endlocal
exit /b

:InstallCygwin
setlocal

if NOT DEFINED CYG_ROOT (set CYG_ROOT=c:/cygwin)
if NOT DEFINED CYG_CACHE (set CYG_CACHE=C:/cygwin/var/cache/setup)
if NOT DEFINED CYG_MIRROR (set CYG_MIRROR=http://mirrors.kernel.org/sourceware/cygwin/)

powershell -c "md $env:CYG_ROOT -ErrorAction SilentlyContinue"
powershell -c "$setup = $env:CYG_ROOT+'/setup.exe'; if (!(Test-Path $setup)){invoke-webrequest https://cygwin.com/setup-x86.exe -outfile $setup}
%CYG_ROOT%/setup.exe -qnNdO -R "%CYG_ROOT%" -s "%CYG_MIRROR%" -l "%CYG_CACHE%" -P make -P autoconf -P automake -P libtool -P dejagnu

endlocal
exit /b

