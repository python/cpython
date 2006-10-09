@rem Used by the buildbot "compile" step.
cmd /c Tools\buildbot\external.bat
call "%VS71COMNTOOLS%vsvars32.bat"
cd PCbuild
devenv.com /useenv /build Debug pcbuild.sln