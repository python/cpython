@echo off
rem Try to find the AMD64 assembler and call it with the supplied arguments.

set MLEXE=Microsoft Platform SDK\Bin\Win64\x86\AMD64\ml64.EXE

rem For the environment variables see also
rem http://msdn.microsoft.com/library/en-us/win64/win64/wow64_implementation_details.asp

if exist "%ProgramFiles%\%MLEXE%" (
  set ML64="%ProgramFiles%\%MLEXE%"
) else if exist "%ProgramW6432%\%MLEXE%" (
  set ML64="%ProgramW6432%\%MLEXE%"
) else (
  set ML64=ml64.exe
)

%ML64% %*
