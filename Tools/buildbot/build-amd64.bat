@rem Used by the buildbot "compile" step.
cmd /c Tools\buildbot\external-amd64.bat
call "%VS90COMNTOOLS%vsvars32.bat"
REM cmd /q/c Tools\buildbot\kill_python.bat
vcbuild PCbuild\pcbuild.sln "Debug|x64"
