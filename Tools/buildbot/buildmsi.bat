@rem Used by the buildbot "buildmsi" step.
setlocal

pushd

@rem build both snapshot MSIs
call "%~dp0..\msi\build.bat" -x86 -x64

popd