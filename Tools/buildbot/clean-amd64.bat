@rem Used by the buildbot "clean" step.
call "%VS100COMNTOOLS%\..\..\VC\vcvarsall.bat" x86_amd64
@echo Deleting .pyc/.pyo files ...
del /s Lib\*.pyc Lib\*.pyo
@echo Deleting test leftovers ...
rmdir /s /q build
cd PCbuild
msbuild /target:clean pcbuild.sln /p:Configuration=Release /p:PlatformTarget=x64
msbuild /target:clean pcbuild.sln /p:Configuration=Debug /p:PlatformTarget=x64
cd ..
