@rem Used by the buildbot "compile" step.
cmd /c Tools\buildbot\external.bat
call "%VS100COMNTOOLS%vsvars32.bat"
cmd /c Tools\buildbot\clean.bat
msbuild /p:useenv=true PCbuild\kill_python.vcxproj /p:Configuration=Debug /p:PlatformTarget=x86
PCbuild\kill_python_d.exe
msbuild /p:useenv=true PCbuild\pcbuild.sln /p:Configuration=Debug /p:PlatformTarget=x86

