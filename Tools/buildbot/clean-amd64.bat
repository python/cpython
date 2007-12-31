@rem Used by the buildbot "clean" step.
call "%VS71COMNTOOLS%vsvars32.bat"
cd PC\VS7.1
@echo Deleting .pyc/.pyo files ...
del /s Lib\*.pyc Lib\*.pyo
devenv.com /clean ReleaseAMD64 pcbuild.sln
