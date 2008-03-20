@rem Used by the buildbot "clean" step.
call "%VS90COMNTOOLS%\..\..\VC\vcvarsall.bat" x86_amd64
cd PCbuild
@echo Deleting .pyc/.pyo files ...
del /s Lib\*.pyc Lib\*.pyo
vcbuild /clean pcbuild.sln "Release|x64"
vcbuild /clean pcbuild.sln "Debug|x64"
