Helpers used by buildbot-driven core Python testing.

external.bat
build.bat
test.bat
clean.bat
    On Windows, these scripts are executed by the code sent
    from the buildbot master to the slaves.

fetch_data_files.py
    Download all the input files various encoding tests want.  This is
    used by build.bat on Windows (but could be used on any platform).
    Note that in Python >= 2.5, the encoding tests download input files
    automatically.
