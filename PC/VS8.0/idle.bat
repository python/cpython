@echo off
rem start idle
rem Usage:  idle [-d]
rem -d   Run Debug build (python_d.exe).  Else release build.

setlocal
set exe=python
PATH %PATH%;..\..\..\tcltk\bin

if "%1"=="-d" (set exe=python_d) & shift

set cmd=%exe% ../../Lib/idlelib/idle.py %1 %2 %3 %4 %5 %6 %7 %8 %9

echo on
%cmd%
