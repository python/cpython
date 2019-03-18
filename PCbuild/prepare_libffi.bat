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
echo.  -?      this help
echo.  --install-cygwin  install cygwin to c:\cygwin
exit /b 127

:Run

set BUILD_X64=
set BUILD_X86=
set INSTALL_CYGWIN=

:CheckOpts
if "%1"=="" goto :CheckOptsDone
if /I "%1"=="-x64" (set BUILD_X64=1) & shift & goto :CheckOpts
if /I "%1"=="-x86" (set BUILD_X86=1) & shift & goto :CheckOpts
if /I "%1"=="-?" goto :Usage
if /I "%1"=="--install-cygwin" (set INSTALL_CYGWIN=1) & shift & goto :CheckOpts
goto :Usage

:CheckOptsDone

if "%BUILD_X64%"=="" if "%BUILD_X86%"=="" if "%BUILD_ARM32%"=="" (
    set BUILD_X64=1
    set BUILD_X86=1
)

if "%INSTALL_CYGWIN%"=="1" call :InstallCygwin

setlocal
if "%SH%" EQU "" if exist c:\cygwin\bin\sh.exe set SH=c:\cygwin\bin\sh.exe

if (%VCVARSALL%) EQU () (
    if exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" (
        set VCVARSALL="C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
    )
)

if "%LIBFFI_SOURCE%" EQU "" echo.&&echo ERROR LIBFFI_SOURCE environment variable not set && goto :Usage
if "%SH%" EQU "" echo ERROR SH environment variable not set && goto :Usage

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

if not exist Makefile.in (%SH% -lc "(cd $LIBFFI_SOURCE; ./autogen.sh;)")

call :BuildOne x86 i686-pc-cygwin i686-pc-cygwin
call :BuildOne x64 x86_64-w64-cygwin x86_64-w64-cygwin

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

if "%VCVARS_PLATFORM%" EQU "" echo ERROR bad VCVARS_PLATFORM&&exit /b 123

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

set LIBFFI_OUT=%~dp0\..\externals\libffi-bin-3.3.0-rc0-r1\%ARCH%

echo get VS build environment
call %VCVARSALL% %VCVARS_PLATFORM%

echo clean %LIBFFI_OUT%
if exist %LIBFFI_OUT% (rd %LIBFFI_OUT% /s/q)

echo Configure the build to generate fficonfig.h and ffi.h
%SH% -lc "(cd $OLDPWD; ./configure CC='%MSVCC% %ASSEMBLER%' CXX='%MSVCC% %ASSEMBLER%' LD='link' CPP='cl -nologo -EP' CXXCPP='cl -nologo -EP' CPPFLAGS='-DFFI_BUILDING_DLL' NM='dumpbin -symbols' STRIP=':' --build=$BUILD --host=$HOST;)"

echo Building libffi
%SH% -lc "(cd $OLDPWD; export PATH=/usr/bin:$PATH; cp src/%SRC_ARCHITECTURE%/ffitarget.h include; make; find .;)"

REM Tests are not needed to produce artifacts
if "%LIBFFI_TEST%" EQU "1" (
    echo "Running tests..."
    %SH% -lc "(cd $OLDPWD; export PATH=/usr/bin:$PATH; cp `find $PWD -name 'libffi-?.dll'` $HOST/testsuite/; make check; cat `find ./ -name libffi.log`)"
) else (
    echo "Not running tests"
)


echo copying files to %LIBFFI_OUT%
if not exist %LIBFFI_OUT%\include (md %LIBFFI_OUT%\include)
copy %ARTIFACTS%\.libs\libffi-7.dll %LIBFFI_OUT%
copy %ARTIFACTS%\.libs\libffi-7.lib %LIBFFI_OUT%
copy %ARTIFACTS%\fficonfig.h %LIBFFI_OUT%\include
copy %ARTIFACTS%\include\*.h %LIBFFI_OUT%\include

endlocal
exit /b

:InstallCygwin
setlocal

if "%CYG_ROOT%"=="" (set CYG_ROOT=c:/cygwin)
if "%CYG_CACHE%"=="" (set CYG_CACHE=C:/cygwin/var/cache/setup)
if "%CYG_MIRROR%"=="" (set CYG_MIRROR=http://mirrors.kernel.org/sourceware/cygwin/)

powershell -c "md $env:CYG_ROOT -ErrorAction SilentlyContinue"
powershell -c "$setup = $env:CYG_ROOT+'/setup.exe'; if (!(Test-Path $setup)){invoke-webrequest https://cygwin.com/setup-x86.exe -outfile $setup}
%CYG_ROOT%/setup.exe -qnNdO -R "%CYG_ROOT%" -s "%CYG_MIRROR%" -l "%CYG_CACHE%" -P make -P autoconf -P automake -P libtool -P dejagnu

endlocal
exit /b

