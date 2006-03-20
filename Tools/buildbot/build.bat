@rem Used by the buildbot "compile" step.
cmd /c Tools\buildbot\external.bat
call "%VS71COMNTOOLS%vsvars32.bat"
cd PCbuild
devenv.com /useenv /build Debug pcbuild.sln
@rem Fetch encoding test files.  Note that python_d needs to be built first.
if not exist BIG5.TXT python_d.exe ..\Tools\buildbot\fetch_data_files.py