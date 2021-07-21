if not exist c:\mnt\ goto nomntdir
@echo c:\mnt found, continuing
cd C:\mnt

set platf=Win32
set builddir=c:\mnt\PCBuild\win32
set outdir=c:\mnt\build-out
set py_version=3.8.10

mkdir %outdir%
if not exist %outdir% exit /b 3

if "%TARGET_ARCH%" == "x64" (
    @echo IN x64 BRANCH
    call %VSTUDIO_ROOT%\VC\Auxiliary\Build\vcvars64.bat
    set platf=x64
    set builddir=%builddir%\amd64
)

if "%TARGET_ARCH%" == "x86" (
    @echo IN x86 BRANCH
    call %VSTUDIO_ROOT%\VC\Auxiliary\Build\vcvars32.bat
)

call ridk enable

REM First, get the required dependencies
call .\PCBuild\get_externals.bat --organization python --no-tkinter --python 3.8

REM -e: Build external libraries fetched by get_externals.bat
REM Do not use -m (Enable parallel build) - the first build will fail since there may be a race condition
REM with libraries needing pythoncore to be built
call .\PCBuild\build.bat --no-tkinter -e -c Release -p %platf%

REM Copy DLLs directory
mkdir %outdir%\DLLs
copy %builddir%\*.pyd %outdir%\DLLs || exit /b 4

REM Copy include directory
robocopy .\include %outdir%\include /MIR /NFL /NDL /NJH /NJS /nc /ns /np
if %ERRORLEVEL% GEQ 8 exit /b 5
copy PC\pyconfig.h %outdir%\include || exit /b 6

REM Copy Lib directory
robocopy .\Lib %outdir%\Lib /MIR /NFL /NDL /NJH /NJS /nc /ns /np /XF *.pyc /XD __pycache__
if %ERRORLEVEL% GEQ 8 exit /b 7

REM Copy libs directory
mkdir %outdir%\libs
copy %builddir%\*.lib %outdir%\libs || exit /b 8

REM Copy files in root directory
copy %builddir%\python3.dll %outdir% || exit /b 9
copy %builddir%\python38.dll %outdir% || exit /b 10
copy %builddir%\python.exe %outdir% || exit /b 11
copy %builddir%\pythonw.exe %outdir% || exit /b 12

REM Generate import library
cd %builddir%
gendef python38.dll
if "%TARGET_ARCH%" == "x64" (
    dlltool -m i386:x86-64 --dllname python38.dll --def python38.def --output-lib libpython38.a
)
if "%TARGET_ARCH%" == "x86" (
    dlltool -m i386 --as-flags=--32 --dllname python38.dll --def python38.def --output-lib libpython38.a
)
copy libpython38.a %outdir%\libs || exit /b 13

REM Generate python zip
7z a -r %outdir%\python-windows-%py_version%-%TARGET_ARCH%.zip %outdir%\*

goto :EOF

:nomntdir
@echo directory not mounted, parameters incorrect
exit /b 1
goto :EOF