@echo off

rem This file is UTF-8 encoded, so we need to update the current code page while executing it.
for /f %%a in ('%~dp0python.exe -Ic "import ctypes; print(ctypes.windll.kernel32.GetConsoleOutputCP())"') do (set "_OLD_CODEPAGE=%%a")

if defined _OLD_CODEPAGE (
    %~dp0python.exe -Ic "import ctypes; ctypes.windll.kernel32.SetConsoleOutputCP(65001)"
)

set "VIRTUAL_ENV=__VENV_DIR__"

if not defined PROMPT (
    set "PROMPT=$P$G"
)

if defined _OLD_VIRTUAL_PROMPT (
    set "PROMPT=%_OLD_VIRTUAL_PROMPT%"
)

if defined _OLD_VIRTUAL_PYTHONHOME (
    set "PYTHONHOME=%_OLD_VIRTUAL_PYTHONHOME%"
)

set "_OLD_VIRTUAL_PROMPT=%PROMPT%"
set "PROMPT=__VENV_PROMPT__%PROMPT%"

if defined PYTHONHOME (
    set "_OLD_VIRTUAL_PYTHONHOME=%PYTHONHOME%"
    set PYTHONHOME=
)

if defined _OLD_VIRTUAL_PATH (
    set "PATH=%_OLD_VIRTUAL_PATH%"
) else (
    set "_OLD_VIRTUAL_PATH=%PATH%"
)

set "PATH=%VIRTUAL_ENV%\__VENV_BIN_NAME__;%PATH%"

:END
if defined _OLD_CODEPAGE (
    %~dp0python.exe -Ic "import ctypes; ctypes.windll.kernel32.SetConsoleOutputCP(%_OLD_CODEPAGE%)"
    set "_OLD_CODEPAGE="
)
