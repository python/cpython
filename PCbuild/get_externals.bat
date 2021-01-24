@echo off
setlocal
rem Simple script to fetch source for external libraries

if NOT DEFINED PCBUILD (set PCBUILD=%~dp0)
if NOT DEFINED EXTERNALS_DIR (set EXTERNALS_DIR=%PCBUILD%\..\externals)

set DO_FETCH=true
set DO_CLEAN=false
set IncludeLibffiSrc=false
set IncludeTkinterSrc=false
set IncludeSSLSrc=false

:CheckOpts
if "%~1"=="--no-tkinter" (set IncludeTkinter=false) & shift & goto CheckOpts
if "%~1"=="--no-openssl" (set IncludeSSL=false) & shift & goto CheckOpts
if "%~1"=="--no-libffi" (set IncludeLibffi=false) & shift & goto CheckOpts
if "%~1"=="--tkinter-src" (set IncludeTkinterSrc=true) & shift & goto CheckOpts
if "%~1"=="--openssl-src" (set IncludeSSLSrc=true) & shift & goto CheckOpts
if "%~1"=="--libffi-src" (set IncludeLibffiSrc=true) & shift & goto CheckOpts
if "%~1"=="--python" (set PYTHON=%2) & shift & shift & goto CheckOpts
if "%~1"=="--organization" (set ORG=%2) & shift & shift & goto CheckOpts
if "%~1"=="-c" (set DO_CLEAN=true) & shift & goto CheckOpts
if "%~1"=="--clean" (set DO_CLEAN=true) & shift & goto CheckOpts
if "%~1"=="--clean-only" (set DO_FETCH=false) & goto clean

rem Include old options for compatibility
if "%~1"=="--no-tkinter" shift & goto CheckOpts
if "%~1"=="--no-openssl" shift & goto CheckOpts

if "x%~1" NEQ "x" goto usage

if "%DO_CLEAN%"=="false" goto fetch
:clean
echo.Cleaning up external libraries.
if exist "%EXTERNALS_DIR%" (
    rem Sometimes this fails the first time; try it twice
    rmdir /s /q "%EXTERNALS_DIR%" || rmdir /s /q "%EXTERNALS_DIR%"
)

if "%DO_FETCH%"=="false" goto end
:fetch

if "%ORG%"=="" (set ORG=python)
call "%PCBUILD%\find_python.bat" "%PYTHON%"

if NOT DEFINED PYTHON (
    where /Q git || echo Python 3.6 could not be found or installed, and git.exe is not on your PATH && exit /B 1
)

echo.Fetching external libraries...

set libraries=
set libraries=%libraries%                                       bzip2-1.0.6
if NOT "%IncludeLibffiSrc%"=="false" set libraries=%libraries%  libffi
if NOT "%IncludeSSLSrc%"=="false" set libraries=%libraries%     openssl-1.1.1i
set libraries=%libraries%                                       sqlite-3.34.0.0
if NOT "%IncludeTkinterSrc%"=="false" set libraries=%libraries% tcl-core-8.6.10.0
if NOT "%IncludeTkinterSrc%"=="false" set libraries=%libraries% tk-8.6.10.0
if NOT "%IncludeTkinterSrc%"=="false" set libraries=%libraries% tix-8.4.3.6
set libraries=%libraries%                                       xz-5.2.2
set libraries=%libraries%                                       zlib-1.2.11

for %%e in (%libraries%) do (
    if exist "%EXTERNALS_DIR%\%%e" (
        echo.%%e already exists, skipping.
    ) else if NOT DEFINED PYTHON (
        echo.Fetching %%e with git...
        git clone --depth 1 https://github.com/%ORG%/cpython-source-deps --branch %%e "%EXTERNALS_DIR%\%%e"
    ) else (
        echo.Fetching %%e...
        %PYTHON% -E "%PCBUILD%\get_external.py" -O %ORG% -e "%EXTERNALS_DIR%" %%e
    )
)

echo.Fetching external binaries...

set binaries=
if NOT "%IncludeLibffi%"=="false"  set binaries=%binaries% libffi
if NOT "%IncludeSSL%"=="false"     set binaries=%binaries% openssl-bin-1.1.1i
if NOT "%IncludeTkinter%"=="false" set binaries=%binaries% tcltk-8.6.10.0
if NOT "%IncludeSSLSrc%"=="false"  set binaries=%binaries% nasm-2.11.06

for %%b in (%binaries%) do (
    if exist "%EXTERNALS_DIR%\%%b" (
        echo.%%b already exists, skipping.
    ) else if NOT DEFINED PYTHON (
        echo.Fetching %%b with git...
        git clone --depth 1 https://github.com/%ORG%/cpython-bin-deps --branch %%b "%EXTERNALS_DIR%\%%b"
    ) else (
        echo.Fetching %%b...
        %PYTHON% -E "%PCBUILD%\get_external.py" -b -O %ORG% -e "%EXTERNALS_DIR%" %%b
    )
)

echo Finished.
goto end

:usage
echo.Valid options: -c, --clean, --clean-only, --organization, --python,
echo.--no-tkinter, --no-openssl
echo.
echo.Pull all sources and binaries necessary for compiling optional extension
echo.modules that rely on external libraries.
echo.
echo.The --organization option determines which github organization to download
echo.from, the --python option determines which Python 3.6+ interpreter to use
echo.with PCbuild\get_external.py.
echo.
echo.Use the -c or --clean option to remove the entire externals directory.
echo.
echo.Use the --clean-only option to do the same cleaning, without pulling in
echo.anything new.
echo.
exit /b -1

:end
