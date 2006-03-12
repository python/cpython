@rem Used by the buildbot "compile" step.
call "%VS71COMNTOOLS%vsvars32.bat"
devenv.com /build Debug PCbuild\pcbuild.sln
