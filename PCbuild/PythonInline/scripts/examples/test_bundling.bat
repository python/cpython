@echo off
REM Python Inline Build and Test Script with Package Bundling
REM This script demonstrates the build-time package bundling capability

echo ================================
echo Python Inline Package Bundling Demo
echo ================================
echo.

REM Set up environment
set "CONFIG=Release"
set "PLATFORM=x64"
set "PROJECT_FILE=python_inline.vcxproj"

echo [1/6] Configuring packages...
echo.
echo Edit PythonInline\packaging\requirements.txt to add packages you want to bundle.
echo Current configuration:
type "PythonInline\packaging\requirements.txt"
echo.
pause

echo [2/6] Building Python Inline with package bundling...
echo.
echo This will:
echo - Install packages from requirements.txt
echo - Build the Python Inline executable  
echo - Bundle packages into the output directory
echo.
msbuild "%PROJECT_FILE%" /p:Configuration=%CONFIG% /p:Platform=%PLATFORM% /verbosity:minimal
if errorlevel 1 (
    echo.
    echo ? Build failed! Check the error messages above.
    pause
    exit /b 1
)

echo.
echo ? Build completed successfully!
echo.

echo [3/6] Verifying bundled packages...
echo.
if exist "amd64\site-packages" (
    echo ? Site-packages directory created: amd64\site-packages
    dir "amd64\site-packages" /b
) else (
    echo ??  No packages bundled (requirements.txt may be empty or commented)
)
echo.

echo [4/6] Testing basic functionality...
echo.
echo Testing basic Python execution:
"amd64\python_inline.exe" -c "print('? Python Inline is working!')"
echo.

echo Testing sys.path configuration:
"amd64\python_inline.exe" -c "import sys; print('Python path:'); [print(f'  {p}') for p in sys.path if 'site-packages' in p]"
echo.

echo [5/6] Testing bundled packages (if any)...
echo.
echo Listing configured packages:
"amd64\python_inline.exe" --list-packages
echo.

echo Testing requests package (if bundled):
"amd64\python_inline.exe" -c "try: import requests; print('? requests available:', requests.__version__); except ImportError: print('??  requests not bundled')"
echo.

echo Testing other common packages:
"amd64\python_inline.exe" -c "packages = ['json', 'sys', 'os', 'datetime', 'urllib']; [print(f'? {pkg} available') for pkg in packages if __import__(pkg)]"
echo.

echo [6/6] Demo completed!
echo.
echo ?? Python Inline with build-time package bundling is ready!
echo.
echo Key features demonstrated:
echo ? Automatic package installation during build
echo ? Package bundling into output directory
echo ? Runtime availability of bundled packages
echo ? No external installation required for users
echo.
echo To use bundled packages in your scripts:
echo   python_inline.exe -c "import requests; print('Hello, World!')"
echo   python_inline.exe --file your_script.py
echo   python_inline.exe --list-packages
echo.
echo To add more packages:
echo   1. Edit PythonInline\packaging\requirements.txt
echo   2. Run: msbuild python_inline.vcxproj
echo   3. New packages will be automatically bundled!
echo.
pause