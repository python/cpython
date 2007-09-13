@rem Used by the buildbot "buildmsi" step.

cmd /c Tools\buildbot\external.bat
@rem build release versions of things
call "%VS71COMNTOOLS%vsvars32.bat"
if not exist db-4.4.20\build_win32\release\libdb44s.lib (
   devenv db-4.4.20\build_win32\Berkeley_DB.sln /build Release /project db_static
)

@rem build Python
cmd /q/c Tools\buildbot\kill_python.bat
devenv.com /useenv /build Release PCbuild\pcbuild.sln

@rem build the documentation
bash.exe -c 'cd Doc;make PYTHON=python2.5 update htmlhelp'
"%ProgramFiles%\HTML Help Workshop\hhc.exe" Doc\build\htmlhelp\pydoc.hhp

@rem buold the MSI file
cd PC
nmake /f icons.mak
cd ..\Tools\msi
del *.msi
nmake /f msisupport.mak
%HOST_PYTHON% msi.py
