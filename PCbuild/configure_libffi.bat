@echo off
goto :Run

:Usage
echo.
echo Before running %~nx0 
echo   LIBFFI_SOURCE environment variable must be set to the location of
echo     of python-source-deps clone of libffi branch
echo   VCVARSALL must be set to location of vcvarsall.bat
echo   cygwin must be installed (see below)
echo   SH environment variable must be set to the location of sh.exe
echo.
echo Tested with cygwin-x86 from https://www.cygwin.com/install.html
echo Select http://mirrors.kernel.org as the download site
echo Include the following cygwin packages in cygwin configuration:
echo     make, autoconf, automake, libtool
echo.
echo Based on https://github.com/libffi/libffi/blob/master/.appveyor.yml
echo.
exit /b 127

:Run
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

set GET_MSVCC=%SH% -lc "cd $OLDPWD; export MSVCC=`/usr/bin/find $PWD -name msvcc.sh`; echo ${MSVCC};"
FOR /F "usebackq delims==" %%i IN (`%GET_MSVCC%`) do @set MSVCC=%%i

echo.
echo VCVARSALL    : %VCVARSALL%
echo SH           : %SH%
echo LIBFFI_SOURCE: %LIBFFI_SOURCE% 
echo MSVCC        : %MSVCC%
echo.

%SH% -lc "(cd $LIBFFI_SOURCE; ./autogen.sh;)"

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
set ML=

if "%VCVARS_PLATFORM%" EQU "" echo ERROR bad VCVARS_PLATFORM&&exit /b 123
if /I "%VCVARS_PLATFORM%" EQU "x64" (set ML=-m64)

REM get VS build environment
call %VCVARSALL% %VCVARS_PLATFORM%

REM clean header output directory
if exist %LIBFFI_SOURCE%\include\%VSCMD_ARG_TGT_ARCH% (rd %LIBFFI_SOURCE%\include\%VSCMD_ARG_TGT_ARCH% /s/q)

REM just configure the build to generate fficonfig.h and ffi.h
%SH% -lc "(cd $OLDPWD; ./configure CC='%MSVCC% %ML%' CXX='%MSVCC% %ML%' LD='link' CPP='cl -nologo -EP' CXXCPP='cl -nologo -EP' CPPFLAGS='-DFFI_BUILDING_DLL' NM='dumpbin -symbols' STRIP=':' --build=$BUILD --host=$HOST;)"

REM There is no support for building .DLLs currently.
REM %SH% -lc "(cd $OLDPWD; cp src/x86/ffitarget.h include; make; find .;)"

REM Running the libffi tests doesn't work when using msvc
REM msvcc.sh is missing support for -l and -L according to the note in appveyor.yml
REM %SH% -lc "(cd $OLDPWD; cp `find . -name 'libffi-?.dll'` $HOST/testsuite/; make check; cat `find ./ -name libffi.log`)"

REM create header output directory and copy headers to check-in location
if not exist %LIBFFI_SOURCE%\include\%VSCMD_ARG_TGT_ARCH% (md %LIBFFI_SOURCE%\include\%VSCMD_ARG_TGT_ARCH%)
copy %LIBFFI_SOURCE%\%HOST%\fficonfig.h  %LIBFFI_SOURCE%\include\%VSCMD_ARG_TGT_ARCH%
copy %LIBFFI_SOURCE%\%HOST%\include\ffi.h  %LIBFFI_SOURCE%\include\%VSCMD_ARG_TGT_ARCH%

endlocal
exit /b

endlocal