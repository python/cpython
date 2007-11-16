@rem Used by the buildbot "clean" step.
call "%VS71COMNTOOLS%vsvars32.bat"
cd PCbuild
@echo Deleting .pyc/.pyo files ...
del /s Lib\*.pyc Lib\*.pyo
devenv.com /clean Release pcbuild.sln
devenv.com /clean Debug pcbuild.sln
