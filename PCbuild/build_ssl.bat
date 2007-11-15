if "%1" == "ReleaseAMD64" call "%MSSdk%\SetEnv" /XP64 /RETAIL

@echo off
if not defined HOST_PYTHON (
  if %1 EQU Debug (
    set HOST_PYTHON=python_d.exe
    if not exist python30_d.dll exit 1
  ) ELSE (
    set HOST_PYTHON=python.exe
    if not exist python30.dll exit 1
  )
)
%HOST_PYTHON% build_ssl.py %1 %2

