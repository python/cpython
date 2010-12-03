from test import regrtest, support


TEMPDIR, TESTCWD = regrtest._make_temp_dir_for_build(regrtest.TEMPDIR)
regrtest.TEMPDIR = TEMPDIR
regrtest.TESTCWD = TESTCWD

# Run the tests in a context manager that temporary changes the CWD to a
# temporary and writable directory. If it's not possible to create or
# change the CWD, the original CWD will be used. The original CWD is
# available from support.SAVEDCWD.
with support.temp_cwd(TESTCWD, quiet=True):
    regrtest.main()
