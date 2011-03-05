@rem Used by the buildbot "buildmsi" step.

cmd /c Tools\buildbot\external.bat
@rem build release versions of things
call "%VS90COMNTOOLS%vsvars32.bat"

@rem build Python
vcbuild /useenv PCbuild\pcbuild.sln "Release|Win32"

@rem build the documentation
bash.exe -c 'cd Doc;make PYTHON=python2.5 update htmlhelp'
"%ProgramFiles%\HTML Help Workshop\hhc.exe" Doc\build\htmlhelp\python26a3.hhp

@rem build the MSI file
cd PC
nmake /f icons.mak
cd ..\Tools\msi
del *.msi
nmake /f msisupport.mak
%HOST_PYTHON% msi.py

