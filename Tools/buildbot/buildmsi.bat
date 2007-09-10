@rem Used by the buildbot "buildmsi" step.
cmd /c Tools\buildbot\external.bat
call "%VS71COMNTOOLS%vsvars32.bat"
cmd /q/c Tools\buildbot\kill_python.bat
devenv.com /useenv /build Release PCbuild\pcbuild.sln
bash.exe -c 'cd Doc;make PYTHON=python2.5 update htmlhelp'
"%ProgramFiles%\HTML Help Workshop\hhc.exe Doc\build\htmlhelp\pydoc.hhp
cd Tools\msi
del *.msi
%HOST_PYTHON% msi.py
