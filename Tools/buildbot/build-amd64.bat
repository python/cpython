@rem Used by the buildbot "compile" step.
setlocal
cmd /c Tools\buildbot\external-amd64.bat
call "%VS71COMNTOOLS%vsvars32.bat"
REM cmd /q/c Tools\buildbot\kill_python.bat
devenv.com /build ReleaseAMD64 PC\VS7.1\pcbuild.sln
