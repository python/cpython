@echo off
setlocal
set D=%~dp0
set PCBUILD=%D%..\..\PCBuild\

set BUILDX86=
set BUILDX64=
set BUILDDOC=

:CheckOpts
if '%1'=='-x86' (set BUILDX86=1) && shift && goto CheckOpts
if '%1'=='-x64' (set BUILDX64=1) && shift && goto CheckOpts
if '%1'=='--doc' (set BUILDDOC=1) && shift && goto CheckOpts

if not defined BUILDX86 if not defined BUILDX64 (set BUILDX86=1) && (set BUILDX64=1)

call "%PCBUILD%env.bat" x86

if defined BUILDX86 (
    call "%PCBUILD%build.bat" -d
    if errorlevel 1 goto :eof
    call "%PCBUILD%build.bat"
    if errorlevel 1 goto :eof
)
if defined BUILDX64 (
    call "%PCBUILD%build.bat" -p x64 -d
    if errorlevel 1 goto :eof
    call "%PCBUILD%build.bat" -p x64
    if errorlevel 1 goto :eof
)

if defined BUILDDOC (
    call "%PCBUILD%..\Doc\make.bat" htmlhelp
    if errorlevel 1 goto :eof
)

if defined BUILDX86 (
    "%PCBUILD%win32\python.exe" "%D%get_wix.py"
    msbuild "%D%bundle\snapshot.wixproj"
    if errorlevel 1 goto :eof
)
if defined BUILDX64 (
    "%PCBUILD%amd64\python.exe" "%D%get_wix.py"
    msbuild "%D%bundle\snapshot.wixproj" /p:Platform=x64
    if errorlevel 1 goto :eof
)
