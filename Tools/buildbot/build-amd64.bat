@rem Used by the buildbot "compile" step.
cmd /c Tools\buildbot\external-amd64.bat
call "%VS100COMNTOOLS%\..\..\VC\vcvarsall.bat" x86_amd64
cmd /c Tools\buildbot\clean-amd64.bat
msbuild /p:useenv=true PCbuild\kill_python.vcxproj /p:Configuration=Debug /p:PlatformTarget=x64
PCbuild\amd64\kill_python_d.exe
msbuild /p:useenv=true PCbuild\pcbuild.sln /p:Configuration=Debug /p:Platform=x64
