@rem Used by the buildbot "compile" step.
set HOST_PYTHON=%CD%\PCbuild\amd64\python_d.exe
cmd /c Tools\buildbot\external-amd64.bat
call "%VS90COMNTOOLS%\..\..\VC\vcvarsall.bat" x86_amd64
cmd /c Tools\buildbot\clean-amd64.bat
vcbuild /useenv PCbuild\kill_python.vcproj "Debug|x64" && PCbuild\amd64\kill_python_d.exe
vcbuild PCbuild\pcbuild.sln "Debug|x64"
