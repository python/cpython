@echo off
rem A batch program to build PGO (Profile guided optimization) by first
rem building instrumented binaries, then running the testsuite, and
rem finally building the optimized code.
rem Note, after the first instrumented run, one can just keep on
rem building the PGUpdate configuration while developing.

setlocal
set platf=Win32

rem use the performance testsuite.  This is quick and simple
set job1=..\tools\pybench\pybench.py -n 1 -C 1 --with-gc
set path1=..\tools\pybench

rem or the whole testsuite for more thorough testing
set job2=..\lib\test\regrtest.py
set path2=..\lib

set job=%job1%
set clrpath=%path1%

:CheckOpts
if "%1"=="-p" (set platf=%2) & shift & shift & goto CheckOpts
if "%1"=="-2" (set job=%job2%) & (set clrpath=%path2%) & shift & goto CheckOpts

set folder=%platf%-pgo


@echo on
rem build the instrumented version
call build -r -p %platf% -c PGInstrument

rem remove .pyc files, .pgc files and execute the job
%folder%\python.exe rmpyc.py %clrpath%
del %folder%\*.pgc
%folder%\python.exe %job%

rem finally build the optimized version
call build -r -p %platf% -c PGUpdate

