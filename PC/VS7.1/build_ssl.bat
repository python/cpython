if "%1" == "ReleaseAMD64" call "%MSSdk%\SetEnv" /XP64 /RETAIL

@echo off
if not defined HOST_PYTHON (
  if %1 EQU Debug (
    set HOST_PYTHON=python_d.exe
  ) ELSE (
    set HOST_PYTHON=python.exe
  )
)
%HOST_PYTHON% build_ssl.py %1 %2

