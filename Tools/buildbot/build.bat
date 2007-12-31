@rem Used by the buildbot "compile" step.
cmd /c Tools\buildbot\external.bat
call "%VS71COMNTOOLS%vsvars32.bat"
cmd /q/c Tools\buildbot\kill_python.bat
devenv.com /useenv /build Debug PC\VS7.1\pcbuild.sln
