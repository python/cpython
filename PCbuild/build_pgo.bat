@echo off
rem A batch program to build PGO (Profile guided optimization) by first
rem building instrumented binaries, then running the testsuite, and
rem finally building the optimized code.
rem Note, after the first instrumented run, one can just keep on
rem building the PGUpdate configuration while developing.

setlocal
set platf=Win32
set dir=%~dp0

rem use the performance testsuite.  This is quick and simple
set job1="%dir%..\tools\pybench\pybench.py" -n 1 -C 1 --with-gc
set path1="%dir%..\tools\pybench"

rem or the whole testsuite for more thorough testing
set job2="%dir%..\lib\test\regrtest.py"
set path2="%dir%..\lib"

set job=%job1%
set clrpath=%path1%

:CheckOpts
if "%1"=="-p" (set platf=%2) & shift & shift & goto CheckOpts
if "%1"=="-2" (set job=%job2%) & (set clrpath=%path2%) & shift & goto CheckOpts

set PGI=%dir%%platf%-pgi
set PGO=%dir%%platf%-pgo

rem We cannot cross compile PGO builds, as the optimization needs to be run natively
set vs_platf=x86
if "%platf%"=="x64" (set vs_platf=amd64)
rem Setup the environment
call "%VS100COMNTOOLS%..\..\VC\vcvarsall.bat" %vs_platf%

rem build the instrumented version
msbuild "%dir%pcbuild.sln" /t:Build /p:Configuration=PGInstrument /p:Platform=%platf% %1 %2 %3 %4 %5 %6 %7 %8 %9

rem remove .pyc files, .pgc files and execute the job
"%PGI%\python.exe" "%dir%rmpyc.py" %clrpath%
del "%PGI%\*.pgc"
"%PGI%\python.exe" %job%

rem clean
if exist "%PGO%" del /s /q "%PGO%"

rem build optimized version
msbuild "%dir%pcbuild.sln" /t:Build /p:Configuration=PGUpdate /p:Platform=%platf% %1 %2 %3 %4 %5 %6 %7 %8 %9
