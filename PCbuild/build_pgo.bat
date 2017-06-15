@echo off
echo.DeprecationWarning:
echo.    This script is deprecated, use `build.bat --pgo` instead.
echo.

call "%~dp0build.bat" --pgo %*
