@rem
@rem Searches for python.exe and may download a private copy from nuget.
@rem
@rem This file is supposed to modify the state of the caller (specifically
@rem the MSBUILD variable), so we do not use setlocal or echo, and avoid
@rem changing any other persistent state.
@rem

@set _Py_D=%~dp0

@rem First argument -q means only show the command in output
@if '%1' EQU '-q' (shift && set _Py_Quiet=1)

@rem No arguments provided means do full search
@if '%1' EQU '' goto :begin_search

@rem One argument may be the full path. Use a goto so we don't try to
@rem parse the next if statement - incorrect quoting in the multi-arg
@rem case can cause us to break immediately.
@if '%2' EQU '' goto :one_arg

@rem Entire command line may represent the full path if quoting failed.
@if exist "%*" (set PYTHON="%*") & (set _Py_Python_Source=from environment) & goto :found
@goto :begin_search

:one_arg
@if exist "%~1" (set PYTHON="%~1") & (set _Py_Python_Source=from environment) & goto :found

:begin_search
@set PYTHON=

@rem If PYTHON_FOR_BUILD is set, use that
@if NOT "%PYTHON_FOR_BUILD%"=="" @(set PYTHON="%PYTHON_FOR_BUILD%") && (set _Py_Python_Source=found as PYTHON_FOR_BUILD) && goto :found

@rem If there is an active virtual env, use that one
@if NOT "%VIRTUAL_ENV%"=="" (set PYTHON="%VIRTUAL_ENV%\Scripts\python.exe") & (set _Py_Python_Source=found in virtual env) & goto :found

@set _Py_EXTERNALS_DIR=%EXTERNALS_DIR%
@if "%_Py_EXTERNALS_DIR%"=="" (set _Py_EXTERNALS_DIR=%_Py_D%\..\externals)

@rem If we have Python in externals, use that one
@if exist "%_Py_EXTERNALS_DIR%\pythonx86\tools\python.exe" ("%_Py_EXTERNALS_DIR%\pythonx86\tools\python.exe" -Ec "import sys; assert sys.version_info[:2] >= (3, 10)" >nul 2>nul) && (set PYTHON="%_Py_EXTERNALS_DIR%\pythonx86\tools\python.exe") && (set _Py_Python_Source=found in externals directory) && goto :found || rmdir /Q /S "%_Py_EXTERNALS_DIR%\pythonx86"

@rem If HOST_PYTHON is recent enough, use that
@if NOT "%HOST_PYTHON%"=="" @%HOST_PYTHON% -Ec "import sys; assert sys.version_info[:2] >= (3, 10)" >nul 2>nul && (set PYTHON="%HOST_PYTHON%") && (set _Py_Python_Source=found as HOST_PYTHON) && goto :found

@rem If py.exe finds a recent enough version, use that one
@rem It is fine to add new versions to this list when they have released,
@rem but we do not use prerelease builds here.
@for %%p in (3.13 3.12 3.11 3.10) do @py -%%p -EV >nul 2>&1 && (set PYTHON=py -%%p) && (set _Py_Python_Source=found %%p with py.exe) && goto :found

@if NOT exist "%_Py_EXTERNALS_DIR%" mkdir "%_Py_EXTERNALS_DIR%"
@set _Py_NUGET=%NUGET%
@set _Py_NUGET_URL=%NUGET_URL%
@set _Py_HOST_PYTHON=%HOST_PYTHON%
@if "%_Py_HOST_PYTHON%"=="" set _Py_HOST_PYTHON=py
@if "%_Py_NUGET%"=="" (set _Py_NUGET=%_Py_EXTERNALS_DIR%\nuget.exe)
@if "%_Py_NUGET_URL%"=="" (set _Py_NUGET_URL=https://aka.ms/nugetclidl)
@if NOT exist "%_Py_NUGET%" (
    @if not "%_Py_Quiet%"=="1" @echo Downloading nuget...
    @rem NB: Must use single quotes around NUGET here, NOT double!
    @rem Otherwise, a space in the path would break things
    @rem If it fails, retry with any available copy of Python
    @powershell.exe -Command Invoke-WebRequest %_Py_NUGET_URL% -OutFile '%_Py_NUGET%'
    @if errorlevel 1 (
        @%_Py_HOST_PYTHON% -E "%_Py_D%\urlretrieve.py" "%_Py_NUGET_URL%" "%_Py_NUGET%"
    )
)

@if not "%_Py_Quiet%"=="1" @echo Installing Python via nuget...
@if not "%_Py_Quiet%"=="1" (
    @"%_Py_NUGET%" install pythonx86 -ExcludeVersion -OutputDirectory "%_Py_EXTERNALS_DIR%"
) else (
    @"%_Py_NUGET%" install pythonx86 -Verbosity quiet -ExcludeVersion -OutputDirectory "%_Py_EXTERNALS_DIR%"
)
@rem Quote it here; it's not quoted later because "py -x.y" wouldn't work
@if not errorlevel 1 (set PYTHON="%_Py_EXTERNALS_DIR%\pythonx86\tools\python.exe") & (set _Py_Python_Source=found on nuget.org) & goto :found


@set _Py_D=
@set _Py_Quiet=
@set _Py_Python_Source=
@set _Py_EXTERNALS_DIR=
@set _Py_NUGET=
@set _Py_NUGET_URL=
@set _Py_HOST_PYTHON=
@exit /b 1

:found
@if "%_Py_Quiet%"=="1" (@echo %PYTHON%) else @echo Using %PYTHON% (%_Py_Python_Source%)

@set _Py_D=
@set _Py_Quiet=
@set _Py_Python_Source=
@set _Py_EXTERNALS_DIR=
@set _Py_NUGET=
@set _Py_NUGET_URL=
@set _Py_HOST_PYTHON=
