@setlocal
@echo off

set D=%~dp0
set PCBUILD=%D%..\..\PCBuild\

set HOST=
set USER=
set TARGET=
set DRYRUN=false

:CheckOpts
if "%1" EQU "-h" goto Help
if "%1" EQU "-o" (set HOST=%~2) && shift && shift && goto CheckOpts
if "%1" EQU "--host" (set HOST=%~2) && shift && shift && goto CheckOpts
if "%1" EQU "-u" (set USER=%~2) && shift && shift && goto CheckOpts
if "%1" EQU "--user" (set USER=%~2) && shift && shift && goto CheckOpts
if "%1" EQU "-t" (set TARGET=%~2) && shift && shift && goto CheckOpts
if "%1" EQU "--target" (set TARGET=%~2) && shift && shift && goto CheckOpts
if "%1" EQU "--dry-run" (set DRYRUN=true) && shift && goto CheckOpts

if not defined PLINK where plink > "%TEMP%\plink.loc" 2> nul && set /P PLINK= < "%TEMP%\plink.loc" & del "%TEMP%\plink.loc"
if not defined PLINK where /R "%ProgramFiles(x86)%\PuTTY" plink > "%TEMP%\plink.loc" 2> nul && set /P PLINK= < "%TEMP%\plink.loc" & del "%TEMP%\plink.loc"
if not defined PLINK where /R "%ProgramFiles(x86)%" plink > "%TEMP%\plink.loc" 2> nul && set /P PLINK= < "%TEMP%\plink.loc" & del "%TEMP%\plink.loc"
if not defined PLINK echo Cannot locate plink.exe & exit /B 1
echo Found plink.exe at %PLINK%

if not defined PSCP where pscp > "%TEMP%\pscp.loc" 2> nul && set /P pscp= < "%TEMP%\pscp.loc" & del "%TEMP%\pscp.loc"
if not defined PSCP where /R "%ProgramFiles(x86)%\PuTTY" pscp > "%TEMP%\pscp.loc" 2> nul && set /P pscp= < "%TEMP%\pscp.loc" & del "%TEMP%\pscp.loc"
if not defined PSCP where /R "%ProgramFiles(x86)%" pscp > "%TEMP%\pscp.loc" 2> nul && set /P pscp= < "%TEMP%\pscp.loc" & del "%TEMP%\pscp.loc"
if not defined PSCP echo Cannot locate pscp.exe & exit /B 1
echo Found pscp.exe at %PSCP%

if not defined GPG where gpg2 > "%TEMP%\gpg.loc" 2> nul && set /P GPG= < "%TEMP%\gpg.loc" & del "%TEMP%\gpg.loc"
if not defined GPG where /R "%PCBUILD%..\externals" gpg2 > "%TEMP%\gpg.loc" 2> nul && set /P GPG= < "%TEMP%\gpg.loc" & del "%TEMP%\gpg.loc"
if not defined GPG echo Cannot locate gpg2.exe. Signatures will not be uploaded & pause
echo Found gpg2.exe at %GPG%

call "%PCBUILD%env.bat" > nul 2> nul
pushd "%D%"
msbuild /v:m /nologo uploadrelease.proj /t:Upload /p:Platform=x86
msbuild /v:m /nologo uploadrelease.proj /t:Upload /p:Platform=x64 /p:IncludeDoc=false
msbuild /v:m /nologo uploadrelease.proj /t:ShowHashes /p:Platform=x86
msbuild /v:m /nologo uploadrelease.proj /t:ShowHashes /p:Platform=x64 /p:IncludeDoc=false
popd
exit /B 0

:Help
echo uploadrelease.bat --host HOST --user USERNAME [--target TARGET] [--dry-run] [-h]
echo.
echo    --host (-o)     Specify the upload host (required)
echo    --user (-u)     Specify the user on the host (required)
echo    --target (-t)   Specify the target directory on the host
echo    --dry-run       Display commands and filenames without executing them
echo    -h              Display this help information
echo.
