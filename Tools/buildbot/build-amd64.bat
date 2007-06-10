@rem Used by the buildbot "compile" step.
REM cmd /c Tools\buildbot\external.bat
call "%VS71COMNTOOLS%vsvars32.bat"
REM cmd /q/c Tools\buildbot\kill_python.bat
devenv.com /build ReleaseAMD64 PCbuild\pcbuild.sln
