@echo off
rem A batch program to clean a particular configuration,
rem just for convenience.

call %~dp0build.bat -t Clean %*
