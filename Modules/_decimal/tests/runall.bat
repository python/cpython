@ECHO OFF

rem Test all machine configurations, pydebug, refleaks, release build.

cd ..

call vcvarsall x64
echo.
echo # ======================================================================
echo #                       test_decimal: platform=x64
echo # ======================================================================
echo.

cd ..\..\PCbuild
echo # ==================== refleak tests =======================
echo.
echo building python ...
echo.
vcbuild /clean pcbuild.sln > NUL 2>&1
vcbuild pcbuild.sln "Debug|x64" > NUL 2>&1
amd64\python_d.exe -m test -uall -R 2:2 test_decimal
echo.

echo # ==================== regular tests =======================
echo.
echo building python ...
echo.
vcbuild /clean pcbuild.sln > NUL 2>&1
vcbuild pcbuild.sln "Release|x64" > NUL 2>&1
amd64\python.exe -m test -uall test_decimal
echo.
echo.


call vcvarsall x86
echo.
echo # ======================================================================
echo #                       test_decimal: platform=x86
echo # ======================================================================
echo.

echo # ==================== refleak tests =======================
echo.
echo building python ...
echo.
vcbuild /clean pcbuild.sln > NUL 2>&1
vcbuild pcbuild.sln "Debug|win32" > NUL 2>&1
python_d.exe -m test -uall -R 2:2 test_decimal
echo.

echo # ==================== regular tests =======================
echo.
echo building python ...
echo.
vcbuild /clean pcbuild.sln > NUL 2>&1
vcbuild pcbuild.sln "Release|win32" > NUL 2>&1
python.exe -m test -uall test_decimal
echo.
echo.


call vcvarsall x64
echo.
echo # ======================================================================
echo #                         deccheck: platform=x64
echo # ======================================================================
echo.
echo.
echo # ==================== debug build =======================
echo.
echo building python ...
echo.
vcbuild /clean pcbuild.sln > NUL 2>&1
vcbuild pcbuild.sln "Debug|x64" > NUL 2>&1
amd64\python_d.exe ..\Modules\_decimal\tests\deccheck.py
echo.
echo.

echo # =================== release build ======================
echo.
echo building python ...
echo.
vcbuild /clean pcbuild.sln > NUL 2>&1
vcbuild pcbuild.sln "Release|x64" > NUL 2>&1
amd64\python.exe ..\Modules\_decimal\tests\deccheck.py
echo.
echo.


call vcvarsall x86
echo.
echo # ======================================================================
echo #                         deccheck: platform=x86
echo # ======================================================================
echo.
echo.
echo # ==================== debug build =======================
echo.
echo building python ...
echo.
vcbuild /clean pcbuild.sln > NUL 2>&1
vcbuild pcbuild.sln "Debug|win32" > NUL 2>&1
python_d.exe ..\Modules\_decimal\tests\deccheck.py
echo.
echo.

echo # =================== release build ======================
echo.
echo building python ...
echo.
vcbuild /clean pcbuild.sln > NUL 2>&1
vcbuild pcbuild.sln "Release|win32" > NUL 2>&1
python.exe ..\Modules\_decimal\tests\deccheck.py
echo.
echo.


cd ..\Modules\_decimal\tests



