@rem Used by the buildbot "buildmsi" step.
setlocal

pushd

@rem The MSI project is gone, so instead we will just
@rem build and then do a default layout.
call "%~dp0..\..\PCbuild\build.bat" -e -k -v %*
call "%~dp0..\..\python.exe" "%~dp0..\..\PC\layout" --preset-default -o "%~dp0..\..\PCbuild\output"

popd