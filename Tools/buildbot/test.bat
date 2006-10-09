@rem Used by the buildbot "test" step.
cd PCbuild
@rem Fetch encoding test files.  Note that python_d needs to be built first.
if not exist BIG5.TXT python_d.exe ..\Tools\buildbot\fetch_data_files.py
call rt.bat -d -q -uall -rw
