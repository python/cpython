REM tested with cygwin-x86 from https://www.cygwin.com/install.html
REM select http://mirrors.kernel.org as the download site
REM include the following packages: make, autoconf, automake, libtool

if not exist c:\cygwin\bin\sh.exe (echo ERROR cygwin-x86 is required to run this script)
set LIBFFI_SOURCE=e:\git\libffi
set MSVCC=/cygdrive/e/git/libffi/msvcc.sh
set OLDPWD=%LIBFFI_SOURCE%
cd /d %LIBFFI_SOURCE%

call :BuildOne x86 i686-pc-cygwin
call :BuildOne amd64 x86_64-w64-cygwin
goto :EOF

:BuildOne

setlocal

set VCVARS_PLATFORM=%1
set BUILD=%2
set HOST=%2
set ML=
if /I "%VCVARS_PLATFORM%" EQU "amd64" (set ML=-m64)

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" %VCVARS_PLATFORM%

REM just configure the build to fficonfig.h and ffi.h
c:\cygwin\bin\sh -lc "(cd $OLDPWD; ./autogen.sh;)"
c:\cygwin\bin\sh -lc "(cd $OLDPWD; ./configure CC='$MSVCC $ML' CXX='$MSVCC $ML' LD='link' CPP='cl -nologo -EP' CXXCPP='cl -nologo -EP' CPPFLAGS='-DFFI_BUILDING_DLL' NM='dumpbin -symbols' STRIP=':' --build=$BUILD --host=$HOST;)"

REM There is no support for building .DLLs currently.  It looks possible to get a static library.
REM c:\cygwin\bin\sh -lc "(cd $OLDPWD; cp src/x86/ffitarget.h include; make; find .;)"

REM Running the libffi tests doesn't when using msvc
REM msvcc.sh is missing support for -l and -L according to the note in appveyor.yml
REM c:\cygwin\bin\sh -lc "(cd $OLDPWD; cp `find . -name 'libffi-?.dll'` $HOST/testsuite/; make check; cat `find ./ -name libffi.log`)"

REM # FIXME: "make check" currently fails.  It just looks like msvcc needs
REM # to learn about -L and -l options.  If you add "make check; cat `find
REM # ./ -name libffi.log" to the end of that build command you'll see
REM # what I mean.

endlocal
exit /b

endlocal