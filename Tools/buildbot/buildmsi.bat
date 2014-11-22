@rem Used by the buildbot "buildmsi" step.
setlocal

set cwd=%CD%

@rem build release versions of things
call "%~dp0build.bat" -c Release

@rem build the documentation
call "%~dp0..\..\Doc\make.bat" htmlhelp

@rem build the MSI file
call "%~dp0..\..\PCBuild\env.bat" x86
cd "%~dp0..\..\PC"
nmake /f icons.mak
cd ..\Tools\msi
del *.msi
nmake /f msisupport.mak
%HOST_PYTHON% msi.py

cd "%cwd%"
