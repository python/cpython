@echo off
goto :Run

:Usage
echo.
echo Before running prepare_ncurses.bat 
echo   NCURSES_SOURCE environment variable must be set to the location of
echo     of python-source-deps clone of ncurses branch
echo   VCVARSALL must be set to location of vcvarsall.bat
echo   cygwin must be installed (see below)
echo   SH environment variable must be set to the location of sh.exe
echo.
echo Tested with cygwin-x86 from https://www.cygwin.com/install.html
echo Select http://mirrors.kernel.org as the download site
echo Include the following cygwin packages in cygwin configuration:
echo     awk cat chmod date make rm sed
echo.
echo Based on https://github.com/conan-io/conan-center-index/blob/master/recipes/ncurses/all/conanfile.py
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
if /I "%1"=="-pdb" (set BUILD_PDB=-debug) & shift & goto :CheckOpts
if /I "%1"=="-noopt" (set BUILD_NOOPT=1) & shift & goto :CheckOpts
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

if DEFINED BUILD_NOOPT (
    set OPTCFLAGS=-O0
) else (
    set OPTCFLAGS=-O2 -Ob2
)

if DEFINED BUILD_PDB (
    set OPTCFLAGS=%OPTCFLAGS% -debug -MDd
    set OPTLDFLAGS=-debug -pdb
) else (
    set OPTCFLAGS=%OPTCFLAGS -MD
)

if "%INSTALL_CYGWIN%"=="1" call :InstallCygwin

setlocal
if NOT DEFINED SH if exist c:\cygwin\bin\sh.exe set SH=c:\cygwin\bin\sh.exe

if NOT DEFINED VCVARSALL (
    if exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" (
        set VCVARSALL="C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
    ) else (
        echo VCVARSALL could not be auto-detected. Please define it.
        exit /b 127
    )
)
if ^%VCVARSALL:~0,1% NEQ ^" SET VCVARSALL="%VCVARSALL%"

if NOT DEFINED NCURSES_SOURCE echo.&&echo ERROR NCURSES_SOURCE environment variable not set && goto :Usage
if NOT DEFINED SH echo ERROR SH environment variable not set && goto :Usage

if not exist %SH% echo ERROR %SH% does not exist && goto :Usage
if not exist %NCURSES_SOURCE% echo ERROR %NCURSES_SOURCE% does not exist && goto :Usage

pushd %NCURSES_SOURCE%

%SH% --login -lc "cygcheck -dc cygwin"

echo.
echo VCVARSALL     : %VCVARSALL%
echo SH            : %SH%
echo NCURSES_SOURCE: %NCURSES_SOURCE% 
echo.

if "%BUILD_X64%"=="1" call :BuildOne x64 x86_64-w64-mingw32-msvc7 x86_64-w64-mingw32-msvc7
if "%BUILD_X86%"=="1" call :BuildOne x86 i686-pc-mingw32-msvc7 i686-pc-mingw32-msvc7
if "%BUILD_ARM32%"=="1" call :BuildOne x86_arm i686-pc-mingw32-msvc7 arm-w32-mingw32-msvc7
if "%BUILD_ARM64%"=="1" call :BuildOne x86_arm64 i686-pc-mingw32-msvc7 aarch64-w64-mingw32-msvc7

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
)
if /I "%VCVARS_PLATFORM%" EQU "x86" (
    set ARCH=win32
)
if /I "%VCVARS_PLATFORM%" EQU "x86_arm" (
    set ARCH=arm32
)
if /I "%VCVARS_PLATFORM%" EQU "x86_arm64" (
    set ARCH=arm64
)

set BUILDDIR=%NCURSES_SOURCE%\%HOST%
md %BUILDDIR%

if NOT DEFINED NCURSES_OUT set NCURSES_OUT=%~dp0\..\externals\ncurses
set _NCURSES_OUT=%NCURSES_OUT%\%ARCH%

echo get VS build environment
call %VCVARSALL% %VCVARS_PLATFORM%

echo clean %_NCURSES_OUT%
if exist %_NCURSES_OUT% (rd %_NCURSES_OUT% /s/q)

echo ================================================================
echo Configure the build
echo ================================================================
%SH% -lc "(cd $BUILDDIR; sh ../configure CC='cl -nologo' CFLAGS='%OPTFLAGS%' LDFLAGS='%OPTLDFLAG%' AR='lib' LD='link -nologo' CPP='cl -nologo -EP' CPPFLAGS='' NM='dumpbin -symbols' RANLIB=':' STRIP=':' --enable-widec --disable-ext-colors --disable-reentrant --without-pcre2 --without-cxx-binding --without-progs --with-shared --without-normal --without-debug --without-tests --disable-macros --disable-termcap --enable-database --enable-sp-funcs --enable-term-driver --enable-interop ac_cv_func_getopt=yes --build=$BUILD --host=$HOST;)"
if errorlevel 1 exit /B %ERRORLEVEL%

echo ================================================================
echo Building ncurses
echo ================================================================
%SH% -lc "(cd $BUILDDIR; make -j4;)"
if errorlevel 1 exit /B %ERRORLEVEL%

REM Tests are not built because MSVC does not have getopt
REM Tests are not needed to produce artifacts
if "%NCURSES_TEST%" EQU "1" (
    echo "Running tests..."
    %SH% -lc "(cd $BUILDDIR; make test;)"
) else (
    echo "Not running tests"
)

echo copying files to %_NCURSES_OUT%
if not exist %_NCURSES_OUT%\include (md %_NCURSES_OUT%\include)
copy %NCURSES_SOURCE%/COPYING %NCURSES_OUT%
copy %BUILDDIR%\lib\ncursesw6.dll %_NCURSES_OUT%
copy %BUILDDIR%\lib\ncursesw.dll.lib %_NCURSES_OUT%
copy %BUILDDIR%\lib\panelw6.dll %_NCURSES_OUT%
copy %BUILDDIR%\lib\panelw.dll.lib %_NCURSES_OUT%
copy %BUILDDIR%\lib\menuw6.dll %_NCURSES_OUT%
copy %BUILDDIR%\lib\menuw.dll.lib %_NCURSES_OUT%
copy %BUILDDIR%\lib\formw6.dll %_NCURSES_OUT%
copy %BUILDDIR%\lib\formw.dll.lib %_NCURSES_OUT%
copy %BUILDDIR%\include\*.h %_NCURSES_OUT%\include
copy %BUILDDIR%\include\curses.h %_NCURSES_OUT%\include\ncurses.h

endlocal
exit /b

:InstallCygwin
setlocal

if NOT DEFINED CYG_ROOT (set CYG_ROOT=c:/cygwin)
if NOT DEFINED CYG_CACHE (set CYG_CACHE=C:/cygwin/var/cache/setup)
if NOT DEFINED CYG_MIRROR (set CYG_MIRROR=http://mirrors.kernel.org/sourceware/cygwin/)

powershell -c "md $env:CYG_ROOT -ErrorAction SilentlyContinue"
powershell -c "$setup = $env:CYG_ROOT+'/setup.exe'; if (!(Test-Path $setup)){invoke-webrequest https://cygwin.com/setup-x86.exe -outfile $setup}
%CYG_ROOT%/setup.exe -qnNdO -R "%CYG_ROOT%" -s "%CYG_MIRROR%" -l "%CYG_CACHE%" -P make -P awk -P sed -P rm -P cat -P chmod -P date

endlocal
exit /b

