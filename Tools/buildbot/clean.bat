@rem Used by the buildbot "clean" step.
call "%VS71COMNTOOLS%vsvars32.bat"
devenv.com /clean Debug PCbuild\pcbuild.sln
