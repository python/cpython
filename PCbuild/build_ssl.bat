@echo off
if not defined HOST_PYTHON (
  if %1 EQU Debug (
    set HOST_PYTHON=python_d.exe
    if not exist python33_d.dll exit 1
  ) ELSE (
    set HOST_PYTHON=python.exe
    if not exist python33.dll exit 1
  )
)
%HOST_PYTHON% build_ssl.py %1 %2 %3

