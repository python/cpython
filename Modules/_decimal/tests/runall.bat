@ECHO OFF

rem Test all machine configurations, pydebug, refleaks, release build.

cd ..\..\..\


echo.
echo # ======================================================================
echo #                            Building Python
echo # ======================================================================
echo.

call "%VS100COMNTOOLS%\..\..\VC\vcvarsall.bat" x64
msbuild /noconsolelogger /target:clean PCbuild\pcbuild.sln /p:Configuration=Release /p:PlatformTarget=x64
msbuild /noconsolelogger /target:clean PCbuild\pcbuild.sln /p:Configuration=Debug /p:PlatformTarget=x64
msbuild /noconsolelogger PCbuild\pcbuild.sln /p:Configuration=Release /p:Platform=x64
msbuild /noconsolelogger PCbuild\pcbuild.sln /p:Configuration=Debug /p:Platform=x64

call "%VS100COMNTOOLS%\..\..\VC\vcvarsall.bat" x86
msbuild /noconsolelogger PCbuild\pcbuild.sln /p:Configuration=Release /p:Platform=Win32
msbuild /noconsolelogger PCbuild\pcbuild.sln /p:Configuration=Debug /p:Platform=Win32
echo.
echo.

echo.
echo # ======================================================================
echo #                       test_decimal: platform=x64
echo # ======================================================================
echo.

cd PCbuild\amd64

echo # ==================== refleak tests =======================
echo.
python_d.exe -m test -uall -R 2:2 test_decimal
echo.
echo.

echo # ==================== regular tests =======================
echo.
python.exe -m test -uall test_decimal
echo.
echo.

cd ..

echo.
echo # ======================================================================
echo #                       test_decimal: platform=x86
echo # ======================================================================
echo.

echo # ==================== refleak tests =======================
echo.
python_d.exe -m test -uall -R 2:2 test_decimal
echo.
echo.

echo # ==================== regular tests =======================
echo.
python.exe -m test -uall test_decimal
echo.
echo.

cd amd64

echo.
echo # ======================================================================
echo #                         deccheck: platform=x64
echo # ======================================================================
echo.

echo # ==================== debug build =======================
echo.
python_d.exe ..\..\Modules\_decimal\tests\deccheck.py
echo.
echo.

echo # =================== release build ======================
echo.
python.exe ..\..\Modules\_decimal\tests\deccheck.py
echo.
echo.

cd ..

echo.
echo # ======================================================================
echo #                         deccheck: platform=x86
echo # ======================================================================
echo.
echo.

echo # ==================== debug build =======================
echo.
python_d.exe ..\Modules\_decimal\tests\deccheck.py
echo.
echo.

echo # =================== release build ======================
echo.
python.exe ..\Modules\_decimal\tests\deccheck.py
echo.
echo.


cd ..\Modules\_decimal\tests



