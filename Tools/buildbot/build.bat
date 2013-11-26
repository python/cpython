@rem Used by the buildbot "compile" step.
cmd /c Tools\buildbot\external.bat
call "%VS100COMNTOOLS%vsvars32.bat"
cmd /c Tools\buildbot\clean.bat

msbuild PCbuild\pcbuild.sln /p:Configuration=Debug /p:Platform=Win32

