@echo off
rem Set these values according to where you installed the software
rem You need to install the necessary bits mentioned in:
rem http://wiki.python.org/moin/Building_Python_with_the_free_MS_C_Toolkit

set TOOLKIT=%ProgramFiles%\Microsoft Visual C++ Toolkit 2003
set SDK=%ProgramFiles%\Microsoft Platform SDK for Windows Server 2003 R2
set NET=%ProgramFiles%\Microsoft Visual Studio .NET 2003
set NANT=%ProgramFiles%\Nant

set PATH=%TOOLKIT%\bin;%PATH%;%SDK%\Bin\Win64;%NANT%\bin;%SDK%\bin
set INCLUDE=%TOOLKIT%\include;%SDK%\Include;%INCLUDE%
set LIB=%TOOLKIT%\lib;%NET%\VC7\lib;%SDK%\lib;%LIB%

echo Build environment for Python
echo TOOLKIT=%TOOLKIT%
echo SDK=%SDK%
echo NET=%NET%
echo NANT=%NANT%
echo Commands:
echo * build 
echo * rt
