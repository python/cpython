@rem Used by the buildbot "clean" step.
call "%VS71COMNTOOLS%vsvars32.bat"
cd PCbuild
@echo Deleting .pyc/.pyo files ...
python.exe rmpyc.py
devenv.com /clean ReleaseAMD64 pcbuild.sln
