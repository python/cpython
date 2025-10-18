@rem
@rem Searches for MSBuild.exe. This is the only tool we need to initiate
@rem a build, so we no longer search for the full VC toolset.
@rem
@rem This file is supposed to modify the state of the caller (specifically
@rem the MSBUILD variable), so we do not use setlocal or echo, and avoid
@rem changing any other persistent state.
@rem

@rem No arguments provided means do full search
@if '%1' EQU '' goto :begin_search

@rem One argument may be the full path. Use a goto so we don't try to
@rem parse the next if statement - incorrect quoting in the multi-arg
@rem case can cause us to break immediately.
@if '%2' EQU '' goto :one_arg

@rem Entire command line may represent the full path if quoting failed.
@if exist "%*" (set MSBUILD="%*") & (set _Py_MSBuild_Source=environment) & goto :found
@goto :begin_search

:one_arg
@if exist "%~1" (set MSBUILD="%~1") & (set _Py_MSBuild_Source=environment) & goto :found

:begin_search
@set MSBUILD=

@rem If msbuild.exe is on the PATH, assume that the user wants that one.
@where msbuild > "%TEMP%\msbuild.loc" 2> nul && set /P MSBUILD= < "%TEMP%\msbuild.loc" & del "%TEMP%\msbuild.loc"
@if exist "%MSBUILD%" set MSBUILD="%MSBUILD%" & (set _Py_MSBuild_Source=PATH) & goto :found

@rem VS 2017 and later provide vswhere.exe, which can be used
@if not exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" goto :skip_vswhere
@set _Py_MSBuild_Root=
@for /F "tokens=*" %%i in ('"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -property installationPath -latest -prerelease -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64') DO @(set _Py_MSBuild_Root=%%i\MSBuild)
@if not defined _Py_MSBuild_Root goto :skip_vswhere
@for %%j in (Current 15.0) DO @if exist "%_Py_MSBuild_Root%\%%j\Bin\msbuild.exe" (set MSBUILD="%_Py_MSBuild_Root%\%%j\Bin\msbuild.exe")
@set _Py_MSBuild_Root=
@if defined MSBUILD @if exist %MSBUILD% (set _Py_MSBuild_Source=Visual Studio installation) & goto :found
:skip_vswhere

:found
@pushd %MSBUILD% >nul 2>nul
@if not ERRORLEVEL 1 @(
  @if exist msbuild.exe @(set MSBUILD="%CD%\msbuild.exe") else @(set MSBUILD=)
  @popd
)

@if defined MSBUILD @echo Using %MSBUILD% (found in the %_Py_MSBuild_Source%)
@if not defined MSBUILD @echo Failed to find MSBuild
@set _Py_MSBuild_Source=
@if not defined MSBUILD @exit /b 1
@exit /b 0
