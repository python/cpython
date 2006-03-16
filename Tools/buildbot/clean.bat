@rem Used by the buildbot "clean" step.
call "%VS71COMNTOOLS%vsvars32.bat"
cd PCbuild
devenv.com /clean Debug pcbuild.sln
@echo Deleting .pyc/.pyo files ...
python_d.exe rmpyc.py
