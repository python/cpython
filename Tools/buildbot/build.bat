@rem Used by the buildbot "compile" step.
cmd /c Tools\buildbot\external.bat
call "%VS90COMNTOOLS%vsvars32.bat"
cmd /q/c Tools\buildbot\kill_python.bat
vcbuild /useenv PCbuild\pcbuild.sln "Debug|Win32"

